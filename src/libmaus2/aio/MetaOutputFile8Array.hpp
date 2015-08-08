/*
    libmaus2
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
#if ! defined(METAOUTPUTFILE8ARRAY_HPP)
#define METAOUTPUTFILE8ARRAY_HPP

#include <libmaus2/aio/OutputBuffer8.hpp>
#include <libmaus2/aio/GenericInput.hpp>
#include <libmaus2/util/IntervalTree.hpp>
#include <libmaus2/aio/InputStreamFactoryContainer.hpp>
#include <vector>
#include <set>
#include <deque>

namespace libmaus2
{
	namespace aio
	{
		/**
		 * class implementing array of MetaOutputBuffer8 objects
		 **/
		struct MetaOutputFile8Array
		{
			//! buffer type
			typedef libmaus2::aio::MetaOutputBuffer8 buffer_type;
			//! buffer pointer type
			typedef ::libmaus2::util::unique_ptr<buffer_type>::type buffer_ptr_type;
			//! this type
			typedef MetaOutputFile8Array this_type;
			//! unique pointer type
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			//! hash intervals
			std::vector< std::pair<uint64_t,uint64_t> > HI;
			//! output file prefix
			std::string const fileprefix;

			//! output buffers
			::libmaus2::autoarray::AutoArray<buffer_ptr_type> buffers;
			//! output filenames for splitting
			std::vector < std::string > filenames;

			//! concatenated file name
			std::string const concfilename;
			//! writer for concatenated file
			::libmaus2::aio::AsynchronousWriter W;

			//! interval tree for hash intervals
			::libmaus2::util::IntervalTree IT;

			private:
			/**
			 * todo triple for list merging
			 **/
			struct TodoTriple
			{
				//! file name
				std::string filename;
				//! id mask
				uint64_t mask;
				//! contained ids
				std::set < uint64_t > ids;

				/**
				 * constructor
				 **/
				TodoTriple() : mask(0) {}
				/**
				 * constructor by parameters
				 * 
				 * @param rfilename file name
				 * @param rmask id mask
				 * @param rids contained id set
				 **/
				TodoTriple(std::string const & rfilename, uint64_t const rmask, std::set < uint64_t > rids )
				: filename(rfilename), mask(rmask), ids(rids) {}
			};

			public:
			/**
			 * construct from hash intervals and file prefix
			 *
			 * @param rHI hash intervals
			 * @param rfileprefix prefix for output file names
			 **/
			MetaOutputFile8Array(
				std::vector< std::pair<uint64_t,uint64_t> > const & rHI, std::string const rfileprefix
			)
			: HI(rHI), fileprefix(rfileprefix), buffers(HI.size()), concfilename ( fileprefix + ".conc" ), 
			  W(concfilename,16), IT(HI,0,HI.size())
			{
				for ( uint64_t i = 0; i < HI.size(); ++i )
				{
					std::ostringstream ostr;
					ostr << fileprefix << "_" << i;
					filenames.push_back(ostr.str());
				}
				for ( uint64_t i = 0; i < HI.size(); ++i )
				{
					buffers[i] = buffer_ptr_type(new buffer_type(W,64*1024,i));
				}
			}
			/**
			 * destructor
			 **/
			~MetaOutputFile8Array()
			{
			}

			/**
			 * count number of different block meta ids in file filename
			 *
			 * @param filename name of input file
			 **/
			static std::set < uint64_t > countIds(std::string const & filename)
			{
				GenericInput<uint64_t> in(filename, 4*1024);

				std::set < uint64_t > ids;
				uint64_t idword;

				while ( in.getNext(idword) )
				{
					ids.insert ( idword );

					uint64_t numwords;
					bool const gotwords = in.getNext(numwords);
					assert ( gotwords );

					for ( uint64_t i = 0; i < numwords; ++i )
					{
						uint64_t dataword;
						bool const gotdata = in.getNext(dataword);
						assert ( gotdata );
					}
				}

				return ids;
			}

			/**
			 * get name of next temp file
			 *
			 * @param nextid current id (will be incremented by call)
			 * @return temporary filename
			 **/
			std::string getNextTempName(uint64_t & nextid)
			{
				std::ostringstream ostr;
				ostr << fileprefix << "_temp_" << (nextid++);
				return ostr.str();
			}

			/**
			 * check if file filename exists
			 *
			 * @param filename name of file
			 * @return true iff file exists
			 **/
			static bool fileExists(std::string const & filename)
			{
				return libmaus2::aio::InputStreamFactoryContainer::tryOpen(filename);
			}

			/**
			 * flush output streams and split final output according to hash values/intervals
			 * 
			 * @param cerrlock lock for debug/verbosity output
			 * @param threadnum thread id for debug/verbosity output
			 **/
			void flush(::libmaus2::parallel::OMPLock & cerrlock, uint64_t const threadnum)
			{
				if ( fileExists(concfilename) )
				{
					// flush all buffers
					for ( uint64_t i = 0; i < buffers.getN(); ++i )
						if ( buffers[i].get() )
						{
							buffers[i]->flush();
							buffers[i].reset();
						}
					W.flush();
			
					unsigned int const maskwidth = 2;
					uint64_t const primelen = 1ull << maskwidth;
					uint64_t const primemask = (1ull<<maskwidth)-1;

					std::deque < TodoTriple > todo;
					todo.push_back ( TodoTriple(concfilename,0,countIds(concfilename)) );
					uint64_t tmpid = 0;
			
					while ( todo.size() )
					{
						std::string const infilename = todo.front().filename;
						uint64_t const maskshift = todo.front().mask;
						std::set<uint64_t> ids = todo.front().ids;
						todo.pop_front();
			
						cerrlock.lock();
						std::cerr << "File " << infilename << " has " << ids.size() << " ids." << std::endl;
						cerrlock.unlock();	

						if ( ids.size() == 0 )
						{
							libmaus2::aio::FileRemoval::removeFile ( infilename );
						}
						else if ( ids.size() == 1 )
						{
							uint64_t const idword = *(ids.begin());
							assert ( idword < filenames.size() );
							rename ( infilename.c_str(), filenames[idword].c_str() );
						}
						else
						{
							std::vector < std::string > out;
							for ( uint64_t i = 0; i < primelen; ++i )
								out . push_back ( getNextTempName(tmpid) );

							typedef ::libmaus2::aio::OutputBuffer8 split_buffer_type;
							typedef ::libmaus2::util::unique_ptr<split_buffer_type>::type split_buffer_ptr_type;

							::libmaus2::autoarray::AutoArray < split_buffer_ptr_type > AW(primelen);

							for ( uint64_t i = 0; i < primelen; ++i )
								AW[i] = split_buffer_ptr_type ( new split_buffer_type (out[i],4*1024) );

							GenericInput<uint64_t> G(infilename,4*1024);

							std::vector < std::set<uint64_t> > vids(primelen);

							for ( std::set < uint64_t> :: const_iterator ita = ids.begin(); ita != ids.end(); ++ita )
							{
								uint64_t const subid = ((*ita)>>maskshift) & primemask;
								assert ( subid < primelen );
								vids [ subid ] .insert ( *ita );
							}
			
							uint64_t newidword;
			
							while ( G.getNext(newidword) )
							{
								uint64_t const subid = (newidword>>maskshift) & primemask;
				
								bool numok;
								uint64_t num;
								numok = G.getNext(num);
								assert ( numok );
			
								split_buffer_type * w = AW[subid].get();
								::std::set<uint64_t> & tids = vids[subid];
			
								if ( tids.size() > 1 )
								{
									w->put(newidword);
									w->put(num);	
								}
								
								for ( uint64_t j = 0; j < num; ++j )
								{
									bool dataok;
									uint64_t data;
									dataok = G.getNext(data);
									assert ( dataok );
									w->put(data);
								}

								cerrlock.lock();
								std::cerr << "{" << threadnum << "}: handled block of size " <<
									num << " for id " << newidword << " for output " << out[subid] 
									<< " done " << 
									(static_cast<double>(G.totalwordsread) / ( G.totalwords ))<< std::endl;
								cerrlock.unlock();
							}
			
							for ( uint64_t i = 0; i < AW.getN(); ++i )
							{
								AW[i]->flush();
								AW[i].reset();
							}

							for ( uint64_t i = 0; i < primelen; ++i )
								todo.push_back (
									TodoTriple ( out[i], maskshift + maskwidth , vids[i] )
								);	

							libmaus2::aio::FileRemoval::removeFile ( infilename );
						}
					}
				}

				for ( uint64_t i = 0; i < filenames.size(); ++i )
					if ( ! fileExists(filenames[i]) )
					{
						libmaus2::aio::OutputStreamInstance ostr(filenames[i]);
						ostr.flush();
					}
			}
			
			/**
			 * get buffer for hash value hashval
			 *
			 * @param hashval hash value
			 * @return buffer for hashval
			 **/
			buffer_type & getBuffer(uint64_t const hashval)
			{
				uint64_t const i = IT.find(hashval);
				assert ( i < HI.size() );
				assert ( hashval >= HI[i].first );
				assert ( hashval < HI[i].second );
				return *(buffers[i]);
			}
		};
	}
}
#endif
