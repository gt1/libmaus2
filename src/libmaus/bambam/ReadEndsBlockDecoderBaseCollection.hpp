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
#if ! defined(LIBMAUS_BAMBAM_READENDSBLOCKDECODERBASECOLLECTION_HPP)
#define LIBMAUS_BAMBAM_READENDSBLOCKDECODERBASECOLLECTION_HPP

#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/bambam/BamAlignment.hpp>
#include <libmaus/bambam/CompactReadEndsBase.hpp>
#include <libmaus/bambam/CompactReadEndsComparator.hpp>
#include <libmaus/bambam/ReadEnds.hpp>
#include <libmaus/bambam/ReadEndsBlockDecoderBaseCollectionInfo.hpp>
#include <libmaus/bambam/ReadEndsContainerBase.hpp>
#include <libmaus/bambam/SortedFragDecoder.hpp>
#include <libmaus/util/DigitTable.hpp>
#include <libmaus/util/CountGetObject.hpp>
#include <libmaus/sorting/SerialRadixSort64.hpp>
#include <libmaus/lz/SnappyOutputStream.hpp>

#include <libmaus/bambam/ReadEndsBlockDecoderBase.hpp>
#include <libmaus/util/UnsignedIntegerIndexIterator.hpp>
#include <libmaus/bambam/ReadEndsHeapPairComparator.hpp>

#include <libmaus/timing/RealTimeClock.hpp>

namespace libmaus
{
	namespace bambam
	{
		template<bool _proxy>
		struct ReadEndsBlockDecoderBaseCollection
		{
			static bool const proxy = _proxy;
			typedef ReadEndsBlockDecoderBaseCollection<proxy> this_type;
			typedef typename libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

			struct ReadEndsBlockDecoderBaseCollectionCountSmallerShortAccessor
			{
				ReadEndsBlockDecoderBaseCollection<proxy> const * object;
				
				ReadEndsBlockDecoderBaseCollectionCountSmallerShortAccessor(ReadEndsBlockDecoderBaseCollection<proxy> * robject = 0) : object(robject) {}
				
				uint64_t get(libmaus::bambam::ReadEndsBase::hash_value_type const & H) const
				{
					return object->countSmallerThanShort(H);
				}
			};

			struct ReadEndsBlockDecoderBaseCollectionCountSmallerLongAccessor
			{
				ReadEndsBlockDecoderBaseCollection<proxy> const * object;
				
				ReadEndsBlockDecoderBaseCollectionCountSmallerLongAccessor(ReadEndsBlockDecoderBaseCollection<proxy> * robject = 0) : object(robject) {}
				
				uint64_t get(libmaus::bambam::ReadEndsBase::hash_value_type const & H) const
				{
					return object->countSmallerThanLong(H);
				}
			};
			
			std::vector<ReadEndsBlockDecoderBaseCollectionInfo> info;
			uint64_t const numblocks;
			
			libmaus::autoarray::AutoArray< typename ::libmaus::bambam::ReadEndsBlockDecoderBase<proxy>::unique_ptr_type> Adecoders;
			
			ReadEndsBlockDecoderBaseCollectionCountSmallerShortAccessor const countShortAccessor;
			ReadEndsBlockDecoderBaseCollectionCountSmallerLongAccessor const countLongAccessor;
			
			static std::vector<ReadEndsBlockDecoderBaseCollectionInfo> constructInfo(
				std::vector<ReadEndsBlockDecoderBaseCollectionInfoBase> const & rinfo, bool const parallelSetup
			)
			{
				std::vector<ReadEndsBlockDecoderBaseCollectionInfo> info(rinfo.size());
			
				if ( parallelSetup )
				{
					#if defined(_OPENMP)
					#pragma omp parallel for
					#endif
					for ( uint64_t i = 0; i < rinfo.size(); ++i )
						info[i] = ReadEndsBlockDecoderBaseCollectionInfo(rinfo[i]);					
				}
				else
				{
					for ( uint64_t i = 0; i < rinfo.size(); ++i )
						info[i] = ReadEndsBlockDecoderBaseCollectionInfo(rinfo[i]);				
				}

				return info;
			}
			
