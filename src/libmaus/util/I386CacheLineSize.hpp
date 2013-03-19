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

#if ! defined(I386CACHELINESIZE_HPP)
#define I386CACHELINESIZE_HPP

#include <libmaus/LibMausConfig.hpp>
#include <iostream>
#include <numeric>

namespace libmaus
{
	namespace util
	{
		#if defined(LIBMAUS_USE_ASSEMBLY) && defined(LIBMAUS_HAVE_i386)
		struct I386CacheLineSize
		{
			static void cpuid(
				uint32_t & eax,
				uint32_t & ebx,
				uint32_t & ecx,
				uint32_t & edx
			);

			static unsigned int getCacheLineSizeSingle(unsigned int const val)
			{
				switch ( val )
				{
					case 0x0a:
						return 32;
					case 0x0c:
						return 32;
					case 0x0d:
					case 0x0e:
					case 0x2c:
					case 0x60:
					case 0x66:
					case 0x67:
					case 0x68:
						return 64;
					default:
						return 0;
				}
			}

			static unsigned int getCacheLineSize(unsigned int const reg)
			{
				unsigned int linesize = 0;

				std::cerr << std::hex << reg << std::dec << std::endl;

				if ( (reg & (1u << 31)) == 0 )
				{
					linesize = std::max(linesize,getCacheLineSizeSingle((reg >>24)&0xFF));
					linesize = std::max(linesize,getCacheLineSizeSingle((reg >>16)&0xFF));
					linesize = std::max(linesize,getCacheLineSizeSingle((reg >> 8)&0xFF));
					linesize = std::max(linesize,getCacheLineSizeSingle((reg >> 0)&0xFF));
				}

				return linesize;
			}

			static unsigned int getCacheLineSize()
			{
				uint32_t eax, ebx, ecx, edx;

				eax = 0;
				ebx = 0;
				ecx = 0;
				edx = 0;
				cpuid(eax,ebx,ecx,edx);
				unsigned int cachelinesize = 0;

				if ( cachelinesize == 0 && 4 <= eax )
				{
					unsigned int nextcnt = 0;
					while ( true )
					{
						eax = 4;
						ecx = nextcnt;
						cpuid(eax,ebx,ecx,edx);

						if ( eax & 0x1F )
						{
							unsigned int const type = eax & 0x1F;
							if ( type == 1 || type == 3) // data or univeral cache
							{
								unsigned int const level = ((eax >> 5) & 7); // level
								if ( level == 1 )
								{
									unsigned int const linesize = (ebx & (((1u<<11)-1)))+1;
									#if 0
									std::cerr << "type " << type << std::endl;
									std::cerr << "level " << level << std::endl;
									std::cerr << "system coherence line size: " << linesize << std::endl;
									#endif
									cachelinesize = linesize;
								}
							}
						}
						else
						{
							break;
						}

						nextcnt++;
					}		
				}
				if ( cachelinesize == 0 && 2 <= eax )
				{
					eax = 2;
					cpuid(eax,ebx,ecx,edx);

					cachelinesize = std::max(cachelinesize,getCacheLineSize(eax));
					cachelinesize = std::max(cachelinesize,getCacheLineSize(ebx));
					cachelinesize = std::max(cachelinesize,getCacheLineSize(ecx));
					cachelinesize = std::max(cachelinesize,getCacheLineSize(edx));
				}

				return cachelinesize;
			}
		};
		#endif
	}
}
#endif
