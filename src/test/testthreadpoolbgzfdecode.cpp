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
#include <libmaus2/lz/BgzfInflateBase.hpp>
#include <libmaus2/parallel/LockedFreeList.hpp>
#include <libmaus2/util/FiniteSizeHeap.hpp>
#include <libmaus2/parallel/SynchronousConditionQueue.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct BgzfInflateBaseAllocator
		{
			BgzfInflateBaseAllocator() {}

			libmaus2::lz::BgzfInflateBase::shared_ptr_type operator()()
			{
				libmaus2::lz::BgzfInflateBase::shared_ptr_type tptr(new libmaus2::lz::BgzfInflateBase);
				return tptr;
			}
		};
	}
}

namespace libmaus2
{
	namespace lz
	{
		struct BgzfInflateBaseTypeInfo
		{
			typedef BgzfInflateBaseTypeInfo this_type;

			typedef libmaus2::lz::BgzfInflateBase::shared_ptr_type pointer_type;

			static pointer_type getNullPointer()
			{
				pointer_type p;
				return p;
			}

			static pointer_type deallocate(pointer_type /* p */)
			{
				return getNullPointer();
			}
		};
	}
}

namespace libmaus2
{
	namespace util
	{
		struct AutoArrayCharBaseAllocator
		{
			uint64_t size;
			AutoArrayCharBaseAllocator(uint64_t rsize = libmaus2::lz::BgzfConstants::getBgzfMaxBlockSize()) : size(rsize) {}

			libmaus2::autoarray::AutoArray<char>::shared_ptr_type operator()()
			{
				libmaus2::autoarray::AutoArray<char>::shared_ptr_type tptr(new libmaus2::autoarray::AutoArray<char>(size,false));
				return tptr;
			}
		};
	}
}

namespace libmaus2
{
	namespace util
	{
		struct AutoArrayCharBaseTypeInfo
		{
			typedef AutoArrayCharBaseTypeInfo this_type;

			typedef libmaus2::autoarray::AutoArray<char>::shared_ptr_type pointer_type;

			static pointer_type getNullPointer()
			{
				pointer_type p;
				return p;
			}

			static pointer_type deallocate(pointer_type /* p */)
			{
				return getNullPointer();
			}
		};
	}
}

#include <libmaus2/parallel/SimpleThreadWorkPackage.hpp>
#include <libmaus2/parallel/SimpleThreadPool.hpp>
#include <libmaus2/parallel/LockedGrowingFreeList.hpp>

struct InputStreamObject
{
	typedef InputStreamObject this_type;
	typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
	typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

	std::istream * istr;
	uint64_t streamid;
	uint64_t volatile blockid;
	bool volatile eof;

	InputStreamObject(
		std::istream * ristr,
		uint64_t const rstreamid,
		uint64_t const rblockid = 0
	) : istr(ristr), streamid(rstreamid), blockid(rblockid), eof(false) {}
};

struct InputStreamObjectAllocator
{
	std::istream * istr;
	uint64_t streamid;

	InputStreamObjectAllocator() {}
	InputStreamObjectAllocator(std::istream * ristr, uint64_t const rstreamid) : istr(ristr), streamid(rstreamid) {}

	InputStreamObject::shared_ptr_type operator()()
	{
		InputStreamObject::shared_ptr_type tptr(new InputStreamObject(istr,streamid));
		return tptr;
	}
};

struct InputStreamObjectTypeInfo
{
	typedef InputStreamObjectTypeInfo this_type;

	typedef InputStreamObject::shared_ptr_type pointer_type;

	static pointer_type getNullPointer()
	{
		pointer_type p;
		return p;
	}

	static pointer_type deallocate(pointer_type /* p */)
	{
		return getNullPointer();
	}
};

struct BgzfInflateReadPackageResult
{
	libmaus2::lz::BgzfInflateBase::shared_ptr_type block;
	libmaus2::lz::BgzfInflateBase::BaseBlockInfo BBI;
	uint64_t streamid;
	uint64_t blockid;
	libmaus2::autoarray::AutoArray<char>::shared_ptr_type ablock;
	uint64_t ubytes;
	bool eof;

	BgzfInflateReadPackageResult() {}
	BgzfInflateReadPackageResult(
		libmaus2::lz::BgzfInflateBase::shared_ptr_type rblock,
		libmaus2::lz::BgzfInflateBase::BaseBlockInfo const rBBI,
		uint64_t const rstreamid,
		uint64_t const rblockid,
		libmaus2::autoarray::AutoArray<char>::shared_ptr_type rablock,
		bool const reof
	) : block(rblock), BBI(rBBI), streamid(rstreamid), blockid(rblockid), ablock(rablock), ubytes(0), eof(reof)
	{

	}

