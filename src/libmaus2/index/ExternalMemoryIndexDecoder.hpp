/*
    libmaus
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
#if ! defined(LIBMAUS_INDEX_EXTERNALMEMORYINDEXDECODER_HPP)
#define LIBMAUS_INDEX_EXTERNALMEMORYINDEXDECODER_HPP

#include <libmaus/index/ExternalMemoryIndexDecoderFindLargestSmallerResult.hpp>
#include <libmaus/aio/PosixFdInputStream.hpp>
#include <libmaus/index/ExternalMemoryIndexRecord.hpp>

namespace libmaus
{
	namespace index
	{
		template<typename _data_type, unsigned int _base_level_log, unsigned int _inner_level_log, typename _comparator = std::less<_data_type> >
		struct ExternalMemoryIndexDecoder
		{
			typedef _data_type data_type;
			static unsigned int const base_level_log = _base_level_log;
			static unsigned int const inner_level_log = _inner_level_log;
			typedef _comparator comparator;
			typedef ExternalMemoryIndexDecoder<data_type,base_level_log,inner_level_log,comparator> this_type;
			typedef typename libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			static uint64_t const base_index_step = 1ull << base_level_log;
			static uint64_t const inner_index_step = 1ull << inner_level_log;
		
			libmaus::aio::PosixFdInputStream::unique_ptr_type PPFIS;
			std::istream & PFIS;

			std::vector<uint64_t> levelstarts;
			std::vector<uint64_t> levelcnts;
						
			uint64_t const object_size;
			uint64_t const record_size;

			typedef ExternalMemoryIndexRecord<data_type> record_type;
			typedef libmaus::autoarray::AutoArray<record_type> cache_level_type;
			typedef typename cache_level_type::shared_ptr_type cache_level_ptr_type;
			typedef libmaus::autoarray::AutoArray< cache_level_ptr_type > cache_type;
			cache_type cache;
			
			data_type minel;
			bool minelvalid;
			std::pair<uint64_t,uint64_t> minelpos;
			data_type maxel;
			bool maxelvalid;
			std::pair<uint64_t,uint64_t> maxelpos;
			
			/**
			 * set up decoder
			 *
			 * @param cache_thres combined size of caches in bytes
			 **/
			void setup(uint64_t const cache_thres = 2048)
			{
				uint64_t const endofindex = libmaus::util::NumberSerialisation::deserialiseNumber(PFIS);				
				PFIS.seekg(endofindex,std::ios::beg);
				PFIS.seekg(-8,std::ios::cur);
				uint64_t const numlevels = libmaus::util::NumberSerialisation::deserialiseNumber(PFIS);
				PFIS.seekg(- static_cast<int64_t>(sizeof(uint64_t) + numlevels * 2 * sizeof(uint64_t)), std::ios::cur);
				for ( uint64_t i = 0; i < numlevels; ++i )
				{
					levelstarts.push_back(libmaus::util::NumberSerialisation::deserialiseNumber(PFIS));
					levelcnts.push_back(libmaus::util::NumberSerialisation::deserialiseNumber(PFIS));
					#if 0
					std::cerr << "levelcnts[" << i << "]=" << levelcnts[i] << " levelstarts[" << i << "]=" << levelstarts[i] << std::endl;
					#endif
				}
				if ( numlevels )
				{
					#if 0
					if ( levelcnts.size() == 1 )
						assert ( levelcnts.back() <= base_index_step );
					else
					#endif
						assert ( levelcnts.back() <= inner_index_step );
				}

				cache = cache_type(numlevels);
				uint64_t cachesum = 0;
				unsigned int numcache = 0;
				for ( uint64_t ii = 0; ii < numlevels; ++ii )
				{
					uint64_t const i = numlevels-ii-1;
					
					if ( (cachesum + levelcnts[i])*record_size <= cache_thres )
					{
						cache_level_ptr_type Pcachelevel(new cache_level_type(levelcnts[i]));
						cache_level_type & cachelevel = *Pcachelevel;
						PFIS.clear();
						PFIS.seekg(levelstarts[i], std::ios::beg);
						for ( uint64_t j = 0; j < levelcnts[i]; ++j )
							cachelevel[j].deserialise(PFIS);
						cache[i] = Pcachelevel;	
						cachesum += levelcnts[i];
						numcache += 1;
					}
				
					#if 0
					std::cerr << "levelcnts[" << i << "]=" << levelcnts[i] << " cachesum=" << cachesum 
						<< " total=" << cachesum * record_size
						<< std::endl;
					#endif
				}
				
				// std::cerr << "checking cache...";
				for ( uint64_t ii = 0; ii < numlevels; ++ii )
				{
					uint64_t const i = numlevels-ii-1;
					
					if ( cache[i] )
					{						
						for ( uint64_t j = 0; j < levelcnts[i]; ++j )
						{
							uint64_t jj = j;

							for ( int64_t l = i; l >= 0 && cache[l]; --l, jj <<= inner_level_log )
							{
								record_type const & ref = (*(cache[i]))[j];
								record_type const & check = (*(cache[l]))[jj];
								assert( ref.equal(check) );
							}
						}
					}
				}
				// std::cerr << "done." << std::endl;
			
				// get minimum element (if any)		 
				if ( levelcnts.size() && levelcnts[0] )
				{
				 	PFIS.clear();
				 	PFIS.seekg(levelstarts[0], std::ios::beg);
				 	minelpos.first  = libmaus::util::NumberSerialisation::deserialiseNumber(PFIS);
				 	minelpos.second = libmaus::util::NumberSerialisation::deserialiseNumber(PFIS);
					minel.deserialise(PFIS);	
					minelvalid = true;

					PFIS.clear();
					PFIS.seekg(levelstarts[0] + (levelcnts[0]-1) * record_size, std::ios::beg);
				 	maxelpos.first  = libmaus::util::NumberSerialisation::deserialiseNumber(PFIS);
				 	maxelpos.second = libmaus::util::NumberSerialisation::deserialiseNumber(PFIS);
					maxel.deserialise(PFIS);					
					maxelvalid = true;
				}
				else
				{
					minelvalid = false;
					maxelvalid = false;
				}
			}

			ExternalMemoryIndexDecoder(std::string const & filename, uint64_t const rcache_thres = 2048) 
			: PPFIS(new libmaus::aio::PosixFdInputStream(filename)), PFIS(*PPFIS), object_size(data_type::getSerialisedObjectSize()), record_size(2*sizeof(uint64_t)+object_size)
			{
				setup(rcache_thres);
			}

			ExternalMemoryIndexDecoder(std::istream & rstream, uint64_t const rcache_thres = 2048) 
			: PPFIS(), PFIS(rstream), object_size(data_type::getSerialisedObjectSize()), record_size(2*sizeof(uint64_t)+object_size)
			{
				setup(rcache_thres);
			}

			/**
			 * get offset for element i at base layer of index			
			 *
			 * @param i base block index
			 * @return offset in data file (start of compressed block and offset inside compressed block)
			 **/
			std::pair<uint64_t,uint64_t> operator[](uint64_t const i)
			{
				assert ( i < levelcnts[0] );
			
				PFIS.clear();
				PFIS.seekg(levelstarts[0] + i * record_size);

				uint64_t const pfirst = libmaus::util::NumberSerialisation::deserialiseNumber(PFIS);
				uint64_t const psecond = libmaus::util::NumberSerialisation::deserialiseNumber(PFIS);
				data_type Q;
				Q.deserialise(PFIS);

				std::pair<uint64_t,uint64_t> const P(pfirst,psecond);
				
				if ( cache[0] )
				{
					assert ( (*(cache[0]))[i].P == P );
					assert ( (*(cache[0]))[i].D == Q );
				}
				
				return P;
			}
			
			data_type getBaseLevelBlockStart(uint64_t const i)
			{
				assert ( i < levelcnts[0] );
				
				PFIS.clear();
				PFIS.seekg(levelstarts[0] + i * record_size);

				uint64_t const pfirst = libmaus::util::NumberSerialisation::deserialiseNumber(PFIS);
				uint64_t const psecond = libmaus::util::NumberSerialisation::deserialiseNumber(PFIS);
				data_type Q;
				Q.deserialise(PFIS);

				std::pair<uint64_t,uint64_t> const P(pfirst,psecond);

				if ( cache[0] )
				{
					assert ( (*(cache[0]))[i].P == P );
					assert ( (*(cache[0]))[i].D == Q );
				}
			
				return Q;
			}

			data_type getLevelBlockStart(unsigned int const level, uint64_t const i)
			{
				assert ( i < levelcnts[level] );
				
				PFIS.clear();
				PFIS.seekg(levelstarts[level] + i * record_size);

				uint64_t const pfirst = libmaus::util::NumberSerialisation::deserialiseNumber(PFIS);
				uint64_t const psecond = libmaus::util::NumberSerialisation::deserialiseNumber(PFIS);
				data_type Q;
				Q.deserialise(PFIS);

				std::pair<uint64_t,uint64_t> const P(pfirst,psecond);

				if ( cache[level] )
				{
					assert ( (*(cache[level]))[i].P == P );
					assert ( (*(cache[level]))[i].D == Q );
				}
				
			
				return Q;
			}
			
			/**
			 * return number of base blocks in index
			 **/
			uint64_t size() const
			{
				return levelcnts.size() ? levelcnts.at(0) : 0;
			}
			
			std::ostream & printLevel(std::ostream & out, int const l)
			{
				PFIS.seekg(levelstarts[l], std::ios::beg);
				for ( uint64_t i = 0; i < levelcnts[l]; ++i )
				{
					uint64_t const pfirst = libmaus::util::NumberSerialisation::deserialiseNumber(PFIS);
					uint64_t const psecond = libmaus::util::NumberSerialisation::deserialiseNumber(PFIS);
					data_type Q;
					Q.deserialise(PFIS);
				
					out << "E(" <<l <<"," << i <<")=([" << pfirst << "," << psecond << "," << Q << ")\n";
				}
				
				return out;

			}

			std::ostream & print(std::ostream & out)
			{
				for ( int i = static_cast<int>(levelstarts.size())-1; i >= 0; --i )
					printLevel(out,i);
				return out;
			}
			
			/**
			 * find largest element smaller than E in index
			 **/
			ExternalMemoryIndexDecoderFindLargestSmallerResult<data_type> findLargestSmaller(
				data_type const & E, 
				bool const cacheOnly = false,
				comparator comp = comparator()
			)
			{
				bool const debug = true;
				
				#if 0
				std::cerr << "checking for " << E << std::endl;
				std::cerr << "minimum element valid " << minelvalid << std::endl;
				std::cerr << "minimum element " << minel << std::endl;
				#endif
				
				if ( ! minelvalid )
					return ExternalMemoryIndexDecoderFindLargestSmallerResult<data_type>();
			
				// check for empty array or minimum too large
				if ( !comp(minel,E) )
					return ExternalMemoryIndexDecoderFindLargestSmallerResult<data_type>(minelpos,0,minel);
			
				// block id
				uint64_t blockid = 0;
				// element
				data_type PE;
				// offset of PE in compressed data file
				std::pair<uint64_t,uint64_t> P;
				
				for ( int level = static_cast<int>(levelstarts.size())-1; level >= 0; --level )
				{
					uint64_t const index_step = (1ull << inner_level_log);
					
					// if level is cached then use binary search
					if ( cache[level] )
					{
						// select block
						cache_level_type const & cachelevel = *(cache[level]);
						uint64_t const scanstart = blockid * index_step;
						typename cache_level_type::const_iterator ita = cachelevel.begin();
						typename cache_level_type::const_iterator it_start = ita + scanstart;
						typename cache_level_type::const_iterator it_end   = ita + std::min(scanstart+index_step,levelcnts[level]);
						
						// binary search
						while ( it_end-it_start > 1 )
						{
							typename cache_level_type::const_iterator it_mid = it_start + ((it_end-it_start)>>1);
							
							if ( comp(it_mid->D,E) )
								it_start = it_mid;
							else
								it_end = it_mid;
						}

						assert ( it_end-it_start == 1 );

						if ( debug )
						{
							typename cache_level_type::const_iterator itz = ita + scanstart;
							
							while ( 
								itz   < it_end &&
								itz+1 < it_end &&
								comp((itz+1)->D,E)
							)
							{
								++itz;
							}
							
							assert ( itz == it_start );
								
						}
					
						blockid = (it_start-ita);
						P  = it_start->P;
						PE = it_start->D;
					}
					// use only cache
					else if ( cacheOnly )
					{
						blockid *= index_step;
					}
					// otherwise scan block in file
					else
					{
						PFIS.clear();

						uint64_t const scanstart = blockid * index_step;
						PFIS.seekg(levelstarts[level] + scanstart * record_size, std::ios::beg);
						
						for ( uint64_t j = scanstart; j < levelcnts[level]; ++j )
						{
							uint64_t const pfirst = libmaus::util::NumberSerialisation::deserialiseNumber(PFIS);
							uint64_t const psecond = libmaus::util::NumberSerialisation::deserialiseNumber(PFIS);
							data_type Q;
							Q.deserialise(PFIS);
							
							if ( comp(Q,E) ) // Q<E -> still ok
							{
								blockid = j;
								P.first = pfirst;
								P.second = psecond;
								PE = Q;
							}
							else // Q >= E -> Q no longer smaller than E -> break
							{
								break;
							}
						}
					}
				}
				
				#if 0
				std::cerr << "blockid=" << blockid << std::endl;
				#endif
				
				return ExternalMemoryIndexDecoderFindLargestSmallerResult<data_type>(P,blockid,PE);
			}
		};
	}
}
#endif
