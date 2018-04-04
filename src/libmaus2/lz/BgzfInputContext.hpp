/*
    libmaus2
    Copyright (C) 2018 German Tischler-Höhle

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
#if !defined(LIBMAUS2_LZ_BGZFINPUTCONTEXT_HPP)
#define LIBMAUS2_LZ_BGZFINPUTCONTEXT_HPP

#include <libmaus2/parallel/SimpleThreadPool.hpp>
#include <libmaus2/lz/BgzfInflateDecompressPackageDispatcher.hpp>
#include <libmaus2/parallel/SynchronousConditionQueue.hpp>
#include <libmaus2/util/FiniteSizeHeap.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct BgzfInputContext
			: public libmaus2::lz::BgzfInflateReadPackageDispatcher::BgzfInflateReadPackageFinished,
			  public libmaus2::lz::BgzfInflateReadPackageDispatcher::BgzfInflateReadPackageReturn,
			  public libmaus2::lz::BgzfInflateDecompressPackageDispatcher::BgzfInflateDecompressPackageReturn,
			  public libmaus2::lz::BgzfInflateDecompressPackageDispatcher::BgzfInflateDecompressPackageFinished
		{
			typedef BgzfInputContext this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			libmaus2::parallel::SimpleThreadPool & STP;

			libmaus2::aio::InputStreamObjectAllocator ialloc;
			libmaus2::parallel::LockedFreeList<
				libmaus2::aio::InputStreamObject::shared_ptr_type,
				libmaus2::aio::InputStreamObjectAllocator,
				libmaus2::aio::InputStreamObjectTypeInfo
			> ifreelist;

			libmaus2::parallel::LockedFreeList<
				libmaus2::lz::BgzfInflateBase::shared_ptr_type,
				libmaus2::lz::BgzfInflateBaseAllocator,
				libmaus2::lz::BgzfInflateBaseTypeInfo
			> zfreelist;

			libmaus2::util::AutoArrayCharBaseAllocator aalloc;
			libmaus2::parallel::LockedGrowingFreeList<
				libmaus2::autoarray::AutoArray<char>::shared_ptr_type,
				libmaus2::util::AutoArrayCharBaseAllocator,
				libmaus2::util::AutoArrayCharBaseTypeInfo
			> afreelist;

			libmaus2::parallel::LockedGrowingFreeList<libmaus2::lz::BgzfInflateReadPackageDispatcher::BgzfInflateReadPackage> readPackageFreeList;
			libmaus2::parallel::LockedGrowingFreeList<libmaus2::lz::BgzfInflateDecompressPackageDispatcher::BgzfInflateDecompressPackage> decompressPackageFreeList;

			bool idle()
			{
				return
					readPackageFreeList.full()
					&&
					decompressPackageFreeList.full();
			}

			uint64_t const readdispatcherid;
			uint64_t const decompressdispatcherid;

			libmaus2::lz::BgzfInflateReadPackageDispatcher birpd;
			libmaus2::lz::BgzfInflateDecompressPackageDispatcher bidpd;

			libmaus2::parallel::LockedBool eofflag;

			uint64_t volatile readc;
			libmaus2::parallel::PosixSpinLock readclock;

			libmaus2::util::FiniteSizeHeap<libmaus2::lz::BgzfInflateReadPackageDispatcher::BgzfInflateReadPackageResult> blockheap;
			uint64_t volatile nextblock;
			libmaus2::parallel::PosixSpinLock nextblocklock;

			libmaus2::parallel::SynchronousConditionQueue<libmaus2::lz::BgzfInflateReadPackageDispatcher::BgzfInflateReadPackageResult> outq;

			BgzfInputContext(
				std::istream & in,
				libmaus2::parallel::SimpleThreadPool & rSTP,
				uint64_t const inputmem = 64*1024*1024
			) :
			  STP(rSTP),
			  ialloc(&in,0),
			  ifreelist(1,ialloc),
			  zfreelist((inputmem + libmaus2::lz::BgzfConstants::getBgzfMaxBlockSize()-1) / libmaus2::lz::BgzfConstants::getBgzfMaxBlockSize()),
			  aalloc(libmaus2::lz::BgzfConstants::getBgzfMaxBlockSize()),
			  afreelist(aalloc),
			  readdispatcherid(STP.getNextDispatcherId()),
			  decompressdispatcherid(STP.getNextDispatcherId()),
			  birpd(&ifreelist,&zfreelist,&afreelist,this,this),
			  bidpd(this,this),
			  eofflag(false),
			  readc(0),
			  readclock(),
			  blockheap(zfreelist.capacity()),
			  nextblock(0),
			  nextblocklock()
			{
				assert ( zfreelist.capacity() );
				STP.registerDispatcher(readdispatcherid,&birpd);
				STP.registerDispatcher(decompressdispatcherid,&bidpd);
			}

			void init()
			{
				libmaus2::lz::BgzfInflateReadPackageDispatcher::BgzfInflateReadPackage * pn = readPackageFreeList.get();
				*pn = libmaus2::lz::BgzfInflateReadPackageDispatcher::BgzfInflateReadPackage(0/*prio*/,readdispatcherid);
				STP.enque(pn);
			}

			void bgzfInflateDecompressPackageReturn(libmaus2::lz::BgzfInflateDecompressPackageDispatcher::BgzfInflateDecompressPackage * package)
			{
				decompressPackageFreeList.put(package);
			}

			void bgzfInflateReadPackageReturn(libmaus2::lz::BgzfInflateReadPackageDispatcher::BgzfInflateReadPackage * p)
			{
				readPackageFreeList.put(p);
			}

			void returnBlock(libmaus2::lz::BgzfInflateReadPackageDispatcher::BgzfInflateReadPackageResult next)
			{
				afreelist.put(next.ablock);
				zfreelist.putAndCount(next.block);

				if ( ! eofflag.get() )
				{
					bool submit = false;
					uint64_t const cap = zfreelist.capacity()/4;

					readclock.lock();
					readc += 1;
					if ( readc == cap )
					{
						submit = true;
						readc = 0;
					}
					readclock.unlock();

					if ( submit )
					{
						libmaus2::lz::BgzfInflateReadPackageDispatcher::BgzfInflateReadPackage * pn = readPackageFreeList.get();
						*pn = libmaus2::lz::BgzfInflateReadPackageDispatcher::BgzfInflateReadPackage(0/*prio*/,readdispatcherid);
						STP.enque(pn);
					}
				}
			}

			void bgzfInflateDecompressPackageFinished(libmaus2::lz::BgzfInflateReadPackageDispatcher::BgzfInflateReadPackageResult ptr)
			{
				libmaus2::parallel::ScopePosixSpinLock slock(nextblocklock);
				blockheap.push(ptr);

				while ( !blockheap.empty() && blockheap.top().blockid == nextblock )
				{
					libmaus2::lz::BgzfInflateReadPackageDispatcher::BgzfInflateReadPackageResult next = blockheap.pop();

					if ( next.eof )
						eofflag.set(true);

					outq.enque(next);

					nextblock += 1;
				}
			}

			void bgzfInflateReadPackageFinished(libmaus2::lz::BgzfInflateReadPackageDispatcher::BgzfInflateReadPackageResult ptr)
			{
				libmaus2::lz::BgzfInflateDecompressPackageDispatcher::BgzfInflateDecompressPackage * pack = decompressPackageFreeList.get();
				*pack = libmaus2::lz::BgzfInflateDecompressPackageDispatcher::BgzfInflateDecompressPackage(ptr,0 /* prio */,decompressdispatcherid);
				STP.enque(pack);
			}
		};
	}
}
#endif
