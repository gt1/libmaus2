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
#if ! defined(LIBMAUS2_FASTX_COMPACTFASTQCONTAINERDICTIONARY_HPP)
#define LIBMAUS2_FASTX_COMPACTFASTQCONTAINERDICTIONARY_HPP

#include <libmaus2/rank/ImpCacheLineRank.hpp>
#include <libmaus2/fastx/FastInterval.hpp>

namespace libmaus2
{
	namespace fastx
	{
		struct CompactFastQContainerDictionary
		{
			typedef CompactFastQContainerDictionary this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			::libmaus2::rank::ImpCacheLineRank::unique_ptr_type const designators;
			::libmaus2::autoarray::AutoArray<uint64_t> const longptrs;
			::libmaus2::autoarray::AutoArray<uint16_t> const shortptrs;
			::libmaus2::fastx::FastInterval const FI;

			uint64_t byteSize() const
			{
				return
					designators->byteSize()+
					longptrs.byteSize()+
					shortptrs.byteSize()+
					FI.byteSize();
			}

			CompactFastQContainerDictionary(std::istream & in)
			: 
				designators(new ::libmaus2::rank::ImpCacheLineRank(in)), 
				longptrs(in), 
				shortptrs(in), 
				FI(::libmaus2::fastx::FastInterval::deserialise(in)) 
			{
				// std::cerr << "Dict FI " << FI << std::endl;
			}
			
			CompactFastQContainerDictionary(::libmaus2::network::SocketBase * in)
			: 
				designators(new ::libmaus2::rank::ImpCacheLineRank(in)), 
				longptrs(in->readMessageInBlocks<uint64_t,::libmaus2::autoarray::alloc_type_cxx>()), 
				shortptrs(in->readMessageInBlocks<uint16_t,::libmaus2::autoarray::alloc_type_cxx>()), 
				FI(::libmaus2::fastx::FastInterval::deserialise(in->readString())) 
			{
				// std::cerr << "Dict FI " << FI << std::endl;
			}
			
			
			
			void serialise(std::ostream & out) const
			{
				designators->serialise(out);
				longptrs.serialize(out);
				shortptrs.serialize(out);
				::libmaus2::fastx::FastInterval::serialise(out,FI);
			}

			void serialise(::libmaus2::network::SocketBase * socket) const
			{
				designators->serialise(socket);
				socket->writeMessageInBlocks(longptrs);
				socket->writeMessageInBlocks(shortptrs);
				socket->writeString(FI.serialise());
			}

			uint64_t operator[](uint64_t const i) const
			{
				return longptrs[designators->rank1(i)] + shortptrs[i];
			}
		};
	}
}
#endif
