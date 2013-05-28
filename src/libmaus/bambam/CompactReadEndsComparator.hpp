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
#if ! defined(LIBMAUS_BAMBAM_COMPACTREADENDSCOMPARATOR_HPP)
#define LIBMAUS_BAMBAM_COMPACTREADENDSCOMPARATOR_HPP

#include <libmaus/types/types.hpp>
#include <libmaus/bambam/CompactReadEndsBase.hpp>
#include <libmaus/rank/ERankBase.hpp>

namespace libmaus
{
	namespace bambam
	{
		struct CompactReadEndsComparator : public ::libmaus::bambam::CompactReadEndsBase
		{
			uint8_t const * D;
			
			CompactReadEndsComparator(uint8_t const * rD) : D(rD)
			{
			
			}
			
			static void prepare(uint8_t * pa, uint64_t const n)
			{
				#if defined(LIBMAUS_HAVE_x86_64)
				libmaus::timing::RealTimeClock rtc; rtc.start();
				for ( uint64_t i = 0; i < n; ++i )
				{
					uint32_t const lena = decodeLength(pa);
					unsigned int const awords = lena >> 3;
					uint32_t const resta = lena-(awords<<3);
					
					uint64_t * wa = reinterpret_cast<uint64_t *>(pa);
					uint64_t * we = wa + awords;
					for ( ; wa != we; ++wa )
						*wa = libmaus::rank::BSwapBase::bswap8(*wa);
						
					pa = reinterpret_cast<uint8_t *>(wa);
					pa += resta;
				}
				#endif
			}

			bool operator()(uint32_t const a, uint32_t const b) const
			{
				uint8_t const * pa = D+a;
				uint8_t const * pb = D+b;
				
				uint32_t const lena = decodeLength(pa);
				uint32_t const lenb = decodeLength(pb);

				uint8_t const * const pae = pa + lena;
				uint8_t const * const pbe = pb + lenb;
				
				#if defined(LIBMAUS_HAVE_x86_64)
				unsigned int const awords = lena >> 3;
				unsigned int const bwords = lenb >> 3;
				unsigned int const ewords = std::min(awords,bwords);
				
				uint64_t const * wa = reinterpret_cast<uint64_t const *>(pa);
				uint64_t const * const wae = wa + ewords;
				uint64_t const * wb = reinterpret_cast<uint64_t const *>(pb);
				
				while ( wa != wae )
					if ( *wa != *wb )
						return *wa < *wb;
					else
						++wa, ++wb;

				pa = reinterpret_cast<uint8_t const *>(wa);
				pb = reinterpret_cast<uint8_t const *>(wb);
				#endif
						
				
				while ( pa != pae && pb != pbe && *pa == *pb )
					++pa, ++pb;
				
				if ( pa != pae && pb != pbe )
					return *pa < *pb;
				// equal	
				else if ( pa == pae && pb == pbe )
					return false;
				// pa is shorter
				else if ( pa == pae )
					return true;
				// pb is shorter
				else // if ( pb == pbe )
					return false;
			}
		};
	}
}
#endif
