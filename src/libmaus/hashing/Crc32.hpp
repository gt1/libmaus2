/*
 * This file contains written code by Stephan Brumme published on http://create.stephan-brumme.com/
 * and modified for libmaus by German Tischler in 2014. The original zlib type license is given below.
 *
 * -----------------
 * Copyright (c) 2011-2013 Stephan Brumme. All rights reserved.
 * see http://create.stephan-brumme.com/disclaimer.html
 *
 * All source code published on http://create.stephan-brumme.com and its sub-pages is licensed similar to the zlib license:
 *
 * This software is provided 'as-is', without any express or implied warranty. In no event will the author be held liable for any damages arising from the use of this software.
 * Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
 * The origin of this software must not be misrepresented; you must not claim that you wrote the original software.
 * If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
 * Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
 * -----------------
 *
 * The changes for libmaus are Copyright (c) 2014 German Tischler, the license remains the same.
 */
#if ! defined(LIBMAUS_HASHING_CRC32_HPP)
#define LIBMAUS_HASHING_CRC32_HPP
 
#include <libmaus/types/types.hpp>
#include <libmaus/rank/BSwapBase.hpp>
#include <cstdlib>

namespace libmaus
{
	namespace hashing
	{
		struct Crc32
		{
			static uint32_t const Crc32Lookup[8][256];
			/**
			 * compute CRC32 (Slicing-by-8 algorithm)
			 **/
			static uint32_t crc32_8bytes(const void* data, size_t length, uint32_t previousCrc32 = 0)
			{
			  uint32_t crc = ~previousCrc32; // same as previousCrc32 ^ 0xFFFFFFFF
			  const uint32_t* current = (const uint32_t*) data;

			  // process eight bytes at once (Slicing-by-8)
			  while (length >= 8)
			  {
#if defined(LIBMAUS_BYTE_ORDER_BIG_ENDIAN)
			    uint32_t one = *current++ ^ libmaus::rank::BSwapBase::bswap4(crc);
			    uint32_t two = *current++;
			    crc  = Crc32Lookup[0][ two      & 0xFF] ^
				   Crc32Lookup[1][(two>> 8) & 0xFF] ^
				   Crc32Lookup[2][(two>>16) & 0xFF] ^
				   Crc32Lookup[3][(two>>24) & 0xFF] ^
				   Crc32Lookup[4][ one      & 0xFF] ^
				   Crc32Lookup[5][(one>> 8) & 0xFF] ^
				   Crc32Lookup[6][(one>>16) & 0xFF] ^
				   Crc32Lookup[7][(one>>24) & 0xFF];
#else
			    uint32_t one = *current++ ^ crc;
			    uint32_t two = *current++;
			    crc  = Crc32Lookup[0][(two>>24) & 0xFF] ^
				   Crc32Lookup[1][(two>>16) & 0xFF] ^
				   Crc32Lookup[2][(two>> 8) & 0xFF] ^
				   Crc32Lookup[3][ two      & 0xFF] ^
				   Crc32Lookup[4][(one>>24) & 0xFF] ^
				   Crc32Lookup[5][(one>>16) & 0xFF] ^
				   Crc32Lookup[6][(one>> 8) & 0xFF] ^
				   Crc32Lookup[7][ one      & 0xFF];
#endif

			    length -= 8;
			  }

			  const uint8_t* currentChar = (const uint8_t*) current;
			  // remaining 1 to 7 bytes (standard algorithm)
			  while (length-- > 0)
			    crc = (crc >> 8) ^ Crc32Lookup[0][(crc & 0xFF) ^ *currentChar++];

			  return ~crc; // same as crc ^ 0xFFFFFFFF
			}
		};
	}
}
#endif
