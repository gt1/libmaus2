/*
    libmaus2
    Copyright (C) 2016 German Tischler

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
#if ! defined(LIBMAUS2_LCP_PLCPBITDECODER_HPP)
#define LIBMAUS2_LCP_PLCPBITDECODER_HPP

#include <libmaus2/bitio/BitVectorInput.hpp>
#include <libmaus2/rank/ImpCacheLineRank.hpp>
#include <libmaus2/lcp/ComputeSuccinctLCPResult.hpp>

namespace libmaus2
{
	namespace lcp
	{
		struct PLCPBitDecoder
		{
			private:
			libmaus2::rank::ImpCacheLineRank::unique_ptr_type Prank;
			libmaus2::rank::ImpCacheLineRankSelectAdapter::unique_ptr_type Pselect;
			uint64_t n;
			libmaus2::lcp::ComputeSuccinctLCPResult meta;

			uint64_t rawValue(uint64_t const i) const
			{
				return Pselect->select1(i)-(i<<1)-1;
			}

			public:
			PLCPBitDecoder(std::string const & lcpbitname, uint64_t const rn)
			: n(rn)
			{
				libmaus2::rank::ImpCacheLineRank::unique_ptr_type Trank(new libmaus2::rank::ImpCacheLineRank(2*n));
				Prank = UNIQUE_PTR_MOVE(Trank);

				libmaus2::bitio::BitVectorInput::unique_ptr_type BVI(new libmaus2::bitio::BitVectorInput(std::vector<std::string>(1,lcpbitname)));

				libmaus2::rank::ImpCacheLineRank::WriteContext WC = Prank->getWriteContext();
				for ( uint64_t i = 0; i < 2*n; ++i )
					WC.writeBit(BVI->readBit());
				WC.flush();
				BVI.reset();

				libmaus2::aio::InputStreamInstance metaISI(lcpbitname + ".meta");
				meta.deserialise(metaISI);

				libmaus2::rank::ImpCacheLineRankSelectAdapter::unique_ptr_type Tselect(new libmaus2::rank::ImpCacheLineRankSelectAdapter(*Prank));
				Pselect = UNIQUE_PTR_MOVE(Tselect);
			}

			PLCPBitDecoder(std::istream & in)
			{
				deserialise(in);
			}

			std::ostream & serialise(std::ostream & out) const
			{
				Prank->serialise(out);
				libmaus2::util::NumberSerialisation::serialiseNumber(out,n);
				meta.serialise(out);
				return out;
			}

			std::istream & deserialise(std::istream & in)
			{
				libmaus2::rank::ImpCacheLineRank::unique_ptr_type Trank(new libmaus2::rank::ImpCacheLineRank(in));
				Prank = UNIQUE_PTR_MOVE(Trank);
				n = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				meta.deserialise(in);
				return in;
			}

			// return n
			uint64_t size() const
			{
				return n;
			}

			// get PLCP[i]
			uint64_t get(uint64_t const i) const
			{
				uint64_t const p = i + meta.circularshift;

				if ( p >= n )
					return rawValue(p-n);
				else
					return rawValue(p);
			}

			// get PLCP[i]
			uint64_t operator[](uint64_t const i) const
			{
				return get(i);
			}
		};
	}
}
#endif