			ReadEndsBlockDecoderBaseCollection(
				std::vector<ReadEndsBlockDecoderBaseCollectionInfoBase> const & rinfo,
				bool const parallelSetup = false
			) : info(constructInfo(rinfo,parallelSetup)),
			    numblocks(computeNumBlocks()),
			    Adecoders(numblocks,false),
			    countShortAccessor(this),
			    countLongAccessor(this)
			{
				uint64_t j = 0;
				for ( uint64_t k = 0; k < info.size(); ++k )
				{
					ReadEndsBlockDecoderBaseCollectionInfo & subinfo = info[k];
					
					for ( uint64_t i = 0; i < subinfo.indexoffset.size(); ++i )
					{
						typename libmaus::bambam::ReadEndsBlockDecoderBase<proxy>::unique_ptr_type tptr(
							new libmaus::bambam::ReadEndsBlockDecoderBase<proxy>(
								subinfo,
								subinfo,
								*(subinfo.datastr),
								*(subinfo.indexstr),
								subinfo.indexoffset.at(i),
								subinfo.blockelcnt.at(i)
							)
						);
						
						Adecoders.at(j++) = UNIQUE_PTR_MOVE(tptr);
					}
				}
			}

			uint64_t computeNumBlocks() const
			{
				uint64_t s = 0;
				for ( uint64_t i = 0; i < info.size(); ++i )
					s += info[i].indexoffset.size();
				return s;
			}
			
			uint64_t size() const
			{
				return numblocks;
			}
			
			::libmaus::bambam::ReadEndsBlockDecoderBase<proxy> const * getBlock(uint64_t const i) const
			{
				return Adecoders[i].get();
			}
			
			uint64_t totalEntries() const
			{
				uint64_t sum = 0;
				for ( uint64_t i = 0; i < size(); ++i )
					sum += getBlock(i)->size();
				return sum;
			}
			
			ReadEnds max() const
			{
				ReadEnds RE;
				for ( uint64_t i = 0; i < size(); ++i )
					if ( getBlock(i) && getBlock(i)->size() )
						RE = std::max(RE,getBlock(i)->get(getBlock(i)->size()-1));
				return RE;
			}

			uint64_t countSmallerThanShort(ReadEndsBase const & A) const
			{
				uint64_t sum = 0;
				for ( uint64_t i = 0; i < size(); ++i )
					sum += getBlock(i)->countSmallerThanShort(A);
				return sum;
			}

			uint64_t countSmallerThanLong(ReadEndsBase const & A) const
			{
				uint64_t sum = 0;
				for ( uint64_t i = 0; i < size(); ++i )
					sum += getBlock(i)->countSmallerThanLong(A);
				return sum;
			}
			
			uint64_t countSmallerThanShort(libmaus::bambam::ReadEndsBase::hash_value_type const & H) const
			{
				ReadEndsBase B;
				B.decodeShortHash(H);
				return countSmallerThanShort(B);
			}

			uint64_t countSmallerThanLong(libmaus::bambam::ReadEndsBase::hash_value_type const & H) const
			{
				ReadEndsBase B;
				B.decodeLongHash(H);
				return countSmallerThanLong(B);
			}
			
			libmaus::bambam::ReadEndsBase::hash_value_type hashLowerBound() const
			{
				return libmaus::bambam::ReadEndsBase::hash_value_type(0);
			}
			
			libmaus::bambam::ReadEndsBase::hash_value_type hashUpperBoundShort() const
			{
				return max().encodeShortHash() + libmaus::bambam::ReadEndsBase::hash_value_type(1);
			}

