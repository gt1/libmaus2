/*
    libmaus2
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2015 Genome Research Limited

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
#if ! defined(LIBMAUS2_INDEX_EXTERNALMEMORYINDEXGENERATOR_HPP)
#define LIBMAUS2_INDEX_EXTERNALMEMORYINDEXGENERATOR_HPP

#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/util/shared_ptr.hpp>
#include <libmaus2/aio/CheckedInputOutputStream.hpp>
#include <libmaus2/util/NumberSerialisation.hpp>
#include <libmaus2/aio/PosixFdInputStream.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/index/ExternalMemoryIndexRecord.hpp>
#include <iomanip>

namespace libmaus2
{
	namespace index
	{
		template<typename _data_type, unsigned int _base_level_log, unsigned int _inner_level_log>
		struct ExternalMemoryIndexGenerator
		{
			typedef _data_type data_type;
			static unsigned int const base_level_log = _base_level_log;
			static unsigned int const inner_level_log = _inner_level_log;
			typedef ExternalMemoryIndexGenerator<data_type,base_level_log,inner_level_log> this_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			static uint64_t const base_index_step = 1ull << base_level_log;
			static uint64_t const base_index_mask = (base_index_step-1);

			static uint64_t const inner_index_step = 1ull << inner_level_log;
			static uint64_t const inner_index_mask = (inner_index_step-1);
		
			libmaus2::aio::CheckedInputOutputStream::unique_ptr_type Pstream;
			std::iostream & stream;
			uint64_t ic;
			bool flushed;
			
			typedef ExternalMemoryIndexRecord<data_type> record_type;
			libmaus2::autoarray::AutoArray<record_type> writeCache;
			record_type * const wa;
			record_type * wc;
			record_type * const we;
			
			size_t byteSize() const
			{
				return
					sizeof(Pstream) +
					sizeof(ic) +
					sizeof(flushed) +
					writeCache.byteSize() +
					sizeof(wa) +
					sizeof(wc) +
					sizeof(we);
			}
			
			ExternalMemoryIndexGenerator(std::string const & filename)
			: Pstream(new libmaus2::aio::CheckedInputOutputStream(filename)), stream(*Pstream), ic(0), flushed(false), writeCache(1024),
			  wa(writeCache.begin()), wc(wa), we(writeCache.end())
			{
			
			}

			ExternalMemoryIndexGenerator(std::iostream & rstream)
			: Pstream(), stream(rstream), ic(0), flushed(false), writeCache(1024),
			  wa(writeCache.begin()), wc(wa), we(writeCache.end())
			{
			
			}
			
			uint64_t setup()
			{
				uint64_t const curpos = stream.tellp();
				
				flushed = false;
				ic = 0;
				
				// make room for pointer
				libmaus2::util::NumberSerialisation::serialiseNumber(stream,0);
				
				return curpos;
			}
			
			uint64_t flush()
			{
				if ( ! flushed )
				{
					uint64_t const object_size = data_type::getSerialisedObjectSize();
					uint64_t const record_size = 2*sizeof(uint64_t)+object_size;
					
					std::vector<uint64_t> levelstarts;
					std::vector<uint64_t> levelcnts;
					
					unsigned int level = 0;
					uint64_t incnt = ic;
					
					// get position of level 0 records in file
					uint64_t l0pos = static_cast<uint64_t>(stream.tellp()) - (ic * record_size);
					
					// store position and number
					levelstarts.push_back(l0pos);
					levelcnts.push_back(incnt);

					// perform subsampling until we have constructed a single root node
					while ( incnt > inner_index_step )
					{
						uint64_t const outcnt = (incnt + inner_index_step-1)/inner_index_step;
						uint64_t gpos = levelstarts[level];
						uint64_t ppos = gpos + incnt * record_size;
						
						levelstarts.push_back(ppos);
						levelcnts.push_back(outcnt);
						
						stream.seekg(gpos,std::ios::beg);

						data_type D;
						for ( uint64_t j = 0; j < incnt; ++j )
						{
							uint64_t pfirst = libmaus2::util::NumberSerialisation::deserialiseNumber(stream);
							uint64_t psecond = libmaus2::util::NumberSerialisation::deserialiseNumber(stream);							
							D.deserialise(stream);
							
							gpos += 2*sizeof(uint64_t) + object_size;
							
							if ( (j & inner_index_mask) == 0 )
							{
								*(wc++) = record_type(std::pair<uint64_t,uint64_t>(pfirst,psecond),D);
								
								if ( wc == we )
								{
									stream.seekp(ppos,std::ios::beg);
									
									for ( record_type * ww = wa; ww < wc; ++ww )
									{
										ppos += libmaus2::util::NumberSerialisation::serialiseNumber(stream,ww->P.first);
										ppos += libmaus2::util::NumberSerialisation::serialiseNumber(stream,ww->P.second);
										ppos += ww->D.serialise(stream);			
									}
									
									stream.seekg(gpos,std::ios::beg);
									wc = wa;
								}							
							}
						}
						
						if ( wc != wa )
						{
							stream.seekp(ppos,std::ios::beg);
							
							for ( record_type * ww = wa; ww < wc; ++ww )
							{
								ppos += libmaus2::util::NumberSerialisation::serialiseNumber(stream,ww->P.first);
								ppos += libmaus2::util::NumberSerialisation::serialiseNumber(stream,ww->P.second);
								ppos += ww->D.serialise(stream);			
							}
							
							stream.seekg(gpos,std::ios::beg);
							wc = wa;
						}							
						
						incnt = outcnt;
						level += 1;
					}

					// go to end of records for last level
					uint64_t const llpos = stream.tellg();
					uint64_t ppos = llpos + levelcnts.back() * record_size;
					// set put pointer to set position
					stream.seekp(ppos);
					
					// store meta information
					for ( uint64_t i = 0; i < levelcnts.size(); ++i )
					{
						ppos += libmaus2::util::NumberSerialisation::serialiseNumber(stream,levelstarts[i]);
						ppos += libmaus2::util::NumberSerialisation::serialiseNumber(stream,levelcnts[i]);
						#if 0
						std::cerr << "levelcnts[" << i << "]=" << levelcnts[i] << " levelstarts[" << i << "]=" << levelstarts[i] << std::endl;
						#endif
					}

					// number of levels
					ppos += libmaus2::util::NumberSerialisation::serialiseNumber(stream,levelcnts.size());
				
					// end of index pointer	
					uint64_t const backppos = stream.tellp();
					// go to beginning of level 0 records minus 8
					stream.seekp(l0pos-8,std::ios::beg);
					// store back of index pointer
					libmaus2::util::NumberSerialisation::serialiseNumber(stream,backppos);
					// go back to end of index
					stream.seekp(backppos);
					
					stream.flush();
					
					flushed = true;
					
					return backppos;
				}
				else
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "ExternalMemoryIndexGenerator::flush(): generator is already flushed" << std::endl;
					lme.finish();
					throw lme;
				}
			}
			
			void put(data_type const & E, std::pair<uint64_t,uint64_t> const & P)
			{
				libmaus2::util::NumberSerialisation::serialiseNumber(stream,P.first);
				libmaus2::util::NumberSerialisation::serialiseNumber(stream,P.second);
				E.serialise(stream);				
				ic += 1;
			}
		};
	}
}
#endif

