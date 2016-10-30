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

#if ! defined(LIBMAUS2_AUTOARRAY_GENERICALIGNEDALLOCATION_HPP)
#define LIBMAUS2_AUTOARRAY_GENERICALIGNEDALLOCATION_HPP

#include <libmaus2/types/types.hpp>
#include <libmaus2/aio/StreamLock.hpp>
#include <libmaus2/util/Destructable.hpp>
#include <iostream>
#include <cstring>
#include <cstdlib>

namespace libmaus2
{
	namespace autoarray
	{
		struct GenericAlignedAllocation
		{
			static libmaus2::util::Destructable::unique_ptr_type allocateU(uint64_t const n, uint64_t const align, uint64_t const objsize)
			{
				void * p = NULL;

				try
				{
					p = allocate(n,align,objsize);
					libmaus2::util::Destructable::unique_ptr_type tptr(
						libmaus2::util::Destructable::construct(p,deallocate)
					);
					return UNIQUE_PTR_MOVE(tptr);
				}
				catch(...)
				{
					deallocate(p);
					throw;
				}
			}

			static libmaus2::util::Destructable::shared_ptr_type allocateS(uint64_t const n, uint64_t const align, uint64_t const objsize)
			{
				void * p = NULL;

				try
				{
					p = allocate(n,align,objsize);
					libmaus2::util::Destructable::shared_ptr_type tptr(
						libmaus2::util::Destructable::sconstruct(p,deallocate)
					);
					return tptr;
				}
				catch(...)
				{
					deallocate(p);
					throw;
				}
			}

			static void * allocate(uint64_t const n, uint64_t const align, uint64_t const objsize)
			{
				#if defined(LIBMAUS2_HAVE_POSIX_MEMALIGN)
				void * alignedp = NULL;
				int r = posix_memalign(reinterpret_cast<void**>(&alignedp),align,n * objsize );
				if ( r )
				{
					int const aerrno = r;
					{
					libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
					std::cerr << "posix_memalign(" << align << "," << n * objsize << ") failed: " << strerror(aerrno) << std::endl;
					}
					throw std::bad_alloc();
				}
				return alignedp;
				#else
				uint64_t const allocbytes = (sizeof(uint8_t *) - 1) + align  + n * objsize;
				uint8_t * const allocp = new uint8_t[ allocbytes ];
				uint8_t * const alloce = allocp + allocbytes;
				uint64_t const mod = reinterpret_cast<uint64_t>(allocp) % align;

				uint8_t * alignedp = mod ? (allocp + (align-mod)) : allocp;
				assert ( reinterpret_cast<uint64_t>(alignedp) % align == 0 );

				while ( (alignedp - allocp) < static_cast< ::std::ptrdiff_t > (sizeof(uint8_t *)) )
					alignedp += align;

				assert ( reinterpret_cast<uint64_t>(alignedp) % align == 0 );
				assert ( alloce - alignedp >= static_cast< ::std::ptrdiff_t >(n*objsize) );
				assert ( alignedp-allocp >= static_cast< ::std::ptrdiff_t >(sizeof(uint8_t *)) );

				(reinterpret_cast<uint8_t **>(alignedp))[-1] = allocp;

				return alignedp;
				#endif
			}

			static void deallocate(void * alignedp)
			{
				#if defined(LIBMAUS2_HAVE_POSIX_MEMALIGN)
				if ( alignedp )
					::free(alignedp);
				#else
				if ( alignedp )
					// delete [] (reinterpret_cast<uint8_t **>((alignedp)))[-1];
					delete [] (
						(uint8_t **)(alignedp)
					)[-1];
				#endif
			}
		};
	}
}
#endif
