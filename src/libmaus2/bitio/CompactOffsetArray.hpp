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

#if ! defined(COMPACTOFFSETARRAY_HPP)
#define COMPACTOFFSETARRAY_HPP

#include <libmaus2/bitio/CompactArrayBase.hpp>

namespace libmaus2
{
	namespace bitio
	{
		struct CompactOffsetArray : public CompactArrayBase
		{
			uint64_t n;
			uint64_t const addoffset;

			uint64_t * D;


			uint64_t size() const
			{
				return 3*sizeof(uint64_t) + 1*sizeof(uint64_t *);
			}

			CompactOffsetArray(
				uint64_t const rn,
				uint64_t const rb,
				uint64_t const raddoffset,
				uint64_t * const rD
			)
			: CompactArrayBase(rb), n(rn), addoffset(raddoffset), D(rD)
			{
			}

			uint64_t get(uint64_t i) const { return getBits(i*b); }
			void set(uint64_t i, uint64_t v) { putBits(i*b, v); }

			uint64_t postfixIncrement(uint64_t i) { uint64_t const v = get(i); set(i,v+1); return v; }

			void putBits(uint64_t const roffset, uint64_t v)
			{
				assert ( ( v & vmask ) == v );

				uint64_t const offset = roffset + addoffset;
				uint64_t * DD = D + (offset >> bshf);
				uint64_t const bitSkip = (offset & bmsk);
				uint64_t const bitsinfirstword = bitsInFirstWord[bitSkip];

				uint64_t t = *DD;
				t &= firstKeepMask[bitSkip];
				t |= (v >> (b - bitsinfirstword)) << firstShift[bitSkip];
				*DD = t;

				if ( b - bitsinfirstword )
				{
					v &= firstValueKeepMask[bitSkip];
					DD++;
					uint64_t t = *DD;
					t &= lastMask[bitSkip];
					t |= (v << lastShift[bitSkip]);
					*DD = t;
				}
			}

			// get bits from an array
			uint64_t getBits(uint64_t const roffset) const
			{
				uint64_t const offset = roffset + addoffset;
				uint64_t byteSkip = (offset >> bshf);
				uint64_t const bitSkip = (offset & bmsk);
				uint64_t const restBits = bcnt-bitSkip;

				uint64_t const * DD = D + byteSkip;
				uint64_t v = *DD;

				// skip bits by masking them
				v &= getFirstMask[bitSkip];

				if ( b <= restBits )
				{
					return v >> (restBits - b);
				}
				else
				{
					unsigned int const numbits = b - restBits;

					v = (v<<numbits) | (( *(++DD) ) >> (bcnt-numbits));

					return v;
				}
			}

		};
	}
}
#endif
