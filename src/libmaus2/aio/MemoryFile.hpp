/*
    libmaus2
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2015 Genome Research Limited

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
#if ! defined(LIBMAUS_AIO_MEMORYFILE_HPP)
#define LIBMAUS_AIO_MEMORYFILE_HPP

#include <libmaus2/autoarray/AutoArray.hpp>

namespace libmaus2
{
	namespace aio
	{
		struct MemoryFile
		{
			typedef MemoryFile this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			
			#if defined(MEMORY_FILE_SINGLE_BLOCK)
			typedef libmaus2::autoarray::AutoArray<char,libmaus2::autoarray::alloc_type_c> array_type;
			typedef array_type::shared_ptr_type array_ptr_type;
			
			array_ptr_type A;
	
			MemoryFile() : A(new array_type(0)), f(0)
			{
			
			}
			
			void truncatep()
			{
				A->resize(0);
				f = 0;
			}
			
			ssize_t writep(uint64_t p, char const * c, uint64_t n)
			{
				while ( p + n > A->size() )
				{
					uint64_t oldsize = A->size();
					uint64_t newsize = std::max(oldsize + 1024, (oldsize * 17)/16 );
					try
					{
						A->resize(newsize);
					}
					catch(...)
					{
						errno = ENOMEM;
						return -1;
					}
				}
				
				memcpy(A->begin() + p, c, n);
				
				if ( p+n > f )
					f = p+n;
					
				return n;
			}
			
			ssize_t readp(uint64_t p, char * c, uint64_t n)
			{
				if ( p > f )
					return -1;
				
				uint64_t const av = f-p;
				uint64_t const r = std::min(n,av);
				
				memcpy(c,A->begin()+p,r);
				
				return r;
			}
			#else
			typedef libmaus2::autoarray::AutoArray<char> block_type;
			typedef block_type::shared_ptr_type block_ptr_type;
			std::vector<block_ptr_type> blocks;
			
			static uint64_t getPrimitiveBlockSize()
			{
				return 4096;
			}
			
			static uint64_t getMaxBlocks()
			{
				return 64;
			}
			
			static uint64_t getMaxBlockSize()
			{
				return 256*1024;
			}
			
			MemoryFile() : blocks(), f(0)
			{
			}
			
			void truncatep()
			{
				f = 0;
				blocks.resize(0);
			}
			
			uint64_t capacity() const
			{
				if ( blocks.size() )
					return blocks.size() * blocks[0]->size();
				else
					return 0;
			}
			
			void addBlock()
			{
				if ( ! blocks.size() )
				{
					block_ptr_type ptr(new block_type(getPrimitiveBlockSize(),false));
					blocks.push_back(ptr);
				}
				else
				{
					if ( blocks.size() >= getMaxBlocks() && blocks[0]->size() < getMaxBlockSize() )
					{
						assert ( blocks.size() == getMaxBlocks() );
						assert ( getMaxBlocks() % 2 == 0 );
						
						uint64_t const oldblocksize = blocks[0]->size();
						uint64_t const newblocksize = oldblocksize << 1;
												
						for ( std::vector<block_ptr_type>::size_type i = 0; i < getMaxBlocks()/2; ++i )
						{
							block_ptr_type ptr(new block_type(newblocksize,false));
							std::copy(blocks[2*i+0]->begin(),blocks[2*i+0]->end(),ptr->begin());
							std::copy(blocks[2*i+1]->begin(),blocks[2*i+1]->end(),ptr->begin()+oldblocksize);
							blocks[2*i+0] = block_ptr_type();
							blocks[2*i+1] = block_ptr_type();
							blocks[i] = ptr;
						}
						
						blocks.resize(getMaxBlocks()/2);		
					}

					block_ptr_type ptr(new block_type(blocks[0]->size(),false));
					blocks.push_back(ptr);				
				}
			}

			ssize_t writep(uint64_t p, char const * c, uint64_t n)
			{
				while ( p+n > capacity() )
				{
					try
					{
						addBlock();
					}
					catch(...)
					{
						errno = ENOMEM;
						return -1;
					}
				}
				
				if ( p + n > f )
					f = p+n;
				
				ssize_t w = 0;
				assert ( blocks.size() );
				uint64_t const blocksize = blocks[0]->size();
				
				while ( n )
				{
					uint64_t const blockid = p / blocksize;
					uint64_t const blocklow = blockid * blocksize;
					uint64_t const blockhigh = blocklow + blocksize;
					assert ( blockhigh > p );
					uint64_t const tocopy = std::min(n, blockhigh - p);
					memcpy(
						blocks[blockid]->begin()+(p-blocklow),
						c,tocopy
					);
					
					p += tocopy;
					c += tocopy;
					n -= tocopy;
					w += tocopy;
				}
			
				return w;
			}
			ssize_t readp(uint64_t p, char * c, uint64_t n)
			{
				if ( p > f )
					return -1;
					
				ssize_t r = 0;
				uint64_t const blocksize = blocks[0]->size();
				
				uint64_t const av = f-p;
				n = std::min(n,av);
				
				while ( n )
				{
					uint64_t const blockid = p / blocksize;
					uint64_t const blocklow = blockid * blocksize;
					uint64_t const blockhigh = blocklow + blocksize;
					assert ( blockhigh > p );
					uint64_t const tocopy = std::min(n, blockhigh - p);
					
					assert ( blockid < blocks.size() );

					memcpy(
						c,
						blocks[blockid]->begin() + (p-blocklow),
						tocopy
					);				

					p += tocopy;
					c += tocopy;
					n -= tocopy;
					r += tocopy;
				}
			
				return r;
			}
			#endif

			uint64_t f;

			uint64_t size() const
			{
				return f;
			}

		};
	}
}
#endif
