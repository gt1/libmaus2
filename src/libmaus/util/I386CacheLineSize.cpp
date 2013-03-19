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

#include <libmaus/util/I386CacheLineSize.hpp>

#if defined(LIBMAUS_USE_ASSEMBLY)
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
#endif
