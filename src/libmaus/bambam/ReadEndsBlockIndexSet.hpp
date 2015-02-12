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
#if ! defined(LIBMAUS_BAMBAM_READENDSBLOCKINDEXSET_HPP)
#define LIBMAUS_BAMBAM_READENDSBLOCKINDEXSET_HPP

#include <libmaus/bambam/DupMarkBase.hpp>
#include <libmaus/bambam/DupSetCallbackSharedVector.hpp>
#include <libmaus/bambam/ReadEndsHeapPairComparator.hpp>
#include <libmaus/lz/SnappyInputStream.hpp>
#include <libmaus/bambam/ReadEndsBlockDecoderBaseCollectionInfoBase.hpp>
#include <libmaus/index/ExternalMemoryIndexDecoder.hpp>
#include <libmaus/bambam/ReadEndsContainerBase.hpp>
#include <libmaus/aio/InputStreamFactoryContainer.hpp>
#include <libmaus/bambam/DupSetCallback.hpp>

namespace libmaus
{
	namespace bambam
	{
		struct ReadEndsBlockIndexSet
		{
			typedef ReadEndsBlockIndexSet this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			typedef libmaus::index::ExternalMemoryIndexDecoder<
				libmaus::bambam::ReadEndsBase,
				libmaus::bambam::ReadEndsContainerBase::baseIndexShift,
				libmaus::bambam::ReadEndsContainerBase::innerIndexShift
			> index_decoder_type;
			typedef index_decoder_type::unique_ptr_type index_decoder_pointer_type;

			static uint64_t computeNumBlocks(std::vector<ReadEndsBlockDecoderBaseCollectionInfoBase> const & rinfo)
			{
				uint64_t numblocks = 0;
				for ( uint64_t i = 0; i < rinfo.size(); ++i )
					numblocks += rinfo[i].indexoffset.size();
				return numblocks;
			}
			
			std::vector<ReadEndsBlockDecoderBaseCollectionInfoBase> const info;
			uint64_t const numblocks;
			std::vector<uint64_t> O;
			libmaus::autoarray::AutoArray<libmaus::aio::InputStream::unique_ptr_type> indexstreams;
			libmaus::autoarray::AutoArray<index_decoder_pointer_type> indexes;
			
			ReadEndsBlockIndexSet(std::vector<ReadEndsBlockDecoderBaseCollectionInfoBase> const & rinfo)
			: info(rinfo), numblocks(computeNumBlocks(rinfo)), O(rinfo.size()), indexstreams(rinfo.size()), indexes(numblocks)
			{
				for ( uint64_t i = 1; i < O.size(); ++i )
					O[i] = O[i-1] + rinfo[i-1].indexoffset.size();
				
				for ( uint64_t i = 0; i < rinfo.size(); ++i )
				{
					libmaus::aio::InputStream::unique_ptr_type tptr(
						libmaus::aio::InputStreamFactoryContainer::constructUnique(rinfo[i].indexfilename)
					);
					indexstreams[i] = UNIQUE_PTR_MOVE(tptr);
					
					for ( uint64_t j = 0; j < rinfo[i].indexoffset.size(); ++j )
					{
						indexstreams[i]->clear();
						indexstreams[i]->seekg(rinfo[i].indexoffset[j]);
						index_decoder_pointer_type iptr(
							new index_decoder_type(*(indexstreams[i]))
						);
						indexes[O[i] + j] = UNIQUE_PTR_MOVE(iptr);
					}
				}
			}
			
			uint64_t size() const
			{
				return numblocks;
			}

			index_decoder_type & operator[](uint64_t i)
			{
				return *(indexes[i]);
			}
			
			std::pair<uint64_t,uint64_t> getOffset(uint64_t const block, uint64_t const subblock)
			{
				return (*this)[block][subblock];
			}
			
			static uint64_t getBaseBlockSize()
			{
				return libmaus::bambam::ReadEndsContainerBase::baseIndexStep;
			}
			
