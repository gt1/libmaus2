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
	namespace lz
	{
		struct AutoArrayBaseAllocator
		{
			uint64_t size;
			AutoArrayBaseAllocator(uint64_t rsize = libmaus2::lz::BgzfConstants::getBgzfMaxBlockSize()) : size(rsize) {}

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
	namespace lz
	{
		struct AutoArrayBaseTypeInfo
		{
			typedef AutoArrayBaseTypeInfo this_type;

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
	
	BgzfInflateReadPackageResult() {}
	BgzfInflateReadPackageResult(
		libmaus2::lz::BgzfInflateBase::shared_ptr_type rblock,
		libmaus2::lz::BgzfInflateBase::BaseBlockInfo const rBBI,
		uint64_t const rstreamid,
		uint64_t const rblockid,
		libmaus2::autoarray::AutoArray<char>::shared_ptr_type rablock
	) : block(rblock), BBI(rBBI), streamid(rstreamid), blockid(rblockid), ablock(rablock), ubytes(0)
	{
	
	}
};

struct BgzfInflateReadPackageFinished
{
	virtual ~BgzfInflateReadPackageFinished() {}
	virtual void bgzfInflateReadPackageFinished(BgzfInflateReadPackageResult ptr) = 0;
};

struct BgzfInflateReadPackageEOF
{
	virtual ~BgzfInflateReadPackageEOF() {}
	virtual void bgzfInflateReadPackageEOF() = 0;
};

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

struct BgzfInflateReadPackageReturn
{
	virtual ~BgzfInflateReadPackageReturn() {}
	virtual void bgzfInflateReadPackageReturn(BgzfInflateReadPackage * p) = 0;
};

struct BgzfInflateReadPackageDispatcher : public libmaus2::parallel::SimpleThreadWorkPackageDispatcher
{
	typedef BgzfInflateReadPackageDispatcher this_type;
	typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
	typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

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
		libmaus2::lz::AutoArrayBaseAllocator,
		libmaus2::lz::AutoArrayBaseTypeInfo
	> * afreelist;

        BgzfInflateReadPackageFinished * finished;
        BgzfInflateReadPackageEOF * eof;
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
			libmaus2::lz::AutoArrayBaseAllocator,
			libmaus2::lz::AutoArrayBaseTypeInfo
		> * rafreelist,
	        BgzfInflateReadPackageFinished * rfinished,
	        BgzfInflateReadPackageEOF * reof,
	        BgzfInflateReadPackageReturn * rpackreturn
	) : libmaus2::parallel::SimpleThreadWorkPackageDispatcher(), ifreelist(rifreelist), zfreelist(rzfreelist), afreelist(rafreelist), finished(rfinished), eof(reof), packreturn(rpackreturn) {}

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
		bool triggereof = false;

		BgzfInflateReadPackage * BIRP = dynamic_cast<BgzfInflateReadPackage *>(P);
		assert ( BIRP );
	
		InputStreamObject::shared_ptr_type iptr;
		libmaus2::lz::BgzfInflateBase::shared_ptr_type block;

		while ( getResources(iptr,block) )
		{
			if ( iptr->istr->peek() == std::istream::traits_type::eof() )
			{
				bool const reporteof = !iptr->eof;
				iptr->eof = true;
				
				freeResources(iptr,block);
				
				if ( reporteof )
				{
					triggereof = true;
					break;
				}
			}
			else
			{
				libmaus2::autoarray::AutoArray<char>::shared_ptr_type ablock = afreelist->get();
				libmaus2::lz::BgzfInflateBase::BaseBlockInfo const BBI = block->readBlock(*(iptr->istr));
				BgzfInflateReadPackageResult result(block,BBI,iptr->streamid,iptr->blockid++,ablock);
				ifreelist->put(iptr);
				finished->bgzfInflateReadPackageFinished(result);
			}
		}
		
		packreturn->bgzfInflateReadPackageReturn(BIRP);

		if ( triggereof )
			eof->bgzfInflateReadPackageEOF();
	}
};

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

struct BgzfInflateDecompressPackageDispatcher : public libmaus2::parallel::SimpleThreadWorkPackageDispatcher
{
	typedef BgzfInflateReadPackageDispatcher this_type;
	typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
	typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

	BgzfInflateDecompressPackageDispatcher() 
	: libmaus2::parallel::SimpleThreadWorkPackageDispatcher()
	{}

	void dispatch(libmaus2::parallel::SimpleThreadWorkPackage * P, libmaus2::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & /* tpi */)
	{
		BgzfInflateDecompressPackage * BIRP = dynamic_cast<BgzfInflateDecompressPackage *>(P);
		assert ( BIRP );
	
		BIRP->BIRP.ubytes = BIRP->BIRP.block->decompressBlock(BIRP->BIRP.ablock->begin(),BIRP->BIRP.BBI);

		#if 0
		packreturn->bgzfInflateReadPackageReturn(BIRP);
		#endif
	}
};

struct BgzfInputContext
	: public BgzfInflateReadPackageFinished,
	  public BgzfInflateReadPackageEOF,
	  public BgzfInflateReadPackageReturn
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

        libmaus2::lz::AutoArrayBaseAllocator aalloc;
	libmaus2::parallel::LockedGrowingFreeList<
		libmaus2::autoarray::AutoArray<char>::shared_ptr_type,
		libmaus2::lz::AutoArrayBaseAllocator,
		libmaus2::lz::AutoArrayBaseTypeInfo
	> afreelist;
	
	libmaus2::parallel::LockedGrowingFreeList<BgzfInflateReadPackage> readPackageFreeList;
	
	uint64_t const readdispatcherid;
	uint64_t const decompressdispatcherid;
		
	BgzfInflateReadPackageDispatcher birpd;

	libmaus2::parallel::LockedBool eofflag;
	
	BgzfInputContext(
		std::istream & in,
		libmaus2::parallel::SimpleThreadPool & rSTP,
		uint64_t const size
	) :
	  STP(rSTP),
	  ialloc(&in,0),
	  ifreelist(1,ialloc),
	  zfreelist(size),
	  aalloc(libmaus2::lz::BgzfConstants::getBgzfMaxBlockSize()),
	  afreelist(aalloc),
	  readdispatcherid(STP.getNextDispatcherId()),
	  decompressdispatcherid(STP.getNextDispatcherId()),
	  birpd(&ifreelist,&zfreelist,&afreelist,this,this,this),
	  eofflag(false)
	{
		STP.registerDispatcher(readdispatcherid,&birpd);
	}
	
	void init()
	{
		BgzfInflateReadPackage * pn = readPackageFreeList.get();
		*pn = BgzfInflateReadPackage(0/*prio*/,readdispatcherid);
		STP.enque(pn);
	}

	void bgzfInflateReadPackageReturn(BgzfInflateReadPackage * p)
	{
		readPackageFreeList.put(p);
	}

	void bgzfInflateReadPackageFinished(BgzfInflateReadPackageResult ptr)
	{
		if ( ptr.blockid % 16384 == 0 )
		{
			std::cerr << "block " << ptr.blockid << std::endl;
		}
	
		afreelist.put(ptr.ablock);
		zfreelist.put(ptr.block);

		BgzfInflateReadPackage * pn = readPackageFreeList.get();
		*pn = BgzfInflateReadPackage(0/*prio*/,readdispatcherid);
		STP.enque(pn);
	}
	
	void bgzfInflateReadPackageEOF()
	{
		std::cerr << "EOF" << std::endl;
		
		eofflag.set(true);
	}
};


int main()
{
	try
	{
		libmaus2::parallel::SimpleThreadPool STP(32);
		BgzfInputContext BIC(std::cin,STP,16);
		BIC.init();
		
		while ( !BIC.eofflag.get() )
		{
			sleep(1);
		}
		
		STP.terminate();
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
