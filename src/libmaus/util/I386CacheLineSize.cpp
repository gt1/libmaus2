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

#include <libmaus/util/I386CacheLineSize.hpp>

#if defined(LIBMAUS_USE_ASSEMBLY) && defined(LIBMAUS_HAVE_i386)
void libmaus::util::I386CacheLineSize::cpuid(
	uint32_t & eax,
	uint32_t & ebx,
	uint32_t & ecx,
	uint32_t & edx
)
{
	#if defined(LIBMAUS_HAVE_x86_64)
	typedef uint64_t regsave_type;
	#else
	typedef uint32_t regsave_type;
	#endif

	regsave_type * regsave = 0;
	
	try
	{
		regsave = new regsave_type[9];

		regsave[0] = eax;
		regsave[1] = ebx;
		regsave[2] = ecx;
		regsave[3] = edx;
		
		#if defined(LIBMAUS_HAVE_x86_64)
		asm volatile(
			"mov %%rax,(4*8)(%0)\n"
			"mov %%rbx,(5*8)(%0)\n"
			"mov %%rcx,(6*8)(%0)\n"
			"mov %%rdx,(7*8)(%0)\n"
			"mov %%rsi,(8*8)(%0)\n"
			"mov %0,%%rsi\n"

			"mov (0*8)(%%rsi),%%rax\n"
			"mov (1*8)(%%rsi),%%rbx\n"
			"mov (2*8)(%%rsi),%%rcx\n"
			"mov (3*8)(%%rsi),%%rdx\n"
			
			"cpuid\n"
			
			"mov %%rax,(0*8)(%%rsi)\n"
			"mov %%rbx,(1*8)(%%rsi)\n"
			"mov %%rcx,(2*8)(%%rsi)\n"
			"mov %%rdx,(3*8)(%%rsi)\n"

			"mov %%rsi,%0\n"
			
			"mov (4*8)(%0),%%rax\n"
			"mov (5*8)(%0),%%rbx\n"
			"mov (6*8)(%0),%%rcx\n"
			"mov (7*8)(%0),%%rdx\n"
			"mov (8*8)(%0),%%rsi\n"
			: : "a" (&regsave[0]) : "memory" ); 
		#else
		asm volatile(
			"mov %%eax,(4*4)(%0)\n"
			"mov %%ebx,(5*4)(%0)\n"
			"mov %%ecx,(6*4)(%0)\n"
			"mov %%edx,(7*4)(%0)\n"
			"mov %%esi,(8*4)(%0)\n"
			"mov %0,%%esi\n"

			"mov (0*4)(%%esi),%%eax\n"
			"mov (1*4)(%%esi),%%ebx\n"
			"mov (2*4)(%%esi),%%ecx\n"
			"mov (3*4)(%%esi),%%edx\n"
			
			"cpuid\n"
			
			"mov %%eax,(0*4)(%%esi)\n"
			"mov %%ebx,(1*4)(%%esi)\n"
			"mov %%ecx,(2*4)(%%esi)\n"
			"mov %%edx,(3*4)(%%esi)\n"

			"mov %%esi,%0\n"
			
			"mov (4*4)(%0),%%eax\n"
			"mov (5*4)(%0),%%ebx\n"
			"mov (6*4)(%0),%%ecx\n"
			"mov (7*4)(%0),%%edx\n"
			"mov (8*4)(%0),%%esi\n"
			: : "a" (&regsave[0]) : "memory" ); 
		#endif
		
		eax = regsave[0];
		ebx = regsave[1];
		ecx = regsave[2];
		edx = regsave[3];
		
		// std::cerr << "eax=" << eax << " ebx=" << ebx << " ecx=" << ecx << " edx=" << edx << std::endl;
		
		delete [] regsave;
		regsave = 0;
	}
	catch(...)
	{
		delete [] regsave;
		regsave = 0;
		throw;
	}
}

uint64_t libmaus::util::I386CacheLineSize::xgetbv(uint32_t const index)
{
	uint32_t eax, edx;
	asm volatile("xgetbv" : "=a" (eax), "=d" (edx) : "c" (index));
	return eax + (static_cast<uint64_t>(edx) << 32);
}

unsigned int libmaus::util::I386CacheLineSize::getCacheLineSizeSingle(unsigned int const val)
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

unsigned int libmaus::util::I386CacheLineSize::getCacheLineSize(unsigned int const reg)
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

unsigned int libmaus::util::I386CacheLineSize::getCacheLineSize()
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

/**
 * @return true if CPU supports SSE
 **/