			std::pair<uint64_t,uint64_t> merge(
				std::vector< std::pair<uint64_t,uint64_t> > V,
				bool (*isDup)(::libmaus::bambam::ReadEndsBase const &, ::libmaus::bambam::ReadEndsBase const &),
				uint64_t (*markDuplicate)(std::vector< ::libmaus::bambam::ReadEnds > const & lfrags, ::libmaus::bambam::DupSetCallback & DSC),
				::libmaus::bambam::DupSetCallback & DSC
			)
			{
				uint64_t exp = 0;
				for ( uint64_t i = 0; i < V.size(); ++i )
					exp += V[i].second-V[i].first;
			
				libmaus::autoarray::AutoArray<libmaus::aio::InputStream::unique_ptr_type> datastreams(info.size());
				libmaus::autoarray::AutoArray<libmaus::lz::SnappyInputStream::unique_ptr_type> zdatastreams(size());
				libmaus::bambam::ReadEnds R;
				
				for ( uint64_t i = 0; i < info.size(); ++i )
				{				
					// open data stream
					libmaus::aio::InputStream::unique_ptr_type tptr(
						libmaus::aio::InputStreamFactoryContainer::constructUnique(info[i].datafilename)
					);
					datastreams[i] = UNIQUE_PTR_MOVE(tptr);
					
					// set up uncompressors
					for ( uint64_t j = 0; j < info[i].indexoffset.size(); ++j )
					{
						uint64_t const blockid    = O[i] + j;
						uint64_t const subblockid = V[blockid].first / getBaseBlockSize();
						std::pair<uint64_t,uint64_t> const zoffset = getOffset(blockid,subblockid);
						
						libmaus::lz::SnappyInputStream::unique_ptr_type zptr(
							new libmaus::lz::SnappyInputStream(
								*(datastreams[i]),
								zoffset.first,
								true /* set pos */
							)
						);
						zptr->ignore(zoffset.second);
						
						uint64_t const rskip = V[blockid].first - subblockid * getBaseBlockSize();
						for ( uint64_t k = 0; k < rskip; ++k )
							R.get(*zptr);
							
						zdatastreams[blockid] = UNIQUE_PTR_MOVE(zptr);
					}
				}			

				//! pair of list index and ReadEnds object
				typedef std::pair<uint64_t,::libmaus::bambam::ReadEnds> qtype;
				//! merge heap
				std::priority_queue<qtype,std::vector<qtype>,::libmaus::bambam::ReadEndsHeapPairComparator> Q;
				
				for ( uint64_t i = 0; i < V.size(); ++i )
					if ( V[i].first < V[i].second )
					{
						R.get(*(zdatastreams[i]));
						Q.push(qtype(i,R));
						V[i].first++;
					}
				
				std::vector< ::libmaus::bambam::ReadEnds > RV;
				
				uint64_t dupcnt = 0;
				uint64_t rcnt = 0;
				bool prevvalid = false;
				::libmaus::bambam::ReadEnds prev;
				
				while ( Q.size() )
				{
					qtype const P = Q.top();
					Q.pop();
					
					rcnt += 1;
					
					if ( prevvalid )
					{
						assert ( prev < P.second );
					}
					prevvalid = true;
					prev = P.second;
					
					if ( RV.size() && ! isDup(RV.back(),P.second) )
					{
						dupcnt += markDuplicate(RV,DSC);
						RV.resize(0);
					}
					RV.push_back(P.second);
					
					// P.second.put(SOS);

					if ( V[P.first].first < V[P.first].second )
					{
						R.get(*(zdatastreams[P.first]));
						Q.push(qtype(P.first,R));
						V[P.first].first++;
					}
				}

				dupcnt += markDuplicate(RV,DSC);
				RV.resize(0);
				
				assert ( exp == rcnt );
				
				return std::pair<uint64_t,uint64_t>(rcnt,dupcnt);
			}

