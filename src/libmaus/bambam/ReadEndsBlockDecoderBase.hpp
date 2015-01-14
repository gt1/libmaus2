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
#if ! defined(LIBMAUS_BAMBAM_READENDSBLOCKDECODERBASE_HPP)
#define LIBMAUS_BAMBAM_READENDSBLOCKDECODERBASE_HPP

#include <libmaus/bambam/CompactReadEndsBase.hpp>
#include <libmaus/bambam/CompactReadEndsComparator.hpp>
#include <libmaus/bambam/ReadEnds.hpp>
#include <libmaus/bambam/ReadEndsBaseLongHashAttributeComparator.hpp>
#include <libmaus/bambam/ReadEndsBaseShortHashAttributeComparator.hpp>
#include <libmaus/bambam/ReadEndsContainerBase.hpp>
#include <libmaus/util/iterator.hpp>
#include <libmaus/lru/SparseLRU.hpp>

namespace libmaus
{
	namespace bambam
	{	
		template<bool _proxy>
		struct ReadEndsBlockDecoderBase : public ::libmaus::bambam::ReadEndsContainerBase
		{
			static bool const proxy = _proxy;
			
			typedef ReadEndsBlockDecoderBase<proxy> this_type;
			typedef typename libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

			typedef libmaus::util::ConstIterator<this_type,ReadEnds> const_iterator;

			std::istream & data;
			std::istream & index;
			
			uint64_t const indexoffset;
			uint64_t const numentries;
			uint64_t const numblocks;
			
			mutable uint64_t blockloaded;
			mutable bool blockloadedvalid;
			mutable libmaus::autoarray::AutoArray<libmaus::bambam::ReadEnds> B;
			mutable libmaus::lru::SparseLRU proxyLRU;
			mutable std::map<uint64_t,ReadEnds> proxyMap;
			
			ReadEndsBlockDecoderBase(
				std::istream & rdata,
				std::istream & rindex,
				uint64_t const rindexoffset,
				uint64_t const rnumentries
			) : data(rdata), index(rindex), indexoffset(rindexoffset), numentries(rnumentries),
			    numblocks((numentries + indexStep-1)/indexStep),
			    blockloaded(0), blockloadedvalid(false),
			    proxyLRU(proxy ? 1024 : 0)
			{
			}
			
			const_iterator begin() const
			{
				return const_iterator(this,0);
			}

			const_iterator end() const
			{
				return const_iterator(this,numentries);
			}
			
			void loadBlock(uint64_t const i) const
			{
				uint64_t const blocklow  = i << indexShift;
				uint64_t const blockhigh = std::min(blocklow + indexStep, numentries);
				uint64_t const blocksize = blockhigh-blocklow;
				
				assert ( i < numblocks );
				index.clear();
				index.seekg(indexoffset + i * 2 * sizeof(uint64_t), std::ios::beg);
				
				uint64_t const blockoffset = libmaus::util::NumberSerialisation::deserialiseNumber(index);
				uint64_t const byteskip = libmaus::util::NumberSerialisation::deserialiseNumber(index);
				
				data.clear();
				data.seekg(blockoffset);
				
				libmaus::lz::SnappyInputStream SIS(data);
				SIS.ignore(byteskip);
				
				if ( blocksize > B.size() )
				{
					B = libmaus::autoarray::AutoArray<libmaus::bambam::ReadEnds>();
					B = libmaus::autoarray::AutoArray<libmaus::bambam::ReadEnds>(blocksize,false);
				}
				
				for ( uint64_t j = 0; j < blocksize; ++j )
					B[j].get(SIS);
					
				blockloaded = i;
				blockloadedvalid = true;
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
					std::map<uint64_t,ReadEnds>::const_iterator ita = proxyMap.find(i);
					if ( ita != proxyMap.end() )
					{
						proxyLRU.update(i);
						return ita->second;
					}
				}
				
				uint64_t const blockid = i >> indexShift;
				if ( (blockloaded != blockid) || (!blockloadedvalid) )
					loadBlock(blockid);
				uint64_t const blocklow = blockid << indexShift;
				
				ReadEnds const & el = B[i-blocklow];
				
				if ( proxy )
				{
					int64_t kickid = proxyLRU.get(i);
					
					if ( kickid >= 0 )
						proxyMap.erase(proxyMap.find(kickid));
						
					proxyMap[i] = el;
				}
				
				return el;
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
