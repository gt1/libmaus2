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
#if ! defined(LIBMAUS2_BAMBAM_READENDSBLOCKDECODERBASE_HPP)
#define LIBMAUS2_BAMBAM_READENDSBLOCKDECODERBASE_HPP

#include <libmaus2/bambam/CompactReadEndsBase.hpp>
#include <libmaus2/bambam/CompactReadEndsComparator.hpp>
#include <libmaus2/bambam/ReadEnds.hpp>
#include <libmaus2/bambam/ReadEndsBaseLongHashAttributeComparator.hpp>
#include <libmaus2/bambam/ReadEndsBaseShortHashAttributeComparator.hpp>
#include <libmaus2/bambam/ReadEndsContainerBase.hpp>
#include <libmaus2/bambam/ReadEndsBlockDecoderBaseCollectionInfo.hpp>
#include <libmaus2/util/iterator.hpp>
#include <libmaus2/lru/SparseLRU.hpp>
#include <libmaus2/index/ExternalMemoryIndexDecoder.hpp>

namespace libmaus2
{
	namespace bambam
	{
		template<bool _proxy>
		struct ReadEndsBlockDecoderBase : public ::libmaus2::bambam::ReadEndsContainerBase
		{
			static bool const proxy = _proxy;

			typedef ReadEndsBlockDecoderBase<proxy> this_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			typedef libmaus2::util::ConstIterator<this_type,ReadEnds> const_iterator;

			private:
			ReadEndsBlockDecoderBaseCollectionInfoDataStreamProvider & dataprovider;
			ReadEndsBlockDecoderBaseCollectionInfoIndexStreamProvider & indexprovider;
			std::istream & data;
			std::istream & index;

			uint64_t const indexoffset;
			uint64_t const numentries;
			uint64_t const numblocks;

			libmaus2::index::ExternalMemoryIndexDecoder<ReadEndsBase,baseIndexShift,innerIndexShift>::unique_ptr_type Pindex;
			libmaus2::index::ExternalMemoryIndexDecoder<ReadEndsBase,baseIndexShift,innerIndexShift> & Rindex;

			mutable uint64_t blockloaded;
			mutable bool blockloadedvalid;
			mutable libmaus2::autoarray::AutoArray<libmaus2::bambam::ReadEnds> B;

			mutable libmaus2::lru::SparseLRU primaryProxyLRU;
			mutable std::map<uint64_t,ReadEnds> primaryProxyMap;

			mutable libmaus2::lru::SparseLRU secondaryProxyLRU;
			mutable std::map<uint64_t,ReadEnds> secondaryProxyMap;

			libmaus2::index::ExternalMemoryIndexDecoder<ReadEndsBase,baseIndexShift,innerIndexShift>::unique_ptr_type openIndex()
			{
				index.clear();
				index.seekg(indexoffset,std::ios::beg);
				libmaus2::index::ExternalMemoryIndexDecoder<ReadEndsBase,baseIndexShift,innerIndexShift>::unique_ptr_type Tindex(
					new libmaus2::index::ExternalMemoryIndexDecoder<ReadEndsBase,baseIndexShift,innerIndexShift>(index)
				);
				return UNIQUE_PTR_MOVE(Tindex);
			}

