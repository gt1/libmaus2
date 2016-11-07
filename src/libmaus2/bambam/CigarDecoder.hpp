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
#if ! defined(LIBMAUS2_BAMBAM_CIGARDECODER_HPP)
#define LIBMAUS2_BAMBAM_CIGARDECODER_HPP

#include <libmaus2/bambam/CigarRunLengthDecoder.hpp>
#include <libmaus2/bambam/DecoderBase.hpp>
#include <libmaus2/bambam/BamFlagBase.hpp>
#include <utility>
#include <cassert>

namespace libmaus2
{
	namespace bambam
	{
		struct CigarDecoder : public ::libmaus2::bambam::DecoderBase
		{
			CigarRunLengthDecoder CD;
			std::pair<uint32_t,uint32_t> P;

			CigarDecoder() : CD(), P(0,0) {}
			CigarDecoder(CigarRunLengthDecoder const & RCD) : CD(RCD) {}

			bool peekNext(BamFlagBase::bam_cigar_ops & op)
			{
				if ( ! P.second )
				{
					if ( ! CD.getNext(P) )
						return false;
				}
				assert ( P.second );

				op = static_cast<BamFlagBase::bam_cigar_ops>(P.first);

				return true;
			}

			bool getNext(BamFlagBase::bam_cigar_ops & op)
			{
				bool const ok = peekNext(op);
				if ( ok )
					P.second -= 1;
				return ok;
			}
		};
	}
}
#endif
