/*
    libmaus
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#include <libmaus/LibMausConfig.hpp>
#include <libmaus/digest/CRC32C_sse42.hpp>
#include <libmaus/exception/LibMausException.hpp>

#if defined(LIBMAUS_HAVE_SMMINTRIN_H)
#include <smmintrin.h>
#endif

void libmaus::digest::CRC32C_sse42::update(uint8_t const * t, size_t l) 
{
	#if defined(LIBMAUS_HAVE_SMMINTRIN_H) && defined(LIBMAUS_USE_ASSEMBLY) && defined(LIBMAUS_HAVE_x86_64) && defined(LIBMAUS_HAVE_i386)
	ctx = ~ctx;
		
	size_t const offset = reinterpret_cast<size_t>(t);
		
	if ( offset & 7 )
	{
		if ( offset & 3 )
		{
			if ( offset & 1 )
			{
				if ( l )
				{
					ctx = _mm_crc32_u8(ctx, *t);
					t += 1;
					l -= 1;
				}
			}
			if ( l >= 2 )
			{
				ctx = _mm_crc32_u16(ctx, *reinterpret_cast<uint16_t const *>(t));
				t += 2;
				l -= 2;			
			}
		}	
		if ( l >= 4 )
		{
			ctx = _mm_crc32_u32(ctx, *reinterpret_cast<uint32_t const *>(t));
			t += 4;
			l -= 4;			
		}
	}
	
	uint64_t const * t64 = reinterpret_cast<uint64_t const *>(t);
	uint64_t const * const t64e = t64 + (l>>3);
	
	while ( t64 != t64e )
		ctx = _mm_crc32_u64(ctx, *(t64++));
		
	l &= 7;
	t = reinterpret_cast<uint8_t const *>(t64);
	
	if ( l >= 4 )
	{
		ctx = _mm_crc32_u32(ctx, *reinterpret_cast<uint32_t const *>(t));
		t += 4;
		l -= 4;	
	}
	if ( l >= 2 )
	{
		ctx = _mm_crc32_u16(ctx, *reinterpret_cast<uint16_t const *>(t));
		t += 2;
		l -= 2;	
	}
	if ( l )
	{
		ctx = _mm_crc32_u8(ctx, *t);
	}
	
	ctx = ~ctx;
	#endif
}
