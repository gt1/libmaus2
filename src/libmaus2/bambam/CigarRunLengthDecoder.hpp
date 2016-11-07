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
#if ! defined(LIBMAUS2_BAMBAM_CIGARRUNLENGTHDECODER_HPP)
#define LIBMAUS2_BAMBAM_CIGARRUNLENGTHDECODER_HPP

#include <libmaus2/bambam/DecoderBase.hpp>
#include <utility>

namespace libmaus2
{
	namespace bambam
	{
		struct CigarRunLengthDecoder : public ::libmaus2::bambam::DecoderBase
		{
			uint8_t const * C;
			uint32_t n;

			CigarRunLengthDecoder() : C(0), n(0) {}
			CigarRunLengthDecoder(uint8_t const * rC, uint64_t const rn) : C(rC), n(rn) {}
			CigarRunLengthDecoder(CigarRunLengthDecoder const & O) : C(O.C), n(O.n) {}

			bool peekNext(std::pair<uint32_t,uint32_t> & op) const
			{
				if ( n )
				{
					uint32_t const v = getLEInteger(C,4);
					op.first = v & ((1ull<<(4))-1);
					op.second = (v >> 4) & ((1ull<<(32-4))-1);
					return true;
				}
				else
				{
					return false;
				}
			}

			bool getNext(std::pair<uint32_t,uint32_t> & op)
			{
				if ( peekNext(op) )
				{
					n -= 1;
					return true;
				}
				else
				{
					return false;
				}
			}
		};
	}
}
#endif
