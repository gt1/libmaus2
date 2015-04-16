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

#if ! defined(COMPACTOFFSETONEARRAY_HPP)
#define COMPACTOFFSETONEARRAY_HPP

#include <libmaus/bitio/CompactArrayBase.hpp>
#include <libmaus/bitio/ReverseTable.hpp>
#include <libmaus/bitio/getBit.hpp>
#include <libmaus/bitio/putBit.hpp>
#include <libmaus/bitio/getBits.hpp>
#include <libmaus/bitio/putBits.hpp>

namespace libmaus
{
	namespace bitio
	{
		struct CompactOffsetOneArray : public CompactArrayBase
		{
			uint64_t n;
			unsigned int const b;
			uint64_t const addoffset;
			
			uint64_t * D;
			
			
			uint64_t size() const
			{
				return 3*sizeof(uint64_t) + 1*sizeof(uint64_t *);
			}
				
			CompactOffsetOneArray(
				uint64_t const rn, 
				uint64_t const rb, 
				uint64_t const raddoffset,
				uint64_t * const rD
			) 
			: CompactArrayBase(rb-1), n(rn), b(rb), addoffset(raddoffset), D(rD) 
			{
			}

			uint64_t get(uint64_t i) const { return getBits(i*b+1); }
			void set(uint64_t i, uint64_t v) { putBits(i*b+1, v); }
			
			bool getMSB(uint64_t i) const { return getBit(D,addoffset + i*b); }

			void cnt(
				uint64_t left, uint64_t const right,
				uint64_t & lsb, uint64_t & msb
			)
			{
				uint64_t cnt[2] = {0,0};
				
				for ( ; left != right; ++left )
					cnt[ ::libmaus::bitio::getBit(D,addoffset+left*b) ]++;
					
				lsb = cnt[0];
				msb = cnt[1];					
			}

			void revertBits(uint64_t l, uint64_t h)
			{
				uint64_t const length = h-l;
				
				for ( uint64_t i = 0; i < length/2; ++i )
				{
					bool const v0 = ::libmaus::bitio::getBit(D,addoffset+l+i);
					bool const v1 = ::libmaus::bitio::getBit(D,addoffset+h-i-1);
					
					::libmaus::bitio::putBit(D,addoffset+l+i,v1);
					::libmaus::bitio::putBit(D,addoffset+h-i-1,v0);
				}	
			}

			void revertBits(
				uint64_t const l,
				uint64_t const h,
				ReverseTable const & revtable)
			{
				uint64_t const length = h-l;
				uint64_t const doublewords = length / (revtable.b<<1);
				
				for ( uint64_t i = 0; i < doublewords; ++i )
				{
					uint64_t a = revtable ( revtable.b, ::libmaus::bitio::getBits ( D, addoffset + l + i*revtable.b, revtable.b ));
					uint64_t b = revtable ( revtable.b, ::libmaus::bitio::getBits ( D, addoffset + h - (i+1)*revtable.b, revtable.b ));
					::libmaus::bitio::putBits ( D, addoffset + l+i*revtable.b, revtable.b, b );
					::libmaus::bitio::putBits ( D, addoffset + h-(i+1)*revtable.b, revtable.b, a );		
				}
				
				uint64_t restl = l + doublewords * revtable.b;
				uint64_t resth = h - doublewords * revtable.b;
				
				uint64_t restlength2 = (resth-restl)/2;
				
				uint64_t a = revtable ( restlength2, ::libmaus::bitio::getBits ( D, addoffset + restl, restlength2 ));
				uint64_t b = revtable ( restlength2, ::libmaus::bitio::getBits ( D, addoffset + resth - restlength2, restlength2 ) );
				
				::libmaus::bitio::putBits ( D, addoffset + restl, restlength2, b );
				::libmaus::bitio::putBits ( D, addoffset + resth-restlength2, restlength2, a );
				
				assert ( restlength2 <= revtable.b );
				
				// revertBits(restl,resth);
			}

