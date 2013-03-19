/**
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
**/

#if ! defined(COMPACTARRAYBASE_HPP)
#define COMPACTARRAYBASE_HPP

#include <libmaus/math/MetaLog.hpp>
#include <libmaus/types/types.hpp>
#include <cassert>
#include <iostream>

namespace libmaus
{
	namespace bitio
	{
		struct CompactArrayBase
		{
			static unsigned int const bcnt = 8 * sizeof(uint64_t);
			static unsigned int const bshf = ::libmaus::math::MetaLog2<bcnt>::log;
			static unsigned int const bmsk = (1ul<<bshf)-1ul;
			
			static unsigned int globalBitsInFirstWord[4160];
			static unsigned int globalFirstShift[4160];
			static uint64_t globalFirstKeepMask[4160];
			static uint64_t globalFirstValueKeepMask[4160];
			static unsigned int globalLastShift[4160];
			static uint64_t globalLastMask[4160];
			static uint64_t globalGetFirstMask[4160];
			static uint64_t globalvmask[65];
			static bool globalinit;
			
			static void printglobaltables()
			{
				initGlobalTables();
				
				std::cerr << "bcnt=" << bcnt << " bshf=" << bshf << " bmsk=" << bmsk << " globalinit=" << globalinit << std::endl;
				
				std::cerr << "globalBitsInFirstWord={";
				for ( uint64_t i = 0; i < sizeof(globalBitsInFirstWord)/sizeof(globalBitsInFirstWord[0]); ++i )
					std::cerr << globalBitsInFirstWord[i] << ";";
				std::cerr << "}" << std::endl;

				std::cerr << "globalFirstShift={";
				for ( uint64_t i = 0; i < sizeof(globalFirstShift)/sizeof(globalFirstShift[0]); ++i )
					std::cerr << globalFirstShift[i] << ";";
				std::cerr << "}" << std::endl;
				
				std::cerr << "globalFirstKeepMask={";
				std::cerr << std::hex;
				for ( uint64_t i = 0; i < sizeof(globalFirstKeepMask)/sizeof(globalFirstKeepMask[0]); ++i )
					std::cerr << "\t" << i << ":" << globalFirstKeepMask[i] << ";\n";
				std::cerr << std::dec;
				std::cerr << "}" << std::endl;

				std::cerr << "globalFirstValueKeepMask={";
				std::cerr << std::hex;
				for ( uint64_t i = 0; i < sizeof(globalFirstValueKeepMask)/sizeof(globalFirstValueKeepMask[0]); ++i )
					std::cerr << globalFirstValueKeepMask[i] << ";";
				std::cerr << std::dec;
				std::cerr << "}" << std::endl;

				std::cerr << "globalLastShift={";
				for ( uint64_t i = 0; i < sizeof(globalLastShift)/sizeof(globalLastShift[0]); ++i )
					std::cerr << globalLastShift[i] << ";";
				std::cerr << "}" << std::endl;

				std::cerr << "globalLastMask={";
				std::cerr << std::hex;
				for ( uint64_t i = 0; i < sizeof(globalLastMask)/sizeof(globalLastMask[0]); ++i )
					std::cerr << globalLastMask[i] << ";";
				std::cerr << std::dec;
				std::cerr << "}" << std::endl;

				std::cerr << "globalGetFirstMask={";
				std::cerr << std::hex;
				for ( uint64_t i = 0; i < sizeof(globalGetFirstMask)/sizeof(globalGetFirstMask[0]); ++i )
					std::cerr << globalGetFirstMask[i] << ";";
				std::cerr << std::dec;
				std::cerr << "}" << std::endl;

				std::cerr << "globalvmask={";
				std::cerr << std::hex;
				for ( uint64_t i = 0; i < sizeof(globalvmask)/sizeof(globalvmask[0]); ++i )
					std::cerr << globalvmask[i] << ";";
				std::cerr << std::dec;
				std::cerr << "}" << std::endl;
			}
			
			static uint64_t lowbits(uint64_t i)
			{
				if ( i < 64 )
					return (static_cast<uint64_t>(1ULL) << i)-1;
				else
					return 0xFFFFFFFFFFFFFFFFULL;
			}
			
			static void initGlobalTables(uint64_t const b)
			{
				assert ( b <= 64 );

				unsigned int * bitsInFirstWord = (&globalBitsInFirstWord[0]) + b*64;
				unsigned int * firstShift = (&globalFirstShift[0]) + b*64;
				uint64_t * firstKeepMask = (&globalFirstKeepMask[0]) + b*64; ;
				uint64_t * firstValueKeepMask = (&globalFirstValueKeepMask[0]) + b*64;
				unsigned int * lastShift = (&globalLastShift[0]) + b*64;
				uint64_t * lastMask = (&globalLastMask[0]) + b*64;
				uint64_t * getFirstMask = (&globalGetFirstMask[0]) + b*64;
				uint64_t & vmask = globalvmask[b];

				for ( uint64_t bitSkip = 0; bitSkip < 64; ++bitSkip )
				{
					bitsInFirstWord[bitSkip] = std::min(b,bcnt-bitSkip);
					firstShift[bitSkip] = bcnt - bitSkip - bitsInFirstWord[bitSkip];
					firstKeepMask[bitSkip] = ~((lowbits(bitsInFirstWord[bitSkip]))<< firstShift[bitSkip]);
					firstValueKeepMask[bitSkip] = lowbits(b-bitsInFirstWord[bitSkip]);
					lastShift[bitSkip] = bcnt - (b - bitsInFirstWord[bitSkip]);
					lastMask[bitSkip] = ~(lowbits(b - bitsInFirstWord[bitSkip]) << lastShift[bitSkip]);
					getFirstMask[bitSkip] = b ? (lowbits(64) >> (bcnt - (bcnt-bitSkip))) : 0;
				}
				
				vmask = lowbits(b);
			
			}
			static void initGlobalTables()
			{
				if ( ! globalinit )
				{
					for ( uint64_t b = 0; b <=64 ; ++b )
						initGlobalTables(b);
					globalinit = true;
				}
			}

			unsigned int const * bitsInFirstWord;
			unsigned int const * firstShift;
			uint64_t const * firstKeepMask;
			uint64_t const * firstValueKeepMask;
			unsigned int const * lastShift;
			uint64_t const * lastMask;
			uint64_t const * getFirstMask;
			uint64_t vmask;
			
			protected:
			uint64_t const b;
			
			public:
			void initTables()
			{
				assert ( b <= 64 );

				initGlobalTables();
				bitsInFirstWord = (&globalBitsInFirstWord[0]) + b*64;
				firstShift = (&globalFirstShift[0]) + b*64;
				firstKeepMask = (&globalFirstKeepMask[0]) + b*64; ;
				firstValueKeepMask = (&globalFirstValueKeepMask[0]) + b*64;
				lastShift = (&globalLastShift[0]) + b*64;
				lastMask = (&globalLastMask[0]) + b*64;
				getFirstMask = (&globalGetFirstMask[0]) + b*64;
				vmask = globalvmask[b];
			}
			
			CompactArrayBase(uint64_t const rb) : b(rb)
			{
				initTables();
			}
			
			uint64_t getB() const
			{
				return b;
			}
		};
	}
}
#endif