			libmaus::bambam::ReadEndsBase::hash_value_type hashUpperBoundLong() const
			{
				return max().encodeLongHash() + libmaus::bambam::ReadEndsBase::hash_value_type(1);
			}

			
			typedef libmaus::util::UnsignedIntegerIndexIterator<ReadEndsBlockDecoderBaseCollectionCountSmallerShortAccessor,uint64_t,libmaus::bambam::ReadEndsBase::hash_value_words>
				outer_short_iterator;
			typedef libmaus::util::UnsignedIntegerIndexIterator<ReadEndsBlockDecoderBaseCollectionCountSmallerLongAccessor,uint64_t,libmaus::bambam::ReadEndsBase::hash_value_words>
				outer_long_iterator;
				
			outer_short_iterator outerShortBegin() const
			{
				return outer_short_iterator(&countShortAccessor,hashLowerBound());
			}

			outer_short_iterator outerShortEnd() const
			{
				return outer_short_iterator(&countShortAccessor,hashUpperBoundShort());
			}

			outer_long_iterator outerLongBegin() const
			{
				return outer_long_iterator(&countLongAccessor,hashLowerBound());
			}

			outer_long_iterator outerLongEnd() const
			{
				return outer_long_iterator(&countLongAccessor,hashUpperBoundLong());
			}

			std::vector < std::vector< std::pair<uint64_t,uint64_t> > > getShortMergeIntervals(uint64_t const numthreads, bool const check = false)
			{
				std::vector < std::vector< std::pair<uint64_t,uint64_t> > > R(numthreads,std::vector< std::pair<uint64_t,uint64_t> >(size()));
				uint64_t const totalentries = totalEntries();
				uint64_t const targetfrac = totalentries/numthreads;
				std::vector< ::libmaus::bambam::ReadEnds::hash_value_type > splitPoints(numthreads);

				for ( uint64_t i = 0; i < numthreads; ++i )
				{
					splitPoints[i] = std::lower_bound(outerShortBegin(),outerShortEnd(),i * targetfrac).i;
					::libmaus::bambam::ReadEnds RA;
					RA.decodeShortHash(splitPoints[i]);
					
					for ( uint64_t j = 0; j < size(); ++j )
						R[i][j].first = getBlock(j)->countSmallerThanShort(RA);
				}

				for ( uint64_t i = 0; (i+1) < numthreads; ++i )
					for ( uint64_t j = 0; j < size(); ++j )
						R[i][j].second = R[i+1][j].first;
				
				if ( numthreads )
					for ( uint64_t j = 0; j < size(); ++j )
						R[numthreads-1][j].second = getBlock(j)->size();
				
				if ( check )
				{
					for ( uint64_t i = 0; (i+1) < numthreads; ++i )
					{
						::libmaus::bambam::ReadEnds::hash_value_type const splitlow = splitPoints[i];
						::libmaus::bambam::ReadEnds::hash_value_type const splithigh = splitPoints[i+1];
						
						for ( uint64_t j = 0; j < size(); ++j )
						{
							std::pair<uint64_t,uint64_t> ind = R[i][j];
							
							for ( uint64_t k = ind.first; k < ind.second; ++k )
							{
								assert ( getBlock(j)->get(k).encodeShortHash() >= splitlow );
								assert ( getBlock(j)->get(k).encodeShortHash() < splithigh );
							}
						}
					}
					
					if ( numthreads )
					{					
						::libmaus::bambam::ReadEnds::hash_value_type const splitlow = splitPoints[numthreads-1];

						for ( uint64_t j = 0; j < size(); ++j )
						{
							std::pair<uint64_t,uint64_t> ind = R[numthreads-1][j];
							
							for ( uint64_t k = ind.first; k < ind.second; ++k )
							{
								assert ( getBlock(j)->get(k).encodeShortHash() >= splitlow );
							}
						}
					}
				}
								
				return R;
			}

