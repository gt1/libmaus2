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


#if ! defined(PUTBITS_HPP)
#define PUTBITS_HPP

#include <libmaus2/math/MetaLog.hpp>
#include <libmaus2/math/lowbits.hpp>

namespace libmaus2
{
	namespace bitio
	{
		#if defined(PUTBIT_DEBUG)
		template<typename value_type>
		std::string sbits(value_type v)
		{
			std::ostringstream ostr;
			value_type mask = static_cast<value_type>(1) << (8*sizeof(value_type)-1);
			for ( unsigned int i = 0; i < 8*sizeof(value_type); ++i, mask >>= 1 )
				ostr << ((v & mask) != 0);
			return ostr.str();
		}
		#endif

		template<typename value_type>
		void putBits(value_type * const A, uint64_t offset, unsigned int numbits, uint64_t v)
		{
			assert ( numbits <= 64 );
			assert ( (v & ::libmaus2::math::lowbits(numbits)) == v );

			unsigned int const bcnt = 8 * sizeof(value_type);
			unsigned int const bshf = ::libmaus2::math::MetaLog2<bcnt>::log;
			unsigned int byteSkip = (offset >> bshf);
			
			if ( numbits )
			{
				unsigned int const bmsk = (1ul<<bshf)-1ul;

				unsigned int const bitSkip = (offset & bmsk);
				unsigned int const restBits = bcnt-bitSkip; // rest bits in word

				#if defined(PUTBIT_DEBUG)
				std::cerr << "bcnt: " << bcnt << std::endl;
				std::cerr << "bshf: " << bshf << std::endl;
				std::cerr << "bmsk: " << bmsk << std::endl;
				std::cerr << "byteSkip: " << byteSkip << std::endl;
				std::cerr << "bitskip: " << bitSkip << std::endl;
				std::cerr << "restBits: " << restBits << std::endl;
				#endif
				
				unsigned int const bitsinfirstword = std::min(numbits,restBits);
				
				#if defined(PUTBIT_DEBUG)
				std::cerr << "bits in first word: " << bitsinfirstword << std::endl;
				#endif
				
				value_type const firstmask = ::libmaus2::math::lowbits(bitsinfirstword); // (static_cast<value_type>(1ULL) << bitsinfirstword) - 1;
				
				#if defined(PUTBIT_DEBUG)
				std::cerr << "first mask: " << sbits(firstmask) << std::endl;
				#endif
				
				unsigned int const firstshift = bcnt - bitSkip - bitsinfirstword;
				
				#if defined(PUTBIT_DEBUG)
				std::cerr << "first shift: " << firstshift << std::endl;
				#endif
				
				value_type const firstkeepmask = ~(firstmask << firstshift);
				
				#if defined(PUTBIT_DEBUG)
				std::cerr << "firstkeepmask: " << sbits(firstkeepmask) << std::endl;
				#endif

				// remove bits	
				A[byteSkip] &= firstkeepmask;
				// put bits
				A[byteSkip] |= static_cast<value_type>(v >> (numbits - bitsinfirstword)) << firstshift;
				
				#if defined(PUTBIT_DEBUG)
				std::cerr << "value: " << sbits(v) << std::endl;
				std::cerr << "first word: " << sbits(A[byteSkip]) << std::endl;
				#endif
				
				uint64_t const keepmask = ::libmaus2::math::lowbits(numbits - bitsinfirstword); // (1ull << (numbits - bitsinfirstword)) - 1;
				
				#if defined(PUTBIT_DEBUG)
				std::cerr << "keepmask " << sbits(keepmask) << std::endl;
				#endif
				
				v &= keepmask;
				numbits -= bitsinfirstword;
				byteSkip++;
				
				#if defined(PUTBIT_DEBUG)
				std::cerr << "v " << sbits(v) << std::endl;
				std::cerr << "numbits " << numbits << std::endl;
				#endif
			}
			
			while ( numbits >= bcnt )
			{
				unsigned int const innershift = numbits - bcnt;
				uint64_t innermask = ::libmaus2::math::lowbits(innershift); // (1ull << innershift)-1;
				
				#if defined(PUTBIT_DEBUG)
				std::cerr << "innershift " << innershift << std::endl;
				std::cerr << "innermask " << sbits(innermask) << std::endl;
				#endif
				
				A[byteSkip] = (v >> innershift);
				
				#if defined(PUTBIT_DEBUG)
				std::cerr << "innerword " << sbits(A[byteSkip]) << std::endl;
				#endif
				
				v &= innermask;
				numbits -= bcnt;
				byteSkip++;
			}

			if ( numbits )
			{
				unsigned int const lastshift = bcnt - numbits;
				value_type const lastmask = ~(
					(
						// (static_cast<value_type>(1ul) << numbits)-1
						::libmaus2::math::lowbits(numbits)
					) << lastshift);
			
				#if defined(PUTBIT_DEBUG)
				std::cerr << "lastshift " << lastshift << std::endl;
				std::cerr << "lastmask " << sbits(lastmask) << std::endl;
				#endif
			
				A[byteSkip] &= lastmask;
				A[byteSkip] |= (v << lastshift);

				#if defined(PUTBIT_DEBUG)
				std::cerr << "lastword " << sbits(A[byteSkip]) << std::endl;
				#endif
			}
		}

