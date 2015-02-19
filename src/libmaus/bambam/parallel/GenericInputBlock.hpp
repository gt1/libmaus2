/*
    libmaus
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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_GENERICINPUTBLOCK_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_GENERICINPUTBLOCK_HPP

#include <libmaus/util/unique_ptr.hpp>
#include <libmaus/util/shared_ptr.hpp>
#include <utility>
#include <vector>
#include <libmaus/parallel/LockedCounter.hpp>

namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{
			struct GenericInputBlockSubBlockInfo
			{
				typedef GenericInputBlockSubBlockInfo this_type;
				typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
				
				std::vector< std::pair<uint8_t *,uint8_t *> > blocks;
				libmaus::parallel::LockedCounter returnedBlocks;
				uint64_t streamid;
				uint64_t blockid;
				bool eof;
				
				GenericInputBlockSubBlockInfo() : returnedBlocks(0), streamid(0), blockid(0), eof(false)
				{
				
				}

				void addBlock(std::pair<uint8_t *,uint8_t *> const & P)
				{
					blocks.push_back(P);
				}
				
				bool returnBlock()
				{
					return returnedBlocks.increment() == blocks.size();
				}

				void reset()
				{
					blocks.resize(0);
					returnedBlocks -= static_cast<uint64_t>(returnedBlocks);
				}
			};

			struct GenericInputBlockFillResult
			{
				typedef GenericInputBlockFillResult this_type;
				
				bool empty;
				bool eof;
				uint64_t gcount;
				
				GenericInputBlockFillResult(bool const rempty = true, bool const reof = true, uint64_t const rgcount = 0)
				: empty(rempty), eof(reof), gcount(rgcount)
				{

				}
			};
					
			template<typename _meta_info_type>
			struct GenericInputBlock
			{
				typedef _meta_info_type meta_info_type;
				typedef GenericInputBlock<meta_info_type> this_type;
				typedef typename libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef typename libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
				
				libmaus::autoarray::AutoArray<uint8_t,libmaus::autoarray::alloc_type_c> A;
				uint8_t * pa;
				uint8_t * pc;
				uint8_t * pe;
				meta_info_type meta;
												
				GenericInputBlock(
					uint64_t const rsize
				)
				: A(rsize,false), pa(A.begin()), pc(pa), pe(pc)
				{
				
				}
				
				void extend(uint64_t const extendc = 9, uint64_t const extendd = 8)
				{
					ptrdiff_t const coff = pc-pa;
					ptrdiff_t const eoff = pe-pa;
				
					uint64_t const newsize = 
						A.size() ? 
						((A.size() * extendc + extendd-1)/extendd) 
						: 1;
					assert ( extendc > extendd );
					assert ( newsize > A.size() );
					A.resize(newsize);
					
					pa = A.begin();
					pc = pa + coff;
					pe = pa + eoff;
				}
				
				void reset()
				{
					pc = pa;
					pe = pa;
					meta.reset();
				}
				
				template<typename iterator>
				void insert(iterator a, iterator e)
				{
					// required space
					assert ( e >= a );
					uint64_t const req = e-a;
					// free space
					assert ( A.end() >= pe );
					uint64_t bfree = A.end() - pe;
					
					// req more than we currently have?
					if ( req > bfree )
					{
						ptrdiff_t const dc = pc-pa;
						ptrdiff_t const de = pe-pa;
						A.resize(A.size() + (req-bfree) );
						pa = A.begin();
						pc = A.begin() + dc;
						pe = A.begin() + de;
					}
					
					// new free space	
					bfree = A.end() - pe;
					assert ( bfree >= req );
										
					std::copy(a,e,pe);
					pe += req;
				}
				
				uint64_t extractRest(libmaus::autoarray::AutoArray<uint8_t,libmaus::autoarray::alloc_type_c> & R)
				{
					// number of bytes in use
					uint64_t const bused = pe - pc;

					if ( bused > R.size() )
						R.resize(bused);
						
					std::copy(pc,pe,R.begin());
					
					return bused;
				}
				
				bool full() const
				{
					return pe == A.end();
				}
				
				
				template<typename stream_type>
				GenericInputBlockFillResult fill(stream_type & stream, bool const finite = false, uint64_t const maxread = 0)
				{
					// number of bytes in use
					uint64_t const bused = pe - pc;
										
					// move bytes in use to front of buffer
					if ( bused )
						::std::memmove(pa,pc,bused);

					// move current pointer to front
					pc = pa;
					// set end pointer
					pe = pc + bused;

					// number of bytes free in buffer
					uint64_t const bfree = A.end() - pe;
					
					uint64_t const readsize = finite ? std::min(bfree,maxread) : bfree;
					
					// read
					stream.read(reinterpret_cast<char *>(pe),readsize);
					
					// update pointer
					pe += stream.gcount();
					
					return GenericInputBlockFillResult(
						pe == pa /* empty */, 
						(stream.gcount() == 0) || (stream.peek() == std::ios::traits_type::eof())/* eof */, 
						stream.gcount()
					);
				}
			};

			template<typename _meta_info_type>
			struct GenericInputBlockAllocator
			{
				typedef _meta_info_type meta_info_type;
				typedef GenericInputBlockAllocator<meta_info_type> this_type;
				typedef typename libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef typename libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
				uint64_t blocksize;
				
				GenericInputBlockAllocator(uint64_t rblocksize = 0) : blocksize(rblocksize) {}
				
				typename GenericInputBlock<meta_info_type>::shared_ptr_type operator()()
				{
					typename GenericInputBlock<meta_info_type>::shared_ptr_type ptr(
						new GenericInputBlock<meta_info_type>(blocksize)
					);
					return ptr;
				}
			};

			template<typename _meta_data_type>
			struct GenericInputBlockTypeInfo
			{
				typedef _meta_data_type meta_data_type;
				typedef GenericInputBlock<meta_data_type> element_type;
				typedef typename element_type::shared_ptr_type pointer_type;			
				
				static pointer_type deallocate(pointer_type /*p*/)
				{
					pointer_type ptr = getNullPointer();
					return ptr;
				}
				
				static pointer_type getNullPointer()
				{
					pointer_type ptr;
					return ptr;
				}
			};
		}
	}
}
#endif