			std::pair<uint64_t,uint64_t> merge(
				std::vector< std::pair<uint64_t,uint64_t> > V,
				bool (*isDup)(::libmaus::bambam::ReadEndsBase const &, ::libmaus::bambam::ReadEndsBase const &),
				uint64_t (*markDuplicate)(std::vector< ::libmaus::bambam::ReadEnds > & lfrags, ::libmaus::bambam::DupSetCallback & DSC),
				::libmaus::bambam::DupSetCallback & DSC
			)
			{
				uint64_t exp = 0;
				for ( uint64_t i = 0; i < V.size(); ++i )
					exp += V[i].second-V[i].first;
			
				libmaus::autoarray::AutoArray<libmaus::aio::InputStream::unique_ptr_type> datastreams(info.size());
				libmaus::autoarray::AutoArray<libmaus::lz::SnappyInputStream::unique_ptr_type> zdatastreams(size());
				libmaus::bambam::ReadEnds R;
				
				for ( uint64_t i = 0; i < info.size(); ++i )
				{				
					// open data stream
					libmaus::aio::InputStream::unique_ptr_type tptr(
						libmaus::aio::InputStreamFactoryContainer::constructUnique(info[i].datafilename)
					);
					datastreams[i] = UNIQUE_PTR_MOVE(tptr);
					
					// set up uncompressors
					for ( uint64_t j = 0; j < info[i].indexoffset.size(); ++j )
					{
						uint64_t const blockid    = O[i] + j;
						uint64_t const subblockid = V[blockid].first / getBaseBlockSize();
						std::pair<uint64_t,uint64_t> const zoffset = getOffset(blockid,subblockid);
						
						libmaus::lz::SnappyInputStream::unique_ptr_type zptr(
							new libmaus::lz::SnappyInputStream(
								*(datastreams[i]),
								zoffset.first,
								true /* set pos */
							)
						);
						zptr->ignore(zoffset.second);
						
						uint64_t const rskip = V[blockid].first - subblockid * getBaseBlockSize();
						for ( uint64_t k = 0; k < rskip; ++k )
							R.get(*zptr);
							
						zdatastreams[blockid] = UNIQUE_PTR_MOVE(zptr);
					}
				}			

				//! pair of list index and ReadEnds object
				typedef std::pair<uint64_t,::libmaus::bambam::ReadEnds> qtype;
				//! merge heap
				std::priority_queue<qtype,std::vector<qtype>,::libmaus::bambam::ReadEndsHeapPairComparator> Q;
				
				for ( uint64_t i = 0; i < V.size(); ++i )
					if ( V[i].first < V[i].second )
					{
						R.get(*(zdatastreams[i]));
						Q.push(qtype(i,R));
						V[i].first++;
					}
				
				std::vector< ::libmaus::bambam::ReadEnds > RV;
				
				uint64_t dupcnt = 0;
				uint64_t rcnt = 0;
				bool prevvalid = false;
				::libmaus::bambam::ReadEnds prev;
				
				while ( Q.size() )
				{
					qtype const P = Q.top();
					Q.pop();

					if ( prevvalid )
					{
						assert ( prev < P.second );
					}
					prevvalid = true;
					prev = P.second;
					
					rcnt += 1;
										
					if ( RV.size() && ! isDup(RV.back(),P.second) )
					{
						#if 0
						if ( RV.size() > 1 )
						{
							std::cerr << std::string(80,'-') << std::endl;
							for ( uint64_t i = 0; i < RV.size(); ++i )
								std::cerr << RV[i] << std::endl;
						}
						#endif

						dupcnt += markDuplicate(RV,DSC);
					
						RV.resize(0);
					}
					RV.push_back(P.second);
					
					// P.second.put(SOS);

					if ( V[P.first].first < V[P.first].second )
					{
						R.get(*(zdatastreams[P.first]));
						Q.push(qtype(P.first,R));
						V[P.first].first++;
					}
				}

				#if 0
				if ( RV.size() > 1 )
				{
					std::cerr << std::string(80,'-') << std::endl;
					for ( uint64_t i = 0; i < RV.size(); ++i )
						std::cerr << RV[i] << std::endl;
				}
				#endif
				dupcnt += markDuplicate(RV,DSC);
				
				assert ( exp == rcnt );
								
				return std::pair<uint64_t,uint64_t>(rcnt,dupcnt);
			}
		};
	}
}
#endif