bool libmaus::util::I386CacheLineSize::hasSSE()
{
	uint32_t eax, ebx, ecx, edx;

	eax = 0;
	ebx = 0;
	ecx = 0;
	edx = 0;
	cpuid(eax,ebx,ecx,edx);
	
	if ( 1 > eax )
		return false;

	eax = 1;
	ebx = 0;
	ecx = 0;
	edx = 0;
	cpuid(eax,ebx,ecx,edx);

	return ((edx>>25)&1) == 1;
}
/**
 * @return true if CPU supports SSE2
 **/
bool libmaus::util::I386CacheLineSize::hasSSE2()
{
	uint32_t eax, ebx, ecx, edx;

	eax = 0;
	ebx = 0;
	ecx = 0;
	edx = 0;
	cpuid(eax,ebx,ecx,edx);
	
	if ( 1 > eax )
		return false;

	eax = 1;
	ebx = 0;
	ecx = 0;
	edx = 0;
	cpuid(eax,ebx,ecx,edx);

	return ((edx>>26)&1) == 1;
}
/**
 * @return true if CPU supports SSE3
 **/
bool libmaus::util::I386CacheLineSize::hasSSE3()
{
	uint32_t eax, ebx, ecx, edx;

	eax = 0;
	ebx = 0;
	ecx = 0;
	edx = 0;
	cpuid(eax,ebx,ecx,edx);
	
	if ( 1 > eax )
		return false;

	eax = 1;
	ebx = 0;
	ecx = 0;
	edx = 0;
	cpuid(eax,ebx,ecx,edx);

	return ((ecx>>0)&1) == 1;
}
/**
 * @return true if CPU supports SSSE3
 **/
bool libmaus::util::I386CacheLineSize::hasSSSE3()
{
	uint32_t eax, ebx, ecx, edx;

	eax = 0;
	ebx = 0;
	ecx = 0;
	edx = 0;
	cpuid(eax,ebx,ecx,edx);
	
	if ( 1 > eax )
		return false;

	eax = 1;
	ebx = 0;
	ecx = 0;
	edx = 0;
	cpuid(eax,ebx,ecx,edx);

	return ((ecx>>9)&1) == 1;
}
/**
 * @return true if CPU supports SSE4.1
 **/
bool libmaus::util::I386CacheLineSize::hasSSE41()
{
	uint32_t eax, ebx, ecx, edx;

	eax = 0;
	ebx = 0;
	ecx = 0;
	edx = 0;
	cpuid(eax,ebx,ecx,edx);
	
	if ( 1 > eax )
		return false;

	eax = 1;
	ebx = 0;
	ecx = 0;
	edx = 0;
	cpuid(eax,ebx,ecx,edx);

	return ((ecx>>19)&1) == 1;
}
/**
 * @return true if CPU supports SSE4.2
 **/
bool libmaus::util::I386CacheLineSize::hasSSE42()
{
	uint32_t eax, ebx, ecx, edx;

	eax = 0;
	ebx = 0;
	ecx = 0;
	edx = 0;
	cpuid(eax,ebx,ecx,edx);
	
	if ( 1 > eax )
		return false;

	eax = 1;
	ebx = 0;
	ecx = 0;
	edx = 0;
	cpuid(eax,ebx,ecx,edx);

	return ((ecx>>20)&1) == 1;
}
/**
 * @return true if CPU supports popcnt
 **/
bool libmaus::util::I386CacheLineSize::hasPopCnt()
{
	uint32_t eax, ebx, ecx, edx;

	eax = 0;
	ebx = 0;
	ecx = 0;
	edx = 0;
	cpuid(eax,ebx,ecx,edx);
	
	if ( 1 > eax )
		return false;

	eax = 1;
	ebx = 0;
	ecx = 0;
	edx = 0;
	cpuid(eax,ebx,ecx,edx);

	return ((ecx>>23)&1) == 1;
}
/**
 * @return true if CPU supports avx
 **/
bool libmaus::util::I386CacheLineSize::hasAVX()
{
	uint32_t eax, ebx, ecx, edx;

	eax = 0;
	ebx = 0;
	ecx = 0;
	edx = 0;
	cpuid(eax,ebx,ecx,edx);
	
	if ( 1 > eax )
		return false;

	eax = 1;
	ebx = 0;
	ecx = 0;
	edx = 0;
	cpuid(eax,ebx,ecx,edx);

	// check for XSAVE/XRSTOR and AVX bits
	if ( (ecx & 0x018000000ul) != 0x018000000ul )
		return false;

	// check state saving for xmm and ymm		
	uint64_t const xbv = xgetbv(0);
		
	if ( (xbv & 0x6) != 0x6 )
		return false;

	return true;
}
#endif
