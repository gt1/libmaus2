/**
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
**/


#if ! defined(METAOUTPUTFILE8ARRAY_HPP)
#define METAOUTPUTFILE8ARRAY_HPP

#include <libmaus/aio/OutputBuffer8.hpp>
#include <libmaus/aio/GenericInput.hpp>
#include <libmaus/util/IntervalTree.hpp>
#include <vector>
#include <set>
#include <deque>

namespace libmaus
{
	namespace aio
	{
		struct MetaOutputFile8Array
		{
			typedef libmaus::aio::MetaOutputBuffer8 buffer_type;
			typedef ::libmaus::util::unique_ptr<buffer_type>::type buffer_ptr_type;
			typedef MetaOutputFile8Array this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			std::vector< std::pair<uint64_t,uint64_t> > HI;
			std::string const fileprefix;

			::libmaus::autoarray::AutoArray<buffer_ptr_type> buffers;
			std::vector < std::string > filenames;

			std::string const concfilename;
			::libmaus::aio::AsynchronousWriter W;

			::libmaus::util::IntervalTree IT;

			struct TodoTriple
			{
				std::string filename;
				uint64_t mask;
				std::set < uint64_t > ids;

				TodoTriple() : mask(0) {}
				TodoTriple(std::string const & rfilename, uint64_t const rmask, std::set < uint64_t > rids )
				: filename(rfilename), mask(rmask), ids(rids) {}
			};

			MetaOutputFile8Array(std::vector< std::pair<uint64_t,uint64_t> > const & rHI, std::string const rfileprefix)
			: HI(rHI), fileprefix(rfileprefix), buffers(HI.size()), concfilename ( fileprefix + ".conc" ), W(concfilename,16),
			  IT(HI,0,HI.size())
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
			~MetaOutputFile8Array()
			{
			}

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

			std::string getNextTempName(uint64_t & nextid)
			{
				std::ostringstream ostr;
				ostr << fileprefix << "_temp_" << (nextid++);
				return ostr.str();
			}

			static bool fileExists(std::string const & filename)
			{
				std::ifstream istr(filename.c_str(), std::ios::binary);
				if ( istr.is_open() )
				{
					istr.close();
					return true;
				}
				else
				{
					return false;
				}
			}

			void flush(::libmaus::parallel::OMPLock & cerrlock, uint64_t const threadnum)
			{
				if ( fileExists(concfilename) )
				{
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
							remove ( infilename.c_str() );
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

							typedef ::libmaus::aio::OutputBuffer8 split_buffer_type;
							typedef ::libmaus::util::unique_ptr<split_buffer_type>::type split_buffer_ptr_type;

							::libmaus::autoarray::AutoArray < split_buffer_ptr_type > AW(primelen);

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

							remove ( infilename.c_str() );
						}
					}
				}

				for ( uint64_t i = 0; i < filenames.size(); ++i )
					if ( ! fileExists(filenames[i]) )
					{
						std::ofstream ostr(filenames[i].c_str(),std::ios::binary);
						ostr.flush();
						ostr.close();
					}
			}
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
