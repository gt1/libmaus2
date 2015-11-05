/*
    libmaus2
    Copyright (C) 2015 German Tischler

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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_TANDEM_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_TANDEM_HPP

#include <libmaus2/dazzler/align/OverlapMeta.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct Tandem
			{
				uint64_t low;
				uint64_t high;
				std::vector<uint64_t> periods;
				std::vector<libmaus2::dazzler::align::OverlapMeta> meta;

				Tandem() {}
				Tandem(uint64_t const rlow, uint64_t const rhigh, std::vector<uint64_t> const & rperiods, std::vector<libmaus2::dazzler::align::OverlapMeta> const & rmeta)
				: low(rlow), high(rhigh), periods(rperiods), meta(rmeta) {}
				Tandem(std::istream & in)
				{
					deserialise(in);
				}

				uint64_t serialise(std::ostream & out) const
				{
					uint64_t offset = 0;
					libmaus2::dazzler::db::OutputBase::putLittleEndianInteger8(out,low,offset);
					libmaus2::dazzler::db::OutputBase::putLittleEndianInteger8(out,high,offset);
					libmaus2::dazzler::db::OutputBase::putLittleEndianInteger8(out,periods.size(),offset);
					for ( uint64_t i = 0; i < periods.size(); ++i )
						libmaus2::dazzler::db::OutputBase::putLittleEndianInteger8(out,periods[i],offset);
					libmaus2::dazzler::db::OutputBase::putLittleEndianInteger8(out,meta.size(),offset);
					uint64_t s = 0;
					for ( uint64_t i = 0; i < meta.size(); ++i )
						 s += meta[i].serialise(out);
					return (4 + periods.size()) * sizeof(uint64_t) + s;
				}

				uint64_t deserialise(std::istream & in)
				{
					uint64_t offset = 0;
					low = libmaus2::dazzler::db::InputBase::getLittleEndianInteger8(in,offset);
					high = libmaus2::dazzler::db::InputBase::getLittleEndianInteger8(in,offset);
					uint64_t const vsize = libmaus2::dazzler::db::InputBase::getLittleEndianInteger8(in,offset);
					periods.resize(vsize);
					for ( uint64_t i = 0; i < vsize; ++i )
						periods[i] = libmaus2::dazzler::db::InputBase::getLittleEndianInteger8(in,offset);
					uint64_t const mvsize = libmaus2::dazzler::db::InputBase::getLittleEndianInteger8(in,offset);
					meta.resize(mvsize);
					uint64_t s = 0;
					for ( uint64_t i = 0; i < mvsize; ++i )
						s += meta[i].deserialise(in);
					return (4 + periods.size()) * sizeof(uint64_t) + s;
				}
			};

			std::ostream & operator<<(std::ostream & out, Tandem const & T);
		}
	}
}
#endif