		#if defined(LIBMAUS_HAVE_SYNC_OPS)
		template<typename value_type>
		void putBitsSync(value_type * const A, uint64_t offset, unsigned int numbits, uint64_t v)
		{
			assert ( numbits <= 64 );
			assert ( (v & ::libmaus2::math::lowbits(numbits)) == v );

			unsigned int const bcnt = 8 * sizeof(value_type);
			unsigned int const bshf = ::libmaus2::math::MetaLog2<bcnt>::log;
			unsigned int byteSkip = (offset >> bshf);
			
			if ( numbits )
			{
				unsigned int const bmsk = (1ul<<bshf)-1ul;

				unsigned int const bitSkip = (offset & bmsk);
				unsigned int const restBits = bcnt-bitSkip; // rest bits in word

				#if defined(PUTBIT_DEBUG)
				std::cerr << "bcnt: " << bcnt << std::endl;
				std::cerr << "bshf: " << bshf << std::endl;
				std::cerr << "bmsk: " << bmsk << std::endl;
				std::cerr << "byteSkip: " << byteSkip << std::endl;
				std::cerr << "bitskip: " << bitSkip << std::endl;
				std::cerr << "restBits: " << restBits << std::endl;
				#endif
				
				unsigned int const bitsinfirstword = std::min(numbits,restBits);
				
				#if defined(PUTBIT_DEBUG)
				std::cerr << "bits in first word: " << bitsinfirstword << std::endl;
				#endif
				
				value_type const firstmask = ::libmaus2::math::lowbits(bitsinfirstword); // (static_cast<value_type>(1ULL) << bitsinfirstword) - 1;
				
				#if defined(PUTBIT_DEBUG)
				std::cerr << "first mask: " << sbits(firstmask) << std::endl;
				#endif
				
				unsigned int const firstshift = bcnt - bitSkip - bitsinfirstword;
				
				#if defined(PUTBIT_DEBUG)
				std::cerr << "first shift: " << firstshift << std::endl;
				#endif
				
				value_type const firstkeepmask = ~(firstmask << firstshift);
				
				#if defined(PUTBIT_DEBUG)
				std::cerr << "firstkeepmask: " << sbits(firstkeepmask) << std::endl;
				#endif

				// remove bits
				__sync_fetch_and_and(A+byteSkip,firstkeepmask);
				// A[byteSkip] &= firstkeepmask;
				// put bits
				__sync_fetch_and_or(A+byteSkip,static_cast<value_type>(v >> (numbits - bitsinfirstword)) << firstshift);
				// A[byteSkip] |= static_cast<value_type>(v >> (numbits - bitsinfirstword)) << firstshift;
				
				#if defined(PUTBIT_DEBUG)
				std::cerr << "value: " << sbits(v) << std::endl;
				std::cerr << "first word: " << sbits(A[byteSkip]) << std::endl;
				#endif
				
				uint64_t const keepmask = ::libmaus2::math::lowbits(numbits - bitsinfirstword); // (1ull << (numbits - bitsinfirstword)) - 1;
				
				#if defined(PUTBIT_DEBUG)
				std::cerr << "keepmask " << sbits(keepmask) << std::endl;
				#endif
				
				v &= keepmask;
				numbits -= bitsinfirstword;
				byteSkip++;
				
				#if defined(PUTBIT_DEBUG)
				std::cerr << "v " << sbits(v) << std::endl;
				std::cerr << "numbits " << numbits << std::endl;
				#endif
			}
			
			while ( numbits >= bcnt )
			{
				unsigned int const innershift = numbits - bcnt;
				uint64_t innermask = ::libmaus2::math::lowbits(innershift); // (1ull << innershift)-1;
				
				#if defined(PUTBIT_DEBUG)
				std::cerr << "innershift " << innershift << std::endl;
				std::cerr << "innermask " << sbits(innermask) << std::endl;
				#endif
				
				A[byteSkip] = (v >> innershift);
				
				#if defined(PUTBIT_DEBUG)
				std::cerr << "innerword " << sbits(A[byteSkip]) << std::endl;
				#endif
				
				v &= innermask;
				numbits -= bcnt;
				byteSkip++;
			}

			if ( numbits )
			{
				unsigned int const lastshift = bcnt - numbits;
				value_type const lastmask = ~(
					(
						// (static_cast<value_type>(1ul) << numbits)-1
						::libmaus2::math::lowbits(numbits)
					) << lastshift);
			
				#if defined(PUTBIT_DEBUG)
				std::cerr << "lastshift " << lastshift << std::endl;
				std::cerr << "lastmask " << sbits(lastmask) << std::endl;
				#endif
			
				// A[byteSkip] &= lastmask;
				__sync_fetch_and_and(A+byteSkip,lastmask);
				//A[byteSkip] |= (v << lastshift);
				__sync_fetch_and_or(A+byteSkip,(v << lastshift));

				#if defined(PUTBIT_DEBUG)
				std::cerr << "lastword " << sbits(A[byteSkip]) << std::endl;
				#endif
			}
		}
		#endif

