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

#if ! defined(SYNCHRONOUSGENERICINPUT_HPP)
#define SYNCHRONOUSGENERICINPUT_HPP

#include <libmaus/util/GetFileSize.hpp>
#include <cerrno>

namespace libmaus
{
	namespace aio
	{
		/**
		 * synchrnous generic buffered input class
		 **/
		template < typename input_type >
		struct SynchronousGenericInput
		{
			//! input type
			typedef input_type value_type;
			//! input file stream type
			typedef ::std::ifstream ifstream_type;
			//! input file stream pointer type
			typedef ::libmaus::util::unique_ptr<ifstream_type>::type ifstream_ptr_type;
			//! this type
			typedef SynchronousGenericInput<input_type> this_type;
			//! unique pointer type
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
		
			private:
			//! buffer size
			uint64_t const bufsize;
			//! input buffer
			::libmaus::autoarray::AutoArray<input_type> buffer;
			//! input buffer start pointer
			input_type const * const pa;
			//! input buffer current pointer
			input_type const * pc;
			//! input buffer end pointer
			input_type const * pe;

			//! input stream pointer
			ifstream_ptr_type Pistr;
			//! input stream reference
			std::istream & istr;

			//! total words to be read
			uint64_t const totalwords;
			//! total words read yet
			uint64_t totalwordsread;
			
			//! check whether bytes read are a multiple of sizeof(value_type)
			bool checkmod;

			/**
			 * fill the input buffer
			 *
			 * @return true iff any data could be extracted to input buffer
			 **/
			bool fillBuffer()
			{
				assert ( totalwordsread <= totalwords );
				uint64_t const remwords = totalwords-totalwordsread;
				uint64_t const toreadwords = std::min(remwords,bufsize);
				
				istr.read ( reinterpret_cast<char *>(buffer.get()), toreadwords * sizeof(input_type));
				uint64_t const bytesread = istr.gcount();
				
				if ( checkmod && (bytesread % sizeof(input_type) != 0) )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "SynchronousGenericInput::fillBuffer: Number of bytes " << bytesread << " read is not a multiple of entity type." << std::endl;
					se.finish();
					throw se;
				}
				
				uint64_t const wordsread = bytesread / sizeof(input_type);
					
				if ( wordsread == 0 )
				{
					if ( totalwordsread != totalwords )
					{
						std::cerr << "SynchronousGenericInput<>::getNext(): WARNING: read 0 words but there should be " <<
							remwords << " left." << std::endl;
					}
				
					return false;
				}
					
				pc = pa;
				pe = pa + wordsread;
			
				return true;
			}

			public:
			/**
			 * read the contents of file inputfilename to an array with allocation type atype
			 *
			 * @param inputfilename input file name
			 * @return contents of file as array
			 **/
			template< ::libmaus::autoarray::alloc_type atype >
			static ::libmaus::autoarray::AutoArray<input_type,atype> readArrayTemplate(std::string const & inputfilename)
			{
				::libmaus::aio::SynchronousGenericInput<input_type> in(inputfilename,64*1024);
				
				uint64_t const fs =
					::libmaus::util::GetFileSize::getFileSize(inputfilename);
				assert ( fs % sizeof(input_type) == 0 );
				uint64_t const n = fs/sizeof(input_type);

				::libmaus::autoarray::AutoArray<input_type,atype> A(n,false);
			
				for ( uint64_t i = 0; i < n; ++i )
				{
					input_type v;
					bool ok = in.getNext(v);
					assert ( ok );
					A[i] = v;
				}
				
				return A;
			}

			/**
			 * read contents of file inputfilename to an array with default (C++) allocation type
			 *
			 * @param inputfilename input file name
			 * @return contents of file as array
			 **/
			static ::libmaus::autoarray::AutoArray<input_type,::libmaus::autoarray::alloc_type_cxx> readArray(std::string const & inputfilename)
			{
				return readArrayTemplate< ::libmaus::autoarray::alloc_type_cxx>(inputfilename);			
			}

			/**
			 * constructor from istream object
			 *
			 * @param ristr input stream
			 * @param rbufsize input buffer size
			 * @param rtotalwords maximum number of words to read
			 * @param rcheckmod check whether number of bytes read is a multiple of sizeof(value_type)
			 */
			SynchronousGenericInput(std::istream & ristr, uint64_t const rbufsize, 
				uint64_t const rtotalwords = std::numeric_limits<uint64_t>::max(),
				bool const rcheckmod = true
			)
			: bufsize(rbufsize), buffer(bufsize,false), 
			  pa(buffer.get()), pc(pa), pe(pa),
			  Pistr(),
			  istr(ristr),
			  totalwords ( rtotalwords ),
			  totalwordsread(0),
			  checkmod(rcheckmod)
			{
			
			}

			/**
			 * constructor from filename
			 *
			 * @param filename input file name
			 * @param rbufsize input buffer size
			 * @param roffset start read offset
			 * @param rtotalwords maximum number of words to read
			 */
			SynchronousGenericInput(
				std::string const & filename, 
				uint64_t const rbufsize, 
				uint64_t const roffset = 0,
				uint64_t const rtotalwords = std::numeric_limits<uint64_t>::max()
			)
			: bufsize(rbufsize), buffer(bufsize,false), 
			  pa(buffer.get()), pc(pa), pe(pa),
			  Pistr(new ifstream_type(filename.c_str(),std::ios::binary)),
			  istr(*Pistr),
			  totalwords ( std::min ( ::libmaus::util::GetFileSize::getFileSize(filename) / sizeof(input_type) - roffset, rtotalwords) ),
			  totalwordsread(0),
			  checkmod(true)
			{
				if ( ! Pistr->is_open() )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Unable to open file " << filename << ": " << strerror(errno);
					se.finish();
					throw se;
				}
				Pistr->seekg(roffset * sizeof(input_type), std::ios::beg);
				if ( ! istr )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Unable to seek file " << filename << ": " << strerror(errno);
					se.finish();
					throw se;
				}

			}
			/**
			 * destructor
			 **/
			~SynchronousGenericInput()
			{
				if ( Pistr && Pistr->is_open() )
				{
					Pistr->close();
					Pistr.reset();
				}
			}
			
			/**
			 * read next word
			 *
			 * @param word reference to space for next word
			 * @return true iff an input word was placed in word (false for eof)
			 **/
			bool getNext(input_type & word)
			{
				if ( pc == pe )
				{
					if ( ! fillBuffer() )
						return false;
				}

				word = *(pc++);
				totalwordsread++;

				return true;
			}

			/**
			 * read next word but do not remove it from the buffer
			 *
			 * @param word reference to space for next word
			 * @return true iff an input word was placed in word (false for eof)
			 **/
			bool peekNext(input_type & word)
			{
				if ( pc == pe )
				{
					if ( ! fillBuffer() )
						return false;
				}

				word = *pc;

				return true;
			}
			
			/**
			 * read the next word
			 *
			 * @return next word or -1 for end of file
			 **/
			int64_t get()
			{
				input_type word;
				if ( getNext(word) )
					return word;
				else
					return -1;
			}
			
			/**
			 * read the next word but do not remove it from the input buffer
			 *
			 * @return next word or -1 for end of file
			 **/
			int64_t peek()
			{
				input_type word;
				if ( peekNext(word) )
					return word;
				else
					return -1;
			}
		};
        }
}
#endif