			static uint64_t getTotalEntries(std::vector<ReadEndsBlockDecoderBaseCollectionInfoBase> const & info)
			{
				uint64_t s = 0;
				for ( uint64_t k = 0; k < info.size(); ++k )
				{
					ReadEndsBlockDecoderBaseCollectionInfo const & subinfo = info[k];
					
					for ( uint64_t i = 0; i < subinfo.blockelcnt.size(); ++i )
						s += subinfo.blockelcnt.at(i);
				}
				
				return s;
			}						

			static std::vector < std::vector< std::pair<uint64_t,uint64_t> > > getLongMergeIntervals(
				std::vector<ReadEndsBlockDecoderBaseCollectionInfoBase> const & rinfo,
				uint64_t const numthreads, 
				bool const check = false
			)
			{
				std::vector < std::vector< std::pair<uint64_t,uint64_t> > > R(numthreads,std::vector< std::pair<uint64_t,uint64_t> >(0));
				uint64_t const totalentries = getTotalEntries(rinfo);
				uint64_t const targetfrac = totalentries/numthreads;
				std::vector< ::libmaus::bambam::ReadEnds::hash_value_type > splitPoints(numthreads);

				#if defined(_OPENMP)
				#pragma omp parallel for
				#endif
				for ( uint64_t i = 0; i < numthreads; ++i )
				{
					this_type col(rinfo);
					splitPoints[i] = std::lower_bound(col.outerLongBegin(),col.outerLongEnd(),i * targetfrac).i;
					::libmaus::bambam::ReadEnds RA;
					RA.decodeLongHash(splitPoints[i]);
					
					for ( uint64_t j = 0; j < col.size(); ++j )
						R[i].push_back(std::pair<uint64_t,uint64_t>(col.getBlock(j)->countSmallerThanLong(RA),0));
				}

				for ( uint64_t i = 0; (i+1) < numthreads; ++i )
					for ( uint64_t j = 0; j < R[i].size(); ++j )
						R[i][j].second = R[i+1][j].first;
				
				if ( numthreads )
				{
					uint64_t j = 0;
					for ( uint64_t k = 0; k < rinfo.size(); ++k )
					{
						ReadEndsBlockDecoderBaseCollectionInfo const & subinfo = rinfo[k];
					
						for ( uint64_t i = 0; i < subinfo.blockelcnt.size(); ++i )
							R[numthreads-1][j++].second = subinfo.blockelcnt[i];
					}
				}
						
				if ( check )
				{					
					for ( uint64_t i = 0; (i+1) < numthreads; ++i )
					{
						this_type col(rinfo);

						::libmaus::bambam::ReadEnds::hash_value_type const splitlow = splitPoints[i];
						::libmaus::bambam::ReadEnds::hash_value_type const splithigh = splitPoints[i+1];
						
						for ( uint64_t j = 0; j < col.size(); ++j )
						{
							std::pair<uint64_t,uint64_t> ind = R[i][j];
							
							for ( uint64_t k = ind.first; k < ind.second; ++k )
							{
								assert ( col.getBlock(j)->get(k).encodeLongHash() >= splitlow );
								assert ( col.getBlock(j)->get(k).encodeLongHash() < splithigh );
							}
						}
					}
					
					if ( numthreads )
					{					
						this_type col(rinfo);
						
						::libmaus::bambam::ReadEnds::hash_value_type const splitlow = splitPoints[numthreads-1];

						for ( uint64_t j = 0; j < col.size(); ++j )
						{
							std::pair<uint64_t,uint64_t> ind = R[numthreads-1][j];
							
							for ( uint64_t k = ind.first; k < ind.second; ++k )
							{
								assert ( col.getBlock(j)->get(k).encodeLongHash() >= splitlow );
							}
						}
					}
				}
								
				return R;
			}