			void loadBlock(uint64_t const i) const
			{
				uint64_t const blocklow  = i << baseIndexShift;
				uint64_t const blockhigh = std::min(blocklow + baseIndexStep, numentries);
				uint64_t const blocksize = blockhigh-blocklow;

				assert ( i < numblocks );
				std::pair<uint64_t,uint64_t> const IP = Rindex[i];

				data.clear();
				data.seekg(IP.first);

				libmaus2::lz::SnappyInputStream SIS(data);
				SIS.ignore(IP.second);

				if ( blocksize > B.size() )
				{
					B = libmaus2::autoarray::AutoArray<libmaus2::bambam::ReadEnds>();
					B = libmaus2::autoarray::AutoArray<libmaus2::bambam::ReadEnds>(blocksize,false);
				}

				for ( uint64_t j = 0; j < blocksize; ++j )
					B[j].get(SIS);

				if ( proxy )
				{
					for ( uint64_t j = 0; j < blocksize; ++j )
					{
						uint64_t const absj = blocklow+j;

						if ( secondaryProxyMap.find(absj) == secondaryProxyMap.end() )
						{
							int64_t kickid = secondaryProxyLRU.get(absj);

							if ( kickid >= 0 )
								secondaryProxyMap.erase(secondaryProxyMap.find(kickid));

							secondaryProxyMap[absj] = B[j];
						}
					}
				}

				blockloaded = i;
				blockloadedvalid = true;
			}

			public:
			ReadEndsBlockDecoderBase(
				ReadEndsBlockDecoderBaseCollectionInfoDataStreamProvider & rdataprovider,
				ReadEndsBlockDecoderBaseCollectionInfoIndexStreamProvider & rindexprovider,
				std::istream & rdata,
				std::istream & rindex,
				uint64_t const rindexoffset,
				uint64_t const rnumentries
			) :
			    dataprovider(rdataprovider),
			    indexprovider(rindexprovider),
			    data(rdata), index(rindex), indexoffset(rindexoffset), numentries(rnumentries),
			    numblocks((numentries + baseIndexStep-1)/baseIndexStep),
			    Pindex(openIndex()),
			    Rindex(*Pindex),
			    blockloaded(0), blockloadedvalid(false),
			    primaryProxyLRU(proxy ? 1024 : 0),
			    secondaryProxyLRU(proxy ? 2*ReadEndsContainerBase::baseIndexStep : 0)
			{
			}


			ReadEnds const & operator[](uint64_t const i) const
			{
				return get(i);
			}

			ReadEnds const & get(uint64_t const i) const
			{
				assert ( i < numentries );

				if ( proxy )
				{
					std::map<uint64_t,ReadEnds>::const_iterator pita = primaryProxyMap.find(i);
					if ( pita != primaryProxyMap.end() )
					{
						primaryProxyLRU.update(i);
						return pita->second;
					}

					std::map<uint64_t,ReadEnds>::const_iterator sita = secondaryProxyMap.find(i);
					if ( sita != secondaryProxyMap.end() )
					{
						secondaryProxyLRU.update(i);

						// copy element to primary proxy
						int64_t kickid = primaryProxyLRU.get(i);

						if ( kickid >= 0 )
							primaryProxyMap.erase(primaryProxyMap.find(kickid));

						primaryProxyMap[i] = sita->second;

						return sita->second;
					}
				}

				uint64_t const blockid = i >> baseIndexShift;
				if ( (blockloaded != blockid) || (!blockloadedvalid) )
					loadBlock(blockid);
				uint64_t const blocklow = blockid << baseIndexShift;

				ReadEnds const & el = B[i-blocklow];

				if ( proxy )
				{
					int64_t kickid = primaryProxyLRU.get(i);

					if ( kickid >= 0 )
						primaryProxyMap.erase(primaryProxyMap.find(kickid));

					primaryProxyMap[i] = el;
				}

				return el;
			}

			const_iterator begin() const
			{
				return const_iterator(this,0);
			}

			const_iterator end() const
			{
				return const_iterator(this,numentries);
			}

			uint64_t size() const
			{
				return numentries;
			}

			uint64_t countSmallerThanShort(ReadEndsBase const & A) const
			{
				return std::lower_bound(begin(),end(),A,ReadEndsBaseShortHashAttributeComparator())-begin();
			}

			uint64_t countSmallerThanLong(ReadEndsBase const & A) const
			{
				return std::lower_bound(begin(),end(),A,ReadEndsBaseLongHashAttributeComparator())-begin();
			}
		};
	}
}
#endif
