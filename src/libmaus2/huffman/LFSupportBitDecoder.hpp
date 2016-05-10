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
#if !defined(LIBMAUS2_HUFFMAN_LFSSUPPORTBITDECODER_HPP)
#define LIBMAUS2_HUFFMAN_LFSSUPPORTBITDECODER_HPP

#include <libmaus2/aio/SynchronousGenericInput.hpp>
#include <libmaus2/math/lowbits.hpp>

namespace libmaus2
{
	namespace huffman
	{
		struct LFSSupportBitDecoder
		{
			libmaus2::aio::SynchronousGenericInput<uint64_t> & SGI;

			uint64_t w;
			unsigned int av;

			uint64_t peekslot;
			unsigned int peekslotnumbits;

			LFSSupportBitDecoder(libmaus2::aio::SynchronousGenericInput<uint64_t> & rSGI)
			: SGI(rSGI), w(0), av(0), peekslot(0), peekslotnumbits(0) {}

			// read b bits
			uint64_t rawRead(unsigned int b)
			{
				uint64_t r = 0;

				while ( b )
				{
					if ( ! av )
					{
						SGI.getNext(w);
						av = 8*sizeof(uint64_t);
					}

					// bits to add in this round
					unsigned int const lav = std::min(b,av);

					// make room for bits in r
					r <<= lav;

					// add bits
					r |= w >> (8*sizeof(uint64_t) - lav);

					// shift out bits from w
					if ( lav < 8*sizeof(uint64_t) )
						w <<= lav;
					else
						w = 0;

					av -= lav;
					b -= lav;
				}

				return r;
			}

			static uint64_t leftShift(uint64_t const v, unsigned int const b)
			{
				if ( expect_false(b == CHAR_BIT*sizeof(uint64_t)) )
					return 0;
				else
					return v << b;
			}

			static uint64_t rightShift(uint64_t const v, unsigned int const b)
			{
				if ( expect_false(b == CHAR_BIT*sizeof(uint64_t)) )
					return 0;
				else
					return v >> b;
			}

			void ensurePeekSlot(unsigned int const numbits)
			{
				if ( peekslotnumbits < numbits )
				{
					unsigned int const need = numbits - peekslotnumbits;
					peekslot = leftShift(peekslot,need) | rawRead(need);
					peekslotnumbits += need;
				}

				assert ( peekslotnumbits >= numbits );
			}

			uint64_t peek(unsigned int const numbits)
			{
				ensurePeekSlot(numbits);
				return peekslot >> (peekslotnumbits-numbits);
			}

			void erase(unsigned int numbits)
			{
				ensurePeekSlot(numbits);
				peekslotnumbits -= numbits;
				peekslot &= ::libmaus2::math::lowbits(peekslotnumbits);
			}

			uint64_t read(unsigned int numbits)
			{
				uint64_t const v = peek(numbits);
				erase(numbits);
				return v;
			}

			uint64_t readBit()
			{
				return read(1);
			}

			void pushBackPeek()
			{
				// while ( peekslotnumbits && (av != (CHAR_BIT * sizeof(uint64_t))) )
				{
					unsigned int const tocopy = std::min(
						static_cast<unsigned int>(peekslotnumbits),
						static_cast<unsigned int>((CHAR_BIT * sizeof(uint64_t))-av));

					w |= leftShift(peekslot & libmaus2::math::lowbits(tocopy), av);
					av += tocopy;
					peekslot = rightShift(peekslot,tocopy);
					peekslotnumbits -= tocopy;
				}
			}

			void flush()
			{
				pushBackPeek();

				if ( av + peekslotnumbits >= 64 )
				{
					assert ( av == 64 );
					SGI.putBack();
					av = 0;
					w = 0;
				}

				assert ( av + peekslotnumbits < 64 );

				w = 0;
				av = 0;
				peekslot = 0;
				peekslotnumbits = 0;
			}
		};
	}
}
#endif
