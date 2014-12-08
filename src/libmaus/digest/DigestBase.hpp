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
#include <sstream>

namespace libmaus
{
	namespace digest
	{
		template<size_t _digestlength, size_t _blockshift, bool _needpad, size_t _numlen, bool _prefersinglecall>
		struct DigestBase
		{
			enum { digestlength = _digestlength };
			enum { blockshift = _blockshift };
			enum { needpad = _needpad };
			enum { numlen = _numlen };
			enum { prefersinglecall = _prefersinglecall };
		
			virtual ~DigestBase() {}
			virtual void digest(uint8_t * digest) = 0;
			
			static uint64_t getPaddedMessageLength(uint64_t const n)
			{
				if ( ! needpad )
				{
					return n;
				}
				else
				{
					// full blocks
					uint64_t const fullblocks = (n >> blockshift);
					// rest bytes in last block
					uint64_t const rest = n - (fullblocks << blockshift);
					// size of block
					uint64_t const blocksize = (1ull << blockshift);
					// sanity check
					assert ( rest < blocksize );
					
					if ( blocksize-rest  >= (1 + numlen) )
						return (fullblocks+1)<<blockshift;
					else
						return (fullblocks+2)<<blockshift;
				}
			}

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
			
			static std::string digestToString(uint8_t const * digest)
			{
				std::ostringstream ostr;
				for ( uint64_t i = 0; i < digestlength; ++i )
					ostr << std::hex  << std::setfill('0') << std::setw(2) << static_cast<int>(digest[i]) << std::dec << std::setw(0);
				return ostr.str();
			}

			static void digestToString(uint8_t const * digest, char * s)
			{
				static char const D[] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' };
				for ( uint64_t i = 0; i < digestlength; ++i )
				{
					(*s++) = D[(digest[i] >> 4)];
					(*s++) = D[(digest[i] & 0xF)];
				}
			}
		};

		template<size_t _blockshift, bool _needpad, size_t _numlen, bool _prefersinglecall>
		struct DigestBase<0,_blockshift,_needpad,_numlen,_prefersinglecall>
		{
			enum { digestlength = 0 };
			enum { blockshift = _blockshift };
			enum { needpad = _needpad };
			enum { numlen = _numlen };
			enum { prefersinglecall = _prefersinglecall };
		
			virtual ~DigestBase() {}
			virtual void digest(uint8_t * digest) = 0;

			static uint64_t getPaddedMessageLength(uint64_t const n)
			{	
				return n;
			}

			libmaus::math::UnsignedInteger<0> digestui() 
			{
				return libmaus::math::UnsignedInteger<0>();
			}
		};
	}
}
#endif
