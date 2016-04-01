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
			~HugePages()
			{
				#if defined(__linux__)
				if ( base )
				{
					munmap(base,alloc);
					base = NULL;
				}
				#endif
			}

			static HugePages & getHugePageObject()
			{
				libmaus2::parallel::ScopePosixSpinLock slock(createLock);
				if ( ! sObject )
				{
					unique_ptr_type tObject(new this_type);
					sObject = UNIQUE_PTR_MOVE(tObject);
				}
				return *sObject;
			}

			std::ostream & print(std::ostream & out)
			{
				libmaus2::parallel::ScopePosixMutex slock(lock);

				uint64_t numblocks = 0;
				MemoryBlock::shared_ptr_type pblock = getFirstBlock();

				while ( pblock )
				{
					out
						<< "MemBlock(" << reinterpret_cast<void *>(pblock.get()) << "," << pblock->p << "," << pblock->n << "," << pblock->prev << "," << pblock->next
						<< ","
						<< ((blocks.find(pblock)!=blocks.end())?"F":"")
						<< ((usedblocks.find(pblock)!=usedblocks.end())?"A":"")
						<< ")";
					pblock = pblock->next;
					numblocks += 1;
				}

				assert ( numblocks == blocks.size() + usedblocks.size() );
				assert ( blocks.size() == sizeblocks.size() );

				pblock = getFirstBlock();
				uint64_t s = 0;
				while ( pblock )
				{
					if ( pblock->prev )
					{
						assert ( pblock->prev->next );
						assert ( pblock->prev->next.get() == pblock.get() );
					}
					if ( pblock->next )
					{
						assert ( pblock->next->prev );
						assert ( pblock->next->prev.get() == pblock.get() );
					}

					s += pblock->n;

					pblock = pblock->next;
				}

				assert ( s == alloc );

				return out;
			}

			/**
			 * allocate memory, prefer huge page memory over conventional
			 **/
			void * malloc(size_t const s, size_t const align = sizeof(uint64_t))
			{
				if ( s )
				{
					void * p = hpmalloc(s,align);

					if ( p )
						return p;

					p = ::malloc(s);

					return p;
				}
				else
				{
					return NULL;
				}
			}

			static bool checkAligned(void * p, ::std::size_t align)
			{
				return ( (reinterpret_cast< ::std::size_t >(reinterpret_cast<char * >(p)) % align) == 0 );
			}

			/**
			 * allocate block of size rs in huge pages
			 **/
			void * hpmalloc(size_t const rs, size_t const align = sizeof(uint64_t))
			{
				libmaus2::parallel::ScopePosixMutex slock(lock);

				#if defined(__linux__)
				init();

				// go to next larger multiple of s
				size_t const s = rs;

				// empty block? return NULL
				if ( ! s )
					return NULL;

				// search for smallest block matching request suitable for producing alignment
				cmpblock->n = s + align - 1;
				std::multiset<MemoryBlock::shared_ptr_type>::iterator it = sizeblocks.lower_bound(cmpblock);

				// no suitable block, return NULL
				if ( it == sizeblocks.end() )
					return NULL;

				// get block
				MemoryBlock::shared_ptr_type pblock = *it;

				// this is supposed to be sufficiently large
				assert ( pblock->n >= (s + align - 1) );

				// check whether block is aligned correctly
				if ( ! checkAligned(pblock->p,align) )
				{
					// compute shift needed for alignment
					::std::size_t const shift = align - (reinterpret_cast< ::std::size_t >(reinterpret_cast<char * >(pblock->p)) % align);
					// check that shift sets alignment right
					assert ( (reinterpret_cast< ::std::size_t >(reinterpret_cast<char * >(pblock->p)+shift) % align) == 0 );

					// create new block by splitting pblock
					MemoryBlock::shared_ptr_type newblock(new MemoryBlock(reinterpret_cast<void *>(reinterpret_cast<char *>(pblock->p) + shift),pblock->n - shift));
					pblock->n = shift;

					MemoryBlock::shared_ptr_type oldprev = pblock->prev;
					MemoryBlock::shared_ptr_type oldnext = pblock->next;

					// adapt prev and next pointers in newblock and pblock
					newblock->prev = pblock;
					newblock->next = pblock->next;
					pblock->next = newblock;

					if ( oldprev )
						oldprev->next = pblock;
					if ( oldnext )
						oldnext->prev = newblock;

					// insert newly created block
					sizeblocks.insert(newblock);
					blocks.insert(newblock);

					// go to split block
					pblock = newblock;
					it = sizeblocks.find(pblock);
					assert ( it != sizeblocks.end() );
					assert ( (*it)->p == pblock->p );
				}

				// check block is aligned now
				assert ( (reinterpret_cast< ::std::size_t >(reinterpret_cast<char * >(pblock->p)) % align) == 0 );

				if ( pblock->n != s )
				{
					// create new block by splitting pblock
					MemoryBlock::shared_ptr_type newblock(new MemoryBlock(reinterpret_cast<void *>(reinterpret_cast<char *>(pblock->p) + s),pblock->n - s));
					pblock->n = s;

					MemoryBlock::shared_ptr_type oldprev = pblock->prev;
					MemoryBlock::shared_ptr_type oldnext = pblock->next;

					// adapt prev and next pointers in newblock and pblock
					newblock->prev = pblock;
					newblock->next = pblock->next;
					pblock->next = newblock;

					if ( oldprev )
						oldprev->next = pblock;
					if ( oldnext )
						oldnext->prev = newblock;

					// insert newly created block
					sizeblocks.insert(newblock);
					blocks.insert(newblock);
				}

				assert ( pblock->n == s );

				// register pblock as used
				usedblocks.insert(pblock);
				// erase pblock from free lists
				sizeblocks.erase(it);
				blocks.erase(pblock);

				return pblock->p;
				#else
				return NULL;
				#endif
			}

			/**
			 * deallocate block p
			 **/
			void free(void * p)
			{
				libmaus2::parallel::ScopePosixMutex slock(lock);

				if ( p )
				{
					cmpblock->p = p;
					std::set      < MemoryBlock::shared_ptr_type, MemoryBlockAddressComparator >::iterator it = usedblocks.find(cmpblock);

					if ( it != usedblocks.end() )
					{
						MemoryBlock::shared_ptr_type pblock = *it;
						usedblocks.erase(it);

						if ( pblock->prev && blocks.find(pblock->prev) != blocks.end() )
						{
							assert ( pblock->prev->next );
							assert ( pblock->prev->next == pblock );

							// get prev block
							MemoryBlock::shared_ptr_type prevblock = pblock->prev;
							// erase prev block from free lists
							blocks.erase ( blocks.find(prevblock) );
							sizeblocks.erase ( sizeblocks.find(prevblock) );

							assert ( (reinterpret_cast<char *>(prevblock->p) + prevblock->n) == reinterpret_cast<char *>(pblock->p) );

							prevblock->n += pblock->n;
							prevblock->next = pblock->next;

							if ( prevblock->next )
								prevblock->next->prev = prevblock;

							pblock = prevblock;
						}

						if ( pblock->next && blocks.find(pblock->next) != blocks.end() )
						{
							assert ( pblock->next->prev );
							assert ( pblock->next->prev == pblock );

							// get next block
							MemoryBlock::shared_ptr_type nextblock = pblock->next;
							// erase next block from free lists
							blocks.erase ( blocks.find(nextblock) );
							sizeblocks.erase ( sizeblocks.find(nextblock) );

							assert (
								(reinterpret_cast<char *>(pblock->p) + pblock->n) ==
								reinterpret_cast<char *>(nextblock->p)
							);

							pblock->n += nextblock->n;
							pblock->next = nextblock->next;

							if ( pblock->next )
								pblock->next->prev = pblock;
						}

						blocks.insert(pblock);
						sizeblocks.insert(pblock);
					}
					// not in used list, assume it is conventional memory
					else
					{
						::free(p);
					}
				}
			}

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

			MemoryBlock::shared_ptr_type chainStart(MemoryBlock::shared_ptr_type pblock)
			{
				MemoryBlock::shared_ptr_type ppblock;

				while ( pblock )
				{
					ppblock = pblock;
					pblock = pblock->prev;
				}

				return ppblock;

			}

			MemoryBlock::shared_ptr_type getFirstBlock()
			{
				if ( blocks.size() )
				{
					return chainStart(*(blocks.begin()));
				}
				else if ( usedblocks.size() )
				{
					return chainStart(*(usedblocks.begin()));
				}
				else
				{
					return MemoryBlock::shared_ptr_type();
				}
			}


			static uint64_t parseUnitNumber(std::string const & value)
			{
				uint64_t digend = 0;
				while ( digend < value.size() && isdigit(value[digend]) )
					++digend;

				uint64_t v;
				std::istringstream istr(value.substr(0,digend));
				istr >> v;

				if ( ! istr )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "libmaus2::util::HugePages: cannot parse " << value << " as number" << std::endl;
					lme.finish();
					throw lme;
				}

				std::string unit = clip(value.substr(digend));

				if ( ! unit.size() )
					return v;
				else if ( unit == "kB" )
					return v*1024ull;
				else if ( unit == "mB" )
					return v*1024ull*1024ull;
				else
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "libmaus2::util::HugePages: cannot parse " << value << " as number (unsupported unit)" << std::endl;
					lme.finish();
					throw lme;
				}
			}

			void init()
			{
				#if defined(__linux__)
				if ( ! setup )
				{
					if ( meminfo.find("Hugepagesize") == meminfo.end() )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "libmaus2::util::HugePages: cannot detect huge page size" << std::endl;
						lme.finish();
						throw lme;
					}
					if ( meminfo.find("HugePages_Total") == meminfo.end() )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "libmaus2::util::HugePages: cannot detect huge info" << std::endl;
						lme.finish();
						throw lme;
					}
					if ( meminfo.find("HugePages_Free") == meminfo.end() )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "libmaus2::util::HugePages: cannot detect huge info" << std::endl;
						lme.finish();
						throw lme;
					}
					if ( meminfo.find("HugePages_Rsvd") == meminfo.end() )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "libmaus2::util::HugePages: cannot detect huge info size" << std::endl;
						lme.finish();
						throw lme;
					}
					if ( meminfo.find("HugePages_Surp") == meminfo.end() )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "libmaus2::util::HugePages: cannot detect huge info" << std::endl;
						lme.finish();
						throw lme;
					}

					pagesize = parseUnitNumber(meminfo.find("Hugepagesize")->second);
					pages_total = parseUnitNumber(meminfo.find("HugePages_Total")->second);
					pages_free = parseUnitNumber(meminfo.find("HugePages_Free")->second);
					pages_rsvd = parseUnitNumber(meminfo.find("HugePages_Rsvd")->second);
					pages_surp = parseUnitNumber(meminfo.find("HugePages_Surp")->second);

					alloc = pagesize * pages_free;

					// std::cerr << pagesize << "," << pages_total << "," << pages_free << "," << pages_rsvd << "," << pages_surp << std::endl;

					if ( alloc )
					{
						base = mmap(NULL, alloc, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_HUGETLB|MAP_PRIVATE, -1 /* fd */, 0 /* offset */);

						if ( base == MAP_FAILED )
						{
							base = NULL;

							int const error = errno;

							libmaus2::exception::LibMausException lme;
							lme.getStream() << "libmaus2::util::HugePages: cannot allocate memory: " << strerror(error) << std::endl;
							lme.finish();
							throw lme;
						}

						// std::cerr << "got " << alloc << std::endl;

						assert ( pagesize % sizeof(uint64_t) == 0 );

						MemoryBlock::shared_ptr_type pblock(new MemoryBlock(base,alloc));
						blocks.insert(pblock);
						sizeblocks.insert(pblock);
					}
					else
					{
						base = NULL;
					}

					setup = true;
				}
				#else
				libmaus2::exception::LibMausException lme;
				lme.getStream() << "libmaus2::util::HugePages: huge page are supported on Linux only" << std::endl;
				lme.finish();
				throw lme;
				#endif
			}


			static std::string clip(std::string s)
			{
				uint64_t i = 0;
				while ( i < s.size() && isspace(s[i]) )
					++i;
				int64_t j = static_cast<int64_t>(s.size())-1;
				while ( j >= 0 && isspace(s[j]) )
					--j;
				j += 1;
				return s.substr(i,j-i);
			}

			static std::map<std::string,std::string> getFileMap(std::string const & fn)
			{
				libmaus2::aio::InputStreamInstance::unique_ptr_type PISI(new libmaus2::aio::InputStreamInstance(fn));
				std::map<std::string,std::string> M;

				std::string line;
				while ( *PISI )
				{
					std::getline(*PISI,line);
					if ( line.size() )
					{
						std::string::size_type const colpos = line.find(':');
						if ( colpos != std::string::npos )
						{
							std::string const key = line.substr(0,colpos);
							line = clip(line.substr(colpos+1));
							M[key] = line;
						}
					}
				}

				return M;
			}

			HugePages() : setup(false), alloc(0), base(0), cmpblock(new MemoryBlock())
			{
				#if defined(__linux__)
				meminfo = getFileMap("/proc/meminfo");
				#endif
			}
		};
	}
}
#endif