	bool operator<(BgzfInflateReadPackageResult const & O) const
	{
		return blockid < O.blockid;
	}
};


struct BgzfInflateReadPackageDispatcher : public libmaus2::parallel::SimpleThreadWorkPackageDispatcher
{
	typedef BgzfInflateReadPackageDispatcher this_type;
	typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
	typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

	struct BgzfInflateReadPackage : public libmaus2::parallel::SimpleThreadWorkPackage
	{
		typedef BgzfInflateReadPackage this_type;
		typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
		typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

		BgzfInflateReadPackage() : SimpleThreadWorkPackage() {}
		BgzfInflateReadPackage(
			uint64_t const rpriority,
			uint64_t const rdispatcherid,
			uint64_t const rpackageid = 0,
			uint64_t const rsubid = 0
		)
		: SimpleThreadWorkPackage(rpriority,rdispatcherid,rpackageid,rsubid) {}

		char const * getPackageName() const
		{
			return "BgzfInflateReadPackage";
		}
	};


	struct BgzfInflateReadPackageFinished
	{
		virtual ~BgzfInflateReadPackageFinished() {}
		virtual void bgzfInflateReadPackageFinished(BgzfInflateReadPackageResult ptr) = 0;
	};

	struct BgzfInflateReadPackageReturn
	{
		virtual ~BgzfInflateReadPackageReturn() {}
		virtual void bgzfInflateReadPackageReturn(BgzfInflateReadPackage * p) = 0;
	};

	libmaus2::parallel::LockedFreeList<
		InputStreamObject::shared_ptr_type,
		InputStreamObjectAllocator,
		InputStreamObjectTypeInfo
	> * ifreelist;
	libmaus2::parallel::LockedFreeList<
                libmaus2::lz::BgzfInflateBase::shared_ptr_type,
                libmaus2::lz::BgzfInflateBaseAllocator,
                libmaus2::lz::BgzfInflateBaseTypeInfo
        > * zfreelist;

	libmaus2::parallel::LockedGrowingFreeList<
		libmaus2::autoarray::AutoArray<char>::shared_ptr_type,
		libmaus2::util::AutoArrayCharBaseAllocator,
		libmaus2::util::AutoArrayCharBaseTypeInfo
	> * afreelist;

        BgzfInflateReadPackageFinished * finished;
        BgzfInflateReadPackageReturn * packreturn;

	BgzfInflateReadPackageDispatcher(
		libmaus2::parallel::LockedFreeList<
			InputStreamObject::shared_ptr_type,
			InputStreamObjectAllocator,
			InputStreamObjectTypeInfo
		> * rifreelist,
		libmaus2::parallel::LockedFreeList<
	                libmaus2::lz::BgzfInflateBase::shared_ptr_type,
        	        libmaus2::lz::BgzfInflateBaseAllocator,
        	        libmaus2::lz::BgzfInflateBaseTypeInfo
	        > * rzfreelist,
		libmaus2::parallel::LockedGrowingFreeList<
			libmaus2::autoarray::AutoArray<char>::shared_ptr_type,
			libmaus2::util::AutoArrayCharBaseAllocator,
			libmaus2::util::AutoArrayCharBaseTypeInfo
		> * rafreelist,
	        BgzfInflateReadPackageFinished * rfinished,
	        BgzfInflateReadPackageReturn * rpackreturn
	) : libmaus2::parallel::SimpleThreadWorkPackageDispatcher(), ifreelist(rifreelist), zfreelist(rzfreelist), afreelist(rafreelist), finished(rfinished), packreturn(rpackreturn) {}

	bool getResources(InputStreamObject::shared_ptr_type & iptr, libmaus2::lz::BgzfInflateBase::shared_ptr_type & block)
	{
		iptr = ifreelist->getIf();

		if ( ! iptr )
		{
			return false;
		}

		block = zfreelist->getIf();

		if ( ! block )
		{
			ifreelist->put(iptr);
			return false;
		}

		return true;
	}

	void freeResources(InputStreamObject::shared_ptr_type & iptr, libmaus2::lz::BgzfInflateBase::shared_ptr_type & block)
	{
		zfreelist->put(block);
		ifreelist->put(iptr);
	}

