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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_TANDEMVECTOR_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_TANDEMVECTOR_HPP

#include <libmaus2/dazzler/align/Tandem.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct TandemVector : public std::vector< ::libmaus2::dazzler::align::Tandem >
			{
				TandemVector() {}
				TandemVector(std::vector< ::libmaus2::dazzler::align::Tandem > const & V) : std::vector< ::libmaus2::dazzler::align::Tandem>(V) {}
				TandemVector(std::istream & in)
				{
					deserialise(in);
				}
				
				uint64_t serialise(std::ostream & out) const
				{
					uint64_t s = sizeof(uint64_t);
					uint64_t offset = 0;
					libmaus2::dazzler::db::OutputBase::putLittleEndianInteger8(out,std::vector< ::libmaus2::dazzler::align::Tandem>::size(),offset);
					for ( uint64_t i = 0; i < std::vector< ::libmaus2::dazzler::align::Tandem>::size(); ++i )
						s += std::vector< ::libmaus2::dazzler::align::Tandem>::operator[](i).serialise(out);
					return s;
				}

				uint64_t deserialise(std::istream & in)
				{
					uint64_t offset = 0;
					uint64_t s = sizeof(uint64_t);
					uint64_t const vsize = libmaus2::dazzler::db::InputBase::getLittleEndianInteger8(in,offset);
					std::vector< ::libmaus2::dazzler::align::Tandem>::resize(vsize);

					for ( uint64_t i = 0; i < std::vector< ::libmaus2::dazzler::align::Tandem>::size(); ++i )
						s += std::vector< ::libmaus2::dazzler::align::Tandem>::operator[](i).deserialise(in);

					return s;
				}
			};
			
			std::ostream & operator<<(std::ostream & out, TandemVector const & TV);
		}
	}
}
#endif
