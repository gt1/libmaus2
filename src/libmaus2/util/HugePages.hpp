/*
    libmaus2
    Copyright (C) 2016 German Tischler

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
#if ! defined(LIBMAUS2_UTIL_HUGEPAGES_HPP)
#define LIBMAUS2_UTIL_HUGEPAGES_HPP

#include <libmaus2/exception/LibMausException.hpp>
#include <libmaus2/aio/InputStreamInstance.hpp>
#include <libmaus2/parallel/PosixMutex.hpp>
#include <libmaus2/parallel/PosixSpinLock.hpp>
#include <cctype>
#include <map>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <set>
#include <limits>

#if defined(__linux__)
#include <sys/mman.h>
#endif

#include <cstdlib>

namespace libmaus2
{
	namespace util
	{
		struct HugePages
		{
			typedef HugePages this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			private:
			static libmaus2::parallel::PosixSpinLock createLock;
			static unique_ptr_type sObject;

			public:
			~HugePages();

			static HugePages & getHugePageObject();

			void check();
			std::ostream & print(std::ostream & out);

			/**
			 * allocate memory, prefer huge page memory over conventional
			 **/
			void * malloc(size_t const s, size_t const align = sizeof(uint64_t));

			static bool checkAligned(void * p, ::std::size_t align);

			/**
			 * allocate block of size rs in huge pages
			 **/
			void * hpmalloc(size_t const rs, size_t const align = sizeof(uint64_t));

			/**
			 * deallocate block p
			 **/
			void free(void * p);

			private:
			std::map<std::string,std::string> meminfo;
			bool setup;

			uint64_t pagesize;
			uint64_t pages_total;
			uint64_t pages_free;
			uint64_t pages_rsvd;
			uint64_t pages_surp;

			uint64_t alloc;
			void * base;

			libmaus2::parallel::PosixMutex lock;

			struct MemoryBlock
			{
				typedef MemoryBlock this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				void * p;
				MemoryBlock::shared_ptr_type next;
				MemoryBlock::shared_ptr_type prev;
				::std::size_t n;

				MemoryBlock(void * rp = NULL, ::std::size_t rn = 0) : p(rp), next(), prev(), n(rn)
				{

				}
			};

			struct MemoryBlockAddressComparator
			{
				bool operator()(MemoryBlock::shared_ptr_type const & A, MemoryBlock::shared_ptr_type const & B) const
				{
					return A->p < B->p;
				}
			};

			struct MemoryBlockSizeComparator
			{
				bool operator()(MemoryBlock::shared_ptr_type const & A, MemoryBlock::shared_ptr_type const & B) const
				{
					if ( A->n != B->n )
						return A->n < B->n;
					else
						return A->p < B->p;
				}
			};

			// free blocks ordered by pointer
			std::set      < MemoryBlock::shared_ptr_type, MemoryBlockAddressComparator > blocks;
			// free blocks ordered by size
			std::multiset < MemoryBlock::shared_ptr_type, MemoryBlockSizeComparator > sizeblocks;
			// block for comparison operations
			MemoryBlock::shared_ptr_type cmpblock;
			// used blocks ordered by pointer
			std::set      < MemoryBlock::shared_ptr_type, MemoryBlockAddressComparator > usedblocks;

			MemoryBlock::shared_ptr_type chainStart(MemoryBlock::shared_ptr_type pblock);
			MemoryBlock::shared_ptr_type getFirstBlock();
			static uint64_t parseUnitNumber(std::string const & value);
			void init();
			static std::string clip(std::string s);
			static std::map<std::string,std::string> getFileMap(std::string const & fn);
			HugePages();
		};
	}
}
#endif