	void dispatch(libmaus2::parallel::SimpleThreadWorkPackage * P, libmaus2::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & /* tpi */)
	{
		BgzfInflateReadPackage * BIRP = dynamic_cast<BgzfInflateReadPackage *>(P);
		assert ( BIRP );

		InputStreamObject::shared_ptr_type iptr;
		libmaus2::lz::BgzfInflateBase::shared_ptr_type block;

		while ( getResources(iptr,block) )
		{
			if ( iptr->istr->peek() == std::istream::traits_type::eof() )
			{
				freeResources(iptr,block);

				if ( !iptr->eof )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "[E] unexpected EOF in BgzfInflateReadPackageDispatcher::dispatch" << std::endl;
					lme.finish();
					throw lme;
				}
				else
				{
					break;
				}
			}
			else
			{
				libmaus2::autoarray::AutoArray<char>::shared_ptr_type ablock = afreelist->get();
				libmaus2::lz::BgzfInflateBase::BaseBlockInfo const BBI = block->readBlock(*(iptr->istr));

				bool reporteof = (iptr->istr->peek() == std::istream::traits_type::eof());
				if ( reporteof )
					iptr->eof = true;

				BgzfInflateReadPackageResult result(block,BBI,iptr->streamid,iptr->blockid++,ablock,reporteof);
				ifreelist->put(iptr);
				finished->bgzfInflateReadPackageFinished(result);
			}
		}

		packreturn->bgzfInflateReadPackageReturn(BIRP);
	}
};

struct BgzfInflateDecompressPackageDispatcher : public libmaus2::parallel::SimpleThreadWorkPackageDispatcher
{
	typedef BgzfInflateReadPackageDispatcher this_type;
	typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
	typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

	struct BgzfInflateDecompressPackage : public libmaus2::parallel::SimpleThreadWorkPackage
	{
		typedef BgzfInflateDecompressPackage this_type;
		typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
		typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

		BgzfInflateReadPackageResult BIRP;

		BgzfInflateDecompressPackage() : SimpleThreadWorkPackage() {}
		BgzfInflateDecompressPackage(
			BgzfInflateReadPackageResult rBIRP,
			uint64_t const rpriority,
			uint64_t const rdispatcherid,
			uint64_t const rpackageid = 0,
			uint64_t const rsubid = 0
		)
		: SimpleThreadWorkPackage(rpriority,rdispatcherid,rpackageid,rsubid), BIRP(rBIRP) {}

		char const * getPackageName() const
		{
			return "BgzfInflateDecompressPackage";
		}
	};

	struct BgzfInflateDecompressPackageReturn
	{
		virtual ~BgzfInflateDecompressPackageReturn() {}
		virtual void bgzfInflateDecompressPackageReturn(BgzfInflateDecompressPackage * package) = 0;
	};

	struct BgzfInflateDecompressPackageFinished
	{
		virtual ~BgzfInflateDecompressPackageFinished() {}
		virtual void bgzfInflateDecompressPackageFinished(BgzfInflateReadPackageResult res) = 0;
	};


	BgzfInflateDecompressPackageReturn * packret;
	BgzfInflateDecompressPackageFinished * fin;

	BgzfInflateDecompressPackageDispatcher(
		BgzfInflateDecompressPackageReturn * rpackret,
		BgzfInflateDecompressPackageFinished * rfin
	)
	: libmaus2::parallel::SimpleThreadWorkPackageDispatcher(), packret(rpackret), fin(rfin)
	{}

	void dispatch(libmaus2::parallel::SimpleThreadWorkPackage * P, libmaus2::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & /* tpi */)
	{
		BgzfInflateDecompressPackage * BIRP = dynamic_cast<BgzfInflateDecompressPackage *>(P);
		BgzfInflateReadPackageResult res = BIRP->BIRP;
		assert ( BIRP );

		res.ubytes = BIRP->BIRP.block->decompressBlock(res.ablock->begin(),res.BBI);

		fin->bgzfInflateDecompressPackageFinished(res);
		packret->bgzfInflateDecompressPackageReturn(BIRP);
	}
};