			std::vector < std::vector< std::pair<uint64_t,uint64_t> > > getLongMergeIntervals(uint64_t const numthreads, bool const check = false)
			{
				std::vector < std::vector< std::pair<uint64_t,uint64_t> > > R(numthreads,std::vector< std::pair<uint64_t,uint64_t> >(size()));
				uint64_t const totalentries = totalEntries();
				uint64_t const targetfrac = totalentries/numthreads;
				std::vector< ::libmaus::bambam::ReadEnds::hash_value_type > splitPoints(numthreads);

				for ( uint64_t i = 0; i < numthreads; ++i )
				{
					splitPoints[i] = std::lower_bound(outerLongBegin(),outerLongEnd(),i * targetfrac).i;
					::libmaus::bambam::ReadEnds RA;
					RA.decodeLongHash(splitPoints[i]);
					
					for ( uint64_t j = 0; j < size(); ++j )
						R[i][j].first = getBlock(j)->countSmallerThanLong(RA);
				}

				for ( uint64_t i = 0; (i+1) < numthreads; ++i )
					for ( uint64_t j = 0; j < size(); ++j )
						R[i][j].second = R[i+1][j].first;
				
				if ( numthreads )
					for ( uint64_t j = 0; j < size(); ++j )
						R[numthreads-1][j].second = getBlock(j)->size();
						
				if ( check )
				{
					for ( uint64_t i = 0; (i+1) < numthreads; ++i )
					{
						::libmaus::bambam::ReadEnds::hash_value_type const splitlow = splitPoints[i];
						::libmaus::bambam::ReadEnds::hash_value_type const splithigh = splitPoints[i+1];
						
						for ( uint64_t j = 0; j < size(); ++j )
						{
							std::pair<uint64_t,uint64_t> ind = R[i][j];
							
							for ( uint64_t k = ind.first; k < ind.second; ++k )
							{
								assert ( getBlock(j)->get(k).encodeLongHash() >= splitlow );
								assert ( getBlock(j)->get(k).encodeLongHash() < splithigh );
							}
						}
					}
					
					if ( numthreads )
					{					
						::libmaus::bambam::ReadEnds::hash_value_type const splitlow = splitPoints[numthreads-1];

						for ( uint64_t j = 0; j < size(); ++j )
						{
							std::pair<uint64_t,uint64_t> ind = R[numthreads-1][j];
							
							for ( uint64_t k = ind.first; k < ind.second; ++k )
							{
								assert ( getBlock(j)->get(k).encodeLongHash() >= splitlow );
							}
						}
					}
				}
								
				return R;
			}
			
			void merge(
				std::vector< std::pair<uint64_t,uint64_t> > V,
				std::ostream & out, std::ostream & indexout
			) const
			{
				//! pair of list index and ReadEnds object
				typedef std::pair<uint64_t,::libmaus::bambam::ReadEnds> qtype;
				//! merge heap
				std::priority_queue<qtype,std::vector<qtype>,::libmaus::bambam::ReadEndsHeapPairComparator> Q;
				
				for ( uint64_t i = 0; i < V.size(); ++i )
					if ( V[i].first < V[i].second )
						Q.push(
							qtype(
								i,
								getBlock(i)->get(V[i].first++)
							)
						);
				
				uint64_t ind = 0;
				libmaus::lz::SnappyOutputStream<std::ostream> SOS(out);
				while ( Q.size() )
				{
					qtype const P = Q.top();
					Q.pop();
					
					P.second.put(SOS);
					if ( ((ind++) & ReadEndsContainerBase::indexMask) == 0 )
					{
						std::pair<uint64_t,uint64_t> const offp = SOS.getOffset();
						libmaus::util::NumberSerialisation::serialiseNumber(indexout,offp.first);
						libmaus::util::NumberSerialisation::serialiseNumber(indexout,offp.second);
					}
					
					if ( V[P.first].first < V[P.first].second )
						Q.push(
							qtype(
								P.first,
								getBlock(P.first)->get(V[P.first].first++)
							)
						);
				}
				
				SOS.flush();
				out.flush();
				indexout.flush();
			}
		};
	}
}
#endif
