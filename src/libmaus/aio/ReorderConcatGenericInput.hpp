/*
    libmaus
    Copyright (C) 2009-2013 German Tischler
    Copyright (C) 2011-2013 Genome Research Limited

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#if ! defined(REORDERCONCATGENERICINPUT_HPP)
#define REORDERCONCATGENERICINPUT_HPP

#include <libmaus/aio/FileFragment.hpp>
#include <libmaus/util/ConcatRequest.hpp>
#include <cerrno>

namespace libmaus
{
	namespace aio
	{
		template < typename input_type >
		struct ReorderConcatGenericInput
		{
			typedef input_type value_type;
			typedef ReorderConcatGenericInput<value_type> this_type;
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			
			private:
			std::vector < FileFragment > V;
			std::vector < FileFragment >::const_iterator it;
			typedef std::ifstream reader_type;
			typedef ::libmaus::util::unique_ptr<reader_type>::type reader_ptr_type;
			
			uint64_t const bufsize;
			::libmaus::autoarray::AutoArray < input_type > B;
			input_type * const pa;
			input_type * pc;
			input_type * pe;
			
			reader_ptr_type reader;
			uint64_t restwords;
			
			uint64_t totalwords;
			uint64_t offset;
			
			// #define GETDEBUG
			#if defined(GETDEBUG)
			public:
			uint64_t getptr;
			private:
			#endif
			
			bool fillBuffer()
			{
		                while ( (! restwords) && (it != V.end()) )
		                        init();
		                
		                // std::cerr << "rest words " << restwords << std::endl;
		                
                                if ( restwords )
                                {
                                        uint64_t const toreadwords = std::min(totalwords,std::min(restwords,bufsize));
                                        uint64_t const toreadbytes = toreadwords*sizeof(input_type);
                                        reader->read( reinterpret_cast<char *>(pa), toreadbytes );

                                        // std::cerr << "total words " << totalwords << " buf size " << bufsize << " toreadwords " << toreadwords << std::endl;

			                if ( (! reader) || (reader->gcount() != static_cast<int64_t>(toreadbytes) ) )
			                {
			                        ::libmaus::exception::LibMausException se;
			                        se.getStream() << "Failed to read in ReorderConcatGenericInput<" 
			                                << ::libmaus::util::Demangle::demangle<input_type>() << ">::fillBuffer(): " << strerror(errno);
                                                se.finish();
                                                throw se;                
			                }

			                pc = pa;
			                pe = pa + toreadwords;
			                restwords -= toreadwords;
			                totalwords -= toreadwords;
			                
			                return (toreadwords != 0);
                                }
                                else
                                {
                                        return false;
                                }
			}
			
			
			bool init()
			{
			        if ( it != V.end() )
			        {
			                // std::cerr << "Switching to " << it->toString() << " offset " << offset << std::endl;
			        
			                reader = UNIQUE_PTR_MOVE(reader_ptr_type(new reader_type(it->filename.c_str(),std::ios::binary)));
			                if ( !reader->is_open() )
			                {
			                        ::libmaus::exception::LibMausException se;
			                        se.getStream() << "Failed to open file " << it->filename << " in ReorderConcatGenericInput<" 
			                                << ::libmaus::util::Demangle::demangle<input_type>() << ">::init(): " << strerror(errno);
                                                se.finish();
                                                throw se;
			                }
			                
			                assert ( offset <= it->len );

			                restwords = it->len - offset ;
			                restwords = std::min(restwords,totalwords);
			                uint64_t const seekword = it->offset+offset;
			                reader->seekg( seekword * sizeof(input_type) , ::std::ios::beg);
			                offset = 0;
			                
			                if ( 
			                        (! reader.get()) || 
			                        (static_cast<int64_t>(reader->tellg()) != static_cast<int64_t>(seekword * sizeof(input_type))) 
                                        )
			                {
			                        ::libmaus::exception::LibMausException se;
			                        se.getStream() << "Failed to seek to word " << seekword << " in file " << it->filename << " in ReorderConcatGenericInput<" 
			                                << ::libmaus::util::Demangle::demangle<input_type>() << ">::init(): " << strerror(errno);
                                                se.finish();
                                                throw se;                
			                }
			                
			                it++;
			                
			                return true;
			        }
			        else
			        {
			                restwords = 0;
			                return false;
			        }
			}
			
			public:
			static unsigned int getDefaultBufferSize()
			{
				return 64*1024;
			}
			
			ReorderConcatGenericInput(
			        std::vector<FileFragment> const & rV,
			        uint64_t const rbufsize = getDefaultBufferSize(),
			        uint64_t const rlimit = std::numeric_limits<uint64_t>::max(),
			        uint64_t const roffset = 0
                        )
			: V(rV), it(V.begin()), bufsize(rbufsize), B(bufsize), pa(B.begin()), pc(pa), pe(pa), 
			  reader(), restwords(0),
			  totalwords( std::min(FileFragment::getTotalLength(V)-roffset,rlimit) ),
			  offset(roffset)
			  #if defined(GETDEBUG)
			  , getptr(0)
			  #endif
			{
			        // std::cerr << "total words " << totalwords << std::endl;
			
			        while ( (it != V.end()) && (offset >= it->len) )
			        {
			                offset -= it->len;
			                ++it;
                                }
			}
			
			uint64_t getTotalWords() const
			{
			        return totalwords;
			}
			
			bool getNext(input_type & word)
			{
			        if ( pc == pe )
			        {
			                bool const ok = fillBuffer();
			                if ( ! ok )
			                        return false;
			        }
			        
			        word = *(pc++);
			        return true;
			}

			bool peekNext(input_type & word)
			{
			        if ( pc == pe )
			        {
			                bool const ok = fillBuffer();
			                if ( ! ok )
			                        return false;
			        }
			        
			        word = *pc;
			        return true;
			}
			
			int64_t getNextCharacter()
			{
			        input_type word;
			        
			        if ( getNext(word) )
			                return static_cast<int>(word);
                                else
                                        return -1;
			}
			
			int64_t peekNextCharacter()
			{
				input_type word;
				
				if ( peekNext(word) )
					return word;
				else
					return -1;
			}
			
			int64_t get()
			{
			        #if defined(GETDEBUG)
			        getptr++;
        	                #endif 
			        return getNextCharacter();
			}
			
			int64_t peek()
			{
				return peekNextCharacter();
			}

			typedef std::vector < ::libmaus::aio::FileFragment > fragment_vector;

                        static fragment_vector loadFragmentVector(std::vector<std::string> const & infilenames)
                        {
                                fragment_vector frags(infilenames.size());
                                
                                for ( int64_t i = 0; i < static_cast<int64_t>(infilenames.size()); ++i )
                                {
                                        std::string const infile = infilenames[i];
                                        
                                        uint64_t const fs = ::libmaus::util::GetFileSize::getFileSize(infile);
                                                                                
                                        if ( fs % sizeof(value_type) )
                                        {
                                                ::libmaus::exception::LibMausException se;
                                                se.getStream() << "Size " << fs << " of file " << infile << " is not a multiple of " <<
                                                        sizeof(value_type);
                                                se.finish();
                                                throw se;
                                        }
                                        
                                        uint64_t const n = fs / sizeof(value_type);
                                        frags[i] = ::libmaus::aio::FileFragment ( infile , 0 , n );
                                }
                                
                                return frags;
                        }

	        	static fragment_vector loadFragmentVectorRequest(std::string const & requestfilename)
        		{
	        	        ::libmaus::util::ConcatRequest::unique_ptr_type req = UNIQUE_PTR_MOVE(::libmaus::util::ConcatRequest::load(requestfilename));
		                return loadFragmentVector(req->infilenames);
                        }

                        static unique_ptr_type openConcatFile(
                                std::string const & requestfilename, 
                                uint64_t const bufsize = 64*1024, 
                                uint64_t const limit = std::numeric_limits<uint64_t>::max(), 
                                uint64_t const offset = 0
                        )
                        {
	        	        ::libmaus::util::ConcatRequest::unique_ptr_type req = UNIQUE_PTR_MOVE(::libmaus::util::ConcatRequest::load(requestfilename));
		                fragment_vector frags = loadFragmentVector(req->infilenames);
                                return UNIQUE_PTR_MOVE(unique_ptr_type( new this_type(frags,bufsize,limit,offset) ));
                        }

                        static unique_ptr_type openConcatFile(
                                std::vector < std::string > const & filenames,
                                uint64_t const bufsize = 64*1024, 
                                uint64_t const limit = std::numeric_limits<uint64_t>::max(), 
                                uint64_t const offset = 0
                        )
                        {
                                fragment_vector const frags = loadFragmentVector(filenames);
                                return UNIQUE_PTR_MOVE(unique_ptr_type( new this_type(frags,bufsize,limit,offset) ));
                        }

                        static uint64_t getSize(std::vector < std::string > const & filenames)
                        {
                                return ::libmaus::util::GetFileSize::getFileSize(filenames) / sizeof(value_type);
                        }
                        static uint64_t getSize(std::string const & requestfilename)
                        {
	        	        ::libmaus::util::ConcatRequest::unique_ptr_type req = UNIQUE_PTR_MOVE(::libmaus::util::ConcatRequest::load(requestfilename));
		                fragment_vector frags = loadFragmentVector(req->infilenames);
		                uint64_t len = 0;
		                
		                for ( uint64_t i = 0; i < frags.size(); ++i )
		                        len += frags[i].len;
                                
                                return len;
                        }

                        static void toSerial(
                        	std::vector < ::libmaus::aio::FileFragment > const & fragments,
                        	std::string const & outputfilename
                        	)
			{
				std::ofstream ostr(outputfilename.c_str(),std::ios::binary);
				
				if ( (! ostr) || (!(ostr.is_open())) )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Failed to open file " << outputfilename << " for writing: " 
						<< strerror(errno)
						<< std::endl;
					se.finish();
					throw se;
				}
			
				toSerial(fragments,ostr,outputfilename);
				
				ostr.close();
			}

                        static void toSerial(
                        	std::vector < ::libmaus::aio::FileFragment > const & fragments,
                        	std::ostream & ostr,
                        	std::string const outputfilename = "-"
                        	)
			{
				uint64_t const bufsize = 64*1024;
				unique_ptr_type infile = UNIQUE_PTR_MOVE(unique_ptr_type(new this_type(fragments,bufsize)));
				uint64_t n = infile->getTotalWords();
				::libmaus::autoarray::AutoArray<value_type> B(bufsize,false);
				
				while ( n )
				{
					uint64_t const toread = std::min(n,bufsize);
					uint64_t const r = infile->fillBlock(B.get(),toread);

					ostr.write ( 
						reinterpret_cast<char const *>(B.get()),
						r * sizeof(value_type) );
						
					if ( (! ostr) )
					{
						::libmaus::exception::LibMausException se;
						se.getStream() << "Failed to write data to file " << outputfilename 
							<< ": " 
							<< strerror(errno)
							<< std::endl;
						se.finish();
						throw se;
					}
					
					n -= r;
				}
				
				ostr.flush();

				if ( (! ostr) )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Failed to flush data to file " << outputfilename 
						<< ": " 
						<< strerror(errno)
						<< std::endl;
					se.finish();
					throw se;
				}
			}

			uint64_t fillBlock(input_type * const A, uint64_t n)
			{
			        input_type * T = A;
			        
			        if ( pc != pe )
			        {
        			        uint64_t const bufcopy = std::min(n,static_cast<uint64_t>(pe-pc));
	        		        std::copy ( pc, pc+bufcopy , T );
		        	        pc += bufcopy;
			                T += bufcopy;
			                n -= bufcopy;
                                }
                                while ( n && totalwords )
                                {
                                        assert ( pc == pe );

        		                while ( (! restwords) && (it != V.end()) )
	        	                        init();

                                        if ( ! restwords )
                                        {
                                                ::libmaus::exception::LibMausException se;
                                                se.getStream() << "Inconsistency: totalwords=" << totalwords << " restwords=" << restwords;
                                                se.finish();
                                                throw se;
                                        }
                                        
                                        uint64_t const bufread = std::min(n,restwords);
                                        reader->read( reinterpret_cast<char *>(T), bufread*sizeof(input_type) );
                                        restwords -= bufread;
                                        totalwords -= bufread;
                                        n -= bufread;
                                        T += bufread;
                                }
                                
                                return (T-A);
			}
		};
	}
}
#endif