struct BgzfInputContext
	: public BgzfInflateReadPackageDispatcher::BgzfInflateReadPackageFinished,
	  public BgzfInflateReadPackageDispatcher::BgzfInflateReadPackageReturn,
	  public BgzfInflateDecompressPackageDispatcher::BgzfInflateDecompressPackageReturn,
	  public BgzfInflateDecompressPackageDispatcher::BgzfInflateDecompressPackageFinished
{
	typedef BgzfInputContext this_type;
	typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
	typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

	libmaus2::parallel::SimpleThreadPool & STP;

	InputStreamObjectAllocator ialloc;
	libmaus2::parallel::LockedFreeList<
		InputStreamObject::shared_ptr_type,
		InputStreamObjectAllocator,
		InputStreamObjectTypeInfo
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

	libmaus2::parallel::LockedGrowingFreeList<BgzfInflateReadPackageDispatcher::BgzfInflateReadPackage> readPackageFreeList;
	libmaus2::parallel::LockedGrowingFreeList<BgzfInflateDecompressPackageDispatcher::BgzfInflateDecompressPackage> decompressPackageFreeList;

	bool idle()
	{
		return
			readPackageFreeList.full()
			&&
			decompressPackageFreeList.full();
	}

	uint64_t const readdispatcherid;
	uint64_t const decompressdispatcherid;

	BgzfInflateReadPackageDispatcher birpd;
	BgzfInflateDecompressPackageDispatcher bidpd;

	libmaus2::parallel::LockedBool eofflag;

	uint64_t volatile readc;
	libmaus2::parallel::PosixSpinLock readclock;

	libmaus2::util::FiniteSizeHeap<BgzfInflateReadPackageResult> blockheap;
	uint64_t volatile nextblock;
	libmaus2::parallel::PosixSpinLock nextblocklock;

	libmaus2::parallel::SynchronousConditionQueue<BgzfInflateReadPackageResult> outq;

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
		BgzfInflateReadPackageDispatcher::BgzfInflateReadPackage * pn = readPackageFreeList.get();
		*pn = BgzfInflateReadPackageDispatcher::BgzfInflateReadPackage(0/*prio*/,readdispatcherid);
		STP.enque(pn);
	}

	void bgzfInflateDecompressPackageReturn(BgzfInflateDecompressPackageDispatcher::BgzfInflateDecompressPackage * package)
	{
		decompressPackageFreeList.put(package);
	}

	void bgzfInflateReadPackageReturn(BgzfInflateReadPackageDispatcher::BgzfInflateReadPackage * p)
	{
		readPackageFreeList.put(p);
	}

	void returnBlock(BgzfInflateReadPackageResult next)
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
				BgzfInflateReadPackageDispatcher::BgzfInflateReadPackage * pn = readPackageFreeList.get();
				*pn = BgzfInflateReadPackageDispatcher::BgzfInflateReadPackage(0/*prio*/,readdispatcherid);
				STP.enque(pn);
			}
		}
	}

	void bgzfInflateDecompressPackageFinished(BgzfInflateReadPackageResult ptr)
	{
		libmaus2::parallel::ScopePosixSpinLock slock(nextblocklock);
		blockheap.push(ptr);

		while ( !blockheap.empty() && blockheap.top().blockid == nextblock )
		{
			BgzfInflateReadPackageResult next = blockheap.pop();

			if ( next.eof )
				eofflag.set(true);

			outq.enque(next);

			nextblock += 1;
		}
	}

	void bgzfInflateReadPackageFinished(BgzfInflateReadPackageResult ptr)
	{
		BgzfInflateDecompressPackageDispatcher::BgzfInflateDecompressPackage * pack = decompressPackageFreeList.get();
		*pack = BgzfInflateDecompressPackageDispatcher::BgzfInflateDecompressPackage(ptr,0 /* prio */,decompressdispatcherid);
		STP.enque(pack);
	}
};


int main()
{
	try
	{
		libmaus2::parallel::SimpleThreadPool STP(32);

		try
		{
			BgzfInputContext BIC(std::cin,STP);
			BIC.init();

			bool eof = false;
			while ( !eof && !STP.isInPanicMode() )
			{
				BgzfInflateReadPackageResult next;
				if ( BIC.outq.timeddeque(next) )
				{
					eof = eof || next.eof;

					if ( next.blockid % 1024 == 0 || next.eof )
						std::cerr << "blockid " << next.blockid << std::endl;

					BIC.returnBlock(next);
				}
			}

			while ( !BIC.idle() && !STP.isInPanicMode() )
			{
				sleep(1);
			}

			std::cerr << "eof" << std::endl;

			if ( STP.isInPanicMode() )
			{
				std::cerr << STP.getPanicMessage() << std::endl;
			}

			STP.terminate();
		}
		catch(std::exception const & ex)
		{
			std::cerr << ex.what() << std::endl;
			STP.terminate();
		}
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