		template<>
		inline void putBits(uint64_t * const A, uint64_t offset, unsigned int numbits, uint64_t v)
		{
			assert ( numbits <= 64 );
			assert ( (v & ::libmaus2::math::lowbits(numbits)) == v );

			unsigned int const bcnt = 8 * sizeof(uint64_t);
			unsigned int const bshf = ::libmaus2::math::MetaLog2<bcnt>::log;
			unsigned int byteSkip = (offset >> bshf);
			
			if ( numbits )
			{
				unsigned int const bmsk = (1ul<<bshf)-1ul;

				unsigned int const bitSkip = (offset & bmsk);
				unsigned int const restBits = bcnt-bitSkip; // rest bits in word

				#if defined(PUTBIT_DEBUG)
				std::cerr << "bcnt: " << bcnt << std::endl;
				std::cerr << "bshf: " << bshf << std::endl;
				std::cerr << "bmsk: " << bmsk << std::endl;
				std::cerr << "byteSkip: " << byteSkip << std::endl;
				std::cerr << "bitskip: " << bitSkip << std::endl;
				std::cerr << "restBits: " << restBits << std::endl;
				#endif
				
				unsigned int const bitsinfirstword = std::min(numbits,restBits);
				
				#if defined(PUTBIT_DEBUG)
				std::cerr << "bits in first word: " << bitsinfirstword << std::endl;
				#endif
				
				uint64_t const firstmask = ::libmaus2::math::lowbits(bitsinfirstword); // (static_cast<uint64_t>(1ULL) << bitsinfirstword) - 1;
				
				#if defined(PUTBIT_DEBUG)
				std::cerr << "first mask: " << sbits(firstmask) << std::endl;
				#endif
				
				unsigned int const firstshift = bcnt - bitSkip - bitsinfirstword;
				
				#if defined(PUTBIT_DEBUG)
				std::cerr << "first shift: " << firstshift << std::endl;
				#endif
				
				uint64_t const firstkeepmask = ~(firstmask << firstshift);
				
				#if defined(PUTBIT_DEBUG)
				std::cerr << "firstkeepmask: " << sbits(firstkeepmask) << std::endl;
				#endif

				// remove bits	
				A[byteSkip] &= firstkeepmask;
				// put bits
				A[byteSkip] |= static_cast<uint64_t>(v >> (numbits - bitsinfirstword)) << firstshift;
				
				#if defined(PUTBIT_DEBUG)
				std::cerr << "value: " << sbits(v) << std::endl;
				std::cerr << "first word: " << sbits(A[byteSkip]) << std::endl;
				#endif
				
				uint64_t const keepmask = ::libmaus2::math::lowbits(numbits - bitsinfirstword); // (1ull << (numbits - bitsinfirstword)) - 1;
				
				#if defined(PUTBIT_DEBUG)
				std::cerr << "keepmask " << sbits(keepmask) << std::endl;
				#endif
				
				v &= keepmask;
				numbits -= bitsinfirstword;
				byteSkip++;
				
				#if defined(PUTBIT_DEBUG)
				std::cerr << "v " << sbits(v) << std::endl;
				std::cerr << "numbits " << numbits << std::endl;
				#endif
			}
			
			if ( numbits )
			{
				unsigned int const lastshift = bcnt - numbits;
				uint64_t const lastmask = ~(
					(
						// (static_cast<uint64_t>(1ul) << numbits)-1
						::libmaus2::math::lowbits(numbits)
					) << lastshift);
			
				#if defined(PUTBIT_DEBUG)
				std::cerr << "lastshift " << lastshift << std::endl;
				std::cerr << "lastmask " << sbits(lastmask) << std::endl;
				#endif
			
				A[byteSkip] &= lastmask;
				A[byteSkip] |= (v << lastshift);

				#if defined(PUTBIT_DEBUG)
				std::cerr << "lastword " << sbits(A[byteSkip]) << std::endl;
				#endif
			}
		}		