			void revert(uint64_t l, uint64_t h)
			{
				uint64_t const length = h-l;
				
				for ( uint64_t i = 0; i < length/2; ++i )
				{
					uint64_t const v0 = get(l+i);
					uint64_t const v1 = get(h-i-1);
					
					set ( l+i, v1 );
					set ( h-i-1, v0 );
				}	
			}
			void rearrange(uint64_t mergelen)
			{
				uint64_t const dmergelen = mergelen << 1;
				uint64_t const fullloops = n / dmergelen;
				
				uint64_t l0 = 0;
				uint64_t h0 = l0+mergelen;
				uint64_t l1 = h0;
				uint64_t h1 = l1+mergelen;
				
				for ( uint64_t z = 0; z < fullloops; ++z )
				{
					uint64_t frontbits = (h0-l0);
					uint64_t backbits = (h1-l1);

					uint64_t frontrest = frontbits*(b-1);
					// uint64_t backrest = backbits*(b-1);
					
					uint64_t t0 = l0*b+frontbits;
					uint64_t m0 = t0+frontrest;
					uint64_t t1 = m0+backbits;

					// revert inner parts
					revertBits(t0,m0);
					revertBits(m0,t1);
					// revert all inner
					revertBits(t0,t1);

					l0 += dmergelen;
					h0 = l0+mergelen;
					l1 = h0;
					h1 = l1+mergelen;
				}
				
				// unhandled rest?
				if ( l1 < n )
				{
					h1 = n;

					uint64_t frontbits = (h0-l0);
					uint64_t backbits = (h1-l1);

					uint64_t frontrest = frontbits*(b-1);
					// uint64_t backrest = backbits*(b-1);
					
					uint64_t t0 = l0*b+frontbits;
					uint64_t m0 = t0+frontrest;
					uint64_t t1 = m0+backbits;

					// revert inner parts
					revertBits(t0,m0);
					revertBits(m0,t1);
					// revert all inner
					revertBits(t0,t1);
				}
				
			}
			void rearrange(uint64_t mergelen, ReverseTable const & revtable)
			{
				uint64_t const dmergelen = mergelen << 1;
				uint64_t const fullloops = n / dmergelen;
				
				uint64_t l0 = 0;
				uint64_t h0 = l0+mergelen;
				uint64_t l1 = h0;
				uint64_t h1 = l1+mergelen;
				
				for ( uint64_t z = 0; z < fullloops; ++z )
				{
					uint64_t frontbits = (h0-l0);
					uint64_t backbits = (h1-l1);

					uint64_t frontrest = frontbits*(b-1);
					// uint64_t backrest = backbits*(b-1);
					
					uint64_t t0 = l0*b+frontbits;
					uint64_t m0 = t0+frontrest;
					uint64_t t1 = m0+backbits;

					// revert inner parts
					revertBits(t0,m0,revtable);
					revertBits(m0,t1,revtable);
					// revert all inner
					revertBits(t0,t1,revtable);

					l0 += dmergelen;
					h0 = l0+mergelen;
					l1 = h0;
					h1 = l1+mergelen;
				}
				
				// unhandled rest?
				if ( l1 < n )
				{
					h1 = n;

					uint64_t frontbits = (h0-l0);
					uint64_t backbits = (h1-l1);

					uint64_t frontrest = frontbits*(b-1);
					// uint64_t backrest = backbits*(b-1);
					
					uint64_t t0 = l0*b+frontbits;
					uint64_t m0 = t0+frontrest;
					uint64_t t1 = m0+backbits;

					// revert inner parts
					revertBits(t0,m0,revtable);
					revertBits(m0,t1,revtable);
					// revert all inner
					revertBits(t0,t1,revtable);
				}
				
			}

			void rearrange()
			{
				uint64_t mergelen = 1;
				
				while ( mergelen < n )
				{
					rearrange(mergelen);
					mergelen <<= 1;
				}
			}

			void rearrange(ReverseTable const & revtable)
			{
				uint64_t mergelen = 1;
				
				while ( mergelen < n )
				{
					rearrange(mergelen,revtable);
					mergelen <<= 1;
				}
			}
			
			void mergeSort(uint64_t const l0, uint64_t const h0)
			{
				uint64_t const n = h0-l0;
				uint64_t mergelen = 1;
				
				while ( mergelen < n )
				{
					merge(l0,h0,mergelen);
					mergelen <<= 1;
				}
			}

			void mergeSort()
			{
				mergeSort(0,n);
			}
			
			void merge(
				uint64_t const rl0,
				uint64_t const rh0,
				uint64_t const mergelen
			)
			{
				// length
				uint64_t const n = rh0-rl0;
				// double merge length
				uint64_t const dmergelen = mergelen << 1;
				// number of full merge operations
				uint64_t const fullloops = n / dmergelen;
				
				uint64_t l0 = rl0;
				uint64_t h0 = l0+mergelen;
				uint64_t l1 = h0;
				uint64_t h1 = l1+mergelen;
				
				for ( uint64_t z = 0; z < fullloops; ++z )
				{
					uint64_t lsb0, msb0, lsb1, msb1;
					
					cnt(l0,h0,lsb0,msb0);
					cnt(l1,h1,lsb1,msb1);
					
					uint64_t const t0 = l0+lsb0;
					uint64_t const t1 = l1+lsb1;

					// revert inner parts
					revert(t0,h0);
					revert(h0,t1);
					// revert all inner
					revert(t0,t1);

					l0 += dmergelen;
					h0 = l0+mergelen;
					l1 = h0;
					h1 = l1+mergelen;
				}
				
				// unhandled rest?
				if ( l1 < rh0 )
				{
					h1 = rh0;

					uint64_t lsb0, msb0, lsb1, msb1;
					
					cnt(l0,h0,lsb0,msb0);
					cnt(l1,h1,lsb1,msb1);

					uint64_t const t0 = l0+lsb0;
					uint64_t const t1 = l1+lsb1;

					// revert inner parts
					revert(t0,h0);
					revert(h0,t1);
					// revert all inner
					revert(t0,t1);
				}
			}
		
			void putBits(uint64_t const roffset, uint64_t v)
			{
				assert ( ( v & vmask ) == v );
				
				uint64_t const offset = roffset + addoffset;
				uint64_t * DD = D + (offset >> bshf);
				uint64_t const bitSkip = (offset & bmsk);
				uint64_t const bitsinfirstword = bitsInFirstWord[bitSkip];
				
				uint64_t t = *DD;
				t &= firstKeepMask[bitSkip];
				t |= (v >> (CompactArrayBase::b - bitsinfirstword)) << firstShift[bitSkip];
				*DD = t;

				if ( CompactArrayBase::b - bitsinfirstword )
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
				
				if ( CompactArrayBase::b <= restBits )
				{
					return v >> (restBits - CompactArrayBase::b);
				}
				else
				{
					unsigned int const numbits = CompactArrayBase::b - restBits;
				
					v = (v<<numbits) | (( *(++DD) ) >> (bcnt-numbits));
				
					return v;
				}
			}
		};

	}
}
#endif
