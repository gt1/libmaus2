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
#if ! defined(LIBMAUS_DIGEST_DIGESTBASE_HPP)
#define LIBMAUS_DIGEST_DIGESTBASE_HPP

#include <libmaus/math/UnsignedInteger.hpp>

namespace libmaus
{
	namespace digest
	{
		template<size_t _digestlength>
		struct DigestBase
		{
			enum { digestlength = _digestlength };
		
			virtual ~DigestBase() {}
			virtual void digest(uint8_t * digest) = 0;

			libmaus::math::UnsignedInteger<digestlength/4> digestui() 
			{
				uint8_t adigest[digestlength];
				digest(adigest);
				
				libmaus::math::UnsignedInteger<digestlength/4> U;
				uint8_t * udigest = &adigest[0];
				for ( size_t i = 0; i < digestlength/4; ++i )
				{
					U[digestlength/4-i-1] = 
						(static_cast<uint32_t>(udigest[0]) << 24) |
						(static_cast<uint32_t>(udigest[1]) << 16) |
						(static_cast<uint32_t>(udigest[2]) <<  8) |
						(static_cast<uint32_t>(udigest[3]) <<  0)
						;
					udigest += 4;
				}
				
				return U;
			}
		};
	}
}
#endif