		#if defined(LIBMAUS_HAVE_SYNC_OPS)
		template<>
		inline void putBitsSync(uint64_t * const A, uint64_t offset, unsigned int numbits, uint64_t v)
		{
			assert ( numbits <= 64 );
			assert ( (v & ::libmaus2::math::lowbits(numbits)) == v );

			unsigned int const bcnt = 8 * sizeof(uint64_t);
			unsigned int const bshf = ::libmaus2::math::MetaLog2<bcnt>::log;
			unsigned int byteSkip = (offset >> bshf);
			
			if ( numbits )
			{
				unsigned int const bmsk = (1ul<<bshf)-1ul;

				unsigned int const bitSkip = (offset & bmsk);
				unsigned int const restBits = bcnt-bitSkip; // rest bits in word

				#if defined(PUTBIT_DEBUG)
				std::cerr << "bcnt: " << bcnt << std::endl;
				std::cerr << "bshf: " << bshf << std::endl;
				std::cerr << "bmsk: " << bmsk << std::endl;
				std::cerr << "byteSkip: " << byteSkip << std::endl;
				std::cerr << "bitskip: " << bitSkip << std::endl;
				std::cerr << "restBits: " << restBits << std::endl;
				#endif
				
				unsigned int const bitsinfirstword = std::min(numbits,restBits);
				
				#if defined(PUTBIT_DEBUG)
				std::cerr << "bits in first word: " << bitsinfirstword << std::endl;
				#endif
				
				uint64_t const firstmask = ::libmaus2::math::lowbits(bitsinfirstword); // (static_cast<uint64_t>(1ULL) << bitsinfirstword) - 1;
				
				#if defined(PUTBIT_DEBUG)
				std::cerr << "first mask: " << sbits(firstmask) << std::endl;
				#endif
				
				unsigned int const firstshift = bcnt - bitSkip - bitsinfirstword;
				
				#if defined(PUTBIT_DEBUG)
				std::cerr << "first shift: " << firstshift << std::endl;
				#endif
				
				uint64_t const firstkeepmask = ~(firstmask << firstshift);
				
				#if defined(PUTBIT_DEBUG)
				std::cerr << "firstkeepmask: " << sbits(firstkeepmask) << std::endl;
				#endif

				// remove bits	
				//A[byteSkip] &= firstkeepmask;
				__sync_fetch_and_and(A+byteSkip,firstkeepmask);
				// put bits
				//A[byteSkip] |= static_cast<uint64_t>(v >> (numbits - bitsinfirstword)) << firstshift;
				__sync_fetch_and_or (A+byteSkip,static_cast<uint64_t>(v >> (numbits - bitsinfirstword)) << firstshift);

				#if defined(PUTBIT_DEBUG)
				std::cerr << "value: " << sbits(v) << std::endl;
				std::cerr << "first word: " << sbits(A[byteSkip]) << std::endl;
				#endif
				
				uint64_t const keepmask = ::libmaus2::math::lowbits(numbits - bitsinfirstword); // (1ull << (numbits - bitsinfirstword)) - 1;
				
				#if defined(PUTBIT_DEBUG)
				std::cerr << "keepmask " << sbits(keepmask) << std::endl;
				#endif
				
				v &= keepmask;
				numbits -= bitsinfirstword;
				byteSkip++;
				
				#if defined(PUTBIT_DEBUG)
				std::cerr << "v " << sbits(v) << std::endl;
				std::cerr << "numbits " << numbits << std::endl;
				#endif
			}
			
			if ( numbits )
			{
				unsigned int const lastshift = bcnt - numbits;
				uint64_t const lastmask = ~(
					(
						// (static_cast<uint64_t>(1ul) << numbits)-1
						::libmaus2::math::lowbits(numbits)
					) << lastshift);
			
				#if defined(PUTBIT_DEBUG)
				std::cerr << "lastshift " << lastshift << std::endl;
				std::cerr << "lastmask " << sbits(lastmask) << std::endl;
				#endif
			
				//A[byteSkip] &= lastmask;
				__sync_fetch_and_and(A+byteSkip,lastmask);
				// A[byteSkip] |= (v << lastshift);
				__sync_fetch_and_or (A+byteSkip,(v << lastshift));

				#if defined(PUTBIT_DEBUG)
				std::cerr << "lastword " << sbits(A[byteSkip]) << std::endl;
				#endif
			}
		}		
		#endif
	
	}
}
#endif
