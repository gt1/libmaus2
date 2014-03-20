/*
    libmaus
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#include <libmaus/parallel/ThreadPool.hpp>
#include <libmaus/lz/BgzfInflateBase.hpp>
#include <libmaus/parallel/LockedBool.hpp>

libmaus::parallel::PosixMutex cerrmutex;

struct BamThreadPoolDecodeBamParseQueueInfo
{
	uint64_t packageid;
	std::pair<uint64_t,uint64_t> blockmeta;
	uint64_t baseid;
	uint64_t blockid;
	
	BamThreadPoolDecodeBamParseQueueInfo()
	: packageid(0), blockmeta(0,0), baseid(0), blockid(0)
	{
	
	}
	BamThreadPoolDecodeBamParseQueueInfo(
		uint64_t rpackageid,
		std::pair<uint64_t,uint64_t> rblockmeta,
		uint64_t rbaseid,
		uint64_t rblockid
	)
	: packageid(rpackageid), blockmeta(rblockmeta), baseid(rbaseid), blockid(rblockid)
	{
	
	}
	
	bool operator<(BamThreadPoolDecodeBamParseQueueInfo const & o) const
	{
		return blockid > o.blockid;
	}
};

struct BamThreadPoolDecodeContextBase
{
	enum bamthreadpooldecodecontextbase_dispatcher_ids
	{
		bamthreadpooldecodecontextbase_dispatcher_id_read = 0,
		bamthreadpooldecodecontextbase_dispatcher_id_decompress = 1,
		bamthreadpooldecodecontextbase_dispatcher_id_bamparse = 2
	};
	
	static unsigned int const bamthreadpooldecodecontextbase_dispatcher_priority_read = 0;
	static unsigned int const bamthreadpooldecodecontextbase_dispatcher_priority_decompress = 0;
	static unsigned int const bamthreadpooldecodecontextbase_dispatcher_priority_bamparse = 0;

	libmaus::parallel::PosixMutex packageIdLock;
	uint64_t nextPackageId;
	
	libmaus::parallel::PosixMutex inputLock;
	std::istream & in;
	uint64_t readCnt;
	libmaus::parallel::LockedBool readComplete;
	libmaus::autoarray::AutoArray< libmaus::lz::BgzfInflateBase::unique_ptr_type > inflateBases;
	libmaus::autoarray::AutoArray< char > inflateDecompressSpace;
	libmaus::parallel::SynchronousQueue<uint64_t> inflateBasesFreeList;
	uint64_t nextInputBlockId;
	
	libmaus::parallel::PosixMutex decompressLock;
	uint64_t decompressCnt;
	libmaus::parallel::LockedBool decompressComplete;
	
	libmaus::parallel::PosixMutex bamParseQueueLock;
	std::priority_queue<BamThreadPoolDecodeBamParseQueueInfo> bamparseQueue;
	uint64_t bamParseNextBlock;

	libmaus::parallel::PosixMutex bamParseLock;
	uint64_t bamParseCnt;
	libmaus::parallel::LockedBool bamParseComplete;
			
	BamThreadPoolDecodeContextBase(
		std::istream & rin,
		uint64_t const numInflateBases
	)
	:
	  //
	  packageIdLock(),
	  nextPackageId(0), 
	  //
	  inputLock(),
	  in(rin),
	  readCnt(0),
	  readComplete(false), 
	  inflateBases(numInflateBases),
	  inflateDecompressSpace(numInflateBases * libmaus::lz::BgzfConstants::getBgzfMaxBlockSize()),
	  inflateBasesFreeList(),
	  nextInputBlockId(0),
	  decompressLock(),
	  decompressCnt(0),
	  decompressComplete(false),
	  bamParseQueueLock(),
	  bamparseQueue(),
	  bamParseNextBlock(0),
	  bamParseLock(),
	  bamParseCnt(0),
	  bamParseComplete(false)
	{
		for ( uint64_t i = 0; i < inflateBases.size(); ++i )
		{
			libmaus::lz::BgzfInflateBase::unique_ptr_type tptr(new libmaus::lz::BgzfInflateBase);
			inflateBases[i] = UNIQUE_PTR_MOVE(tptr);
			inflateBasesFreeList.enque(i);
		}
	}
	
	char * getDecompressSpace(uint64_t const rbaseid)
	{
		return inflateDecompressSpace.begin() + rbaseid * libmaus::lz::BgzfConstants::getBgzfMaxBlockSize();
	}
};

struct BamThreadPoolDecodeReadPackage : public ::libmaus::parallel::ThreadWorkPackage
{
	BamThreadPoolDecodeContextBase * contextbase;

	BamThreadPoolDecodeReadPackage() : contextbase(0) {}
	BamThreadPoolDecodeReadPackage(
		uint64_t const rpackageid,
		BamThreadPoolDecodeContextBase * rcontextbase
	)
	: ::libmaus::parallel::ThreadWorkPackage(
		BamThreadPoolDecodeContextBase::bamthreadpooldecodecontextbase_dispatcher_priority_read,
		BamThreadPoolDecodeContextBase::bamthreadpooldecodecontextbase_dispatcher_id_read,
		rpackageid
	), contextbase(rcontextbase)
	{
	
	}

	virtual ::libmaus::parallel::ThreadWorkPackage::unique_ptr_type uclone() const
	{
		::libmaus::parallel::ThreadWorkPackage::unique_ptr_type tptr(new BamThreadPoolDecodeReadPackage(packageid,contextbase));
		return UNIQUE_PTR_MOVE(tptr);
	}
	virtual ::libmaus::parallel::ThreadWorkPackage::shared_ptr_type sclone() const
	{
		::libmaus::parallel::ThreadWorkPackage::shared_ptr_type sptr(new BamThreadPoolDecodeReadPackage(packageid,contextbase));
		return sptr;
	}
};

struct BamThreadPoolDecodeDecompressPackage : public ::libmaus::parallel::ThreadWorkPackage
{
	BamThreadPoolDecodeContextBase * contextbase;

	std::pair<uint64_t,uint64_t> blockmeta; // block size compressed and uncompressed
	uint64_t baseid;
	uint64_t blockid;

	BamThreadPoolDecodeDecompressPackage() : contextbase(0) {}
	BamThreadPoolDecodeDecompressPackage(
		uint64_t const rpackageid,
		BamThreadPoolDecodeContextBase * rcontextbase,
		std::pair<uint64_t,uint64_t> rblockmeta,
		uint64_t rbaseid,
		uint64_t rblockid
		
	)
	: ::libmaus::parallel::ThreadWorkPackage(
		BamThreadPoolDecodeContextBase::bamthreadpooldecodecontextbase_dispatcher_priority_decompress,
		BamThreadPoolDecodeContextBase::bamthreadpooldecodecontextbase_dispatcher_id_decompress,
		rpackageid
	), contextbase(rcontextbase), blockmeta(rblockmeta), baseid(rbaseid), blockid(rblockid)
	{
	
	}

	virtual ::libmaus::parallel::ThreadWorkPackage::unique_ptr_type uclone() const
	{
		::libmaus::parallel::ThreadWorkPackage::unique_ptr_type tptr(new BamThreadPoolDecodeDecompressPackage(packageid,contextbase,blockmeta,baseid,blockid));
		return UNIQUE_PTR_MOVE(tptr);
	}
	virtual ::libmaus::parallel::ThreadWorkPackage::shared_ptr_type sclone() const
	{
		::libmaus::parallel::ThreadWorkPackage::shared_ptr_type sptr(new BamThreadPoolDecodeDecompressPackage(packageid,contextbase,blockmeta,baseid,blockid));
		return sptr;
	}
};


struct BamThreadPoolDecodeBamParsePackage : public ::libmaus::parallel::ThreadWorkPackage
{
	BamThreadPoolDecodeContextBase * contextbase;

	std::pair<uint64_t,uint64_t> blockmeta; // block size compressed and uncompressed
	uint64_t baseid;
	uint64_t blockid;

	BamThreadPoolDecodeBamParsePackage() : contextbase(0) {}
	BamThreadPoolDecodeBamParsePackage(
		uint64_t const rpackageid,
		BamThreadPoolDecodeContextBase * rcontextbase,
		std::pair<uint64_t,uint64_t> rblockmeta,
		uint64_t rbaseid,
		uint64_t rblockid		
	)
	: ::libmaus::parallel::ThreadWorkPackage(
		BamThreadPoolDecodeContextBase::bamthreadpooldecodecontextbase_dispatcher_priority_bamparse,
		BamThreadPoolDecodeContextBase::bamthreadpooldecodecontextbase_dispatcher_id_bamparse,
		rpackageid
	), contextbase(rcontextbase), blockmeta(rblockmeta), baseid(rbaseid), blockid(rblockid)
	{
	
	}

	virtual ::libmaus::parallel::ThreadWorkPackage::unique_ptr_type uclone() const
	{
		::libmaus::parallel::ThreadWorkPackage::unique_ptr_type tptr(new BamThreadPoolDecodeBamParsePackage(packageid,contextbase,blockmeta,baseid,blockid));
		return UNIQUE_PTR_MOVE(tptr);
	}
	virtual ::libmaus::parallel::ThreadWorkPackage::shared_ptr_type sclone() const
	{
		::libmaus::parallel::ThreadWorkPackage::shared_ptr_type sptr(new BamThreadPoolDecodeBamParsePackage(packageid,contextbase,blockmeta,baseid,blockid));
		return sptr;
	}
};

struct BamThreadPoolDecodeReadPackageDispatcher : public libmaus::parallel::ThreadWorkPackageDispatcher
{
	virtual ~BamThreadPoolDecodeReadPackageDispatcher() {}
	virtual void dispatch(
		libmaus::parallel::ThreadWorkPackage * P, 
		libmaus::parallel::ThreadPoolInterfaceEnqueTermInterface & tpi
	)
	{
		assert ( dynamic_cast<BamThreadPoolDecodeReadPackage *>(P) != 0 );
		
		BamThreadPoolDecodeReadPackage & RP = *dynamic_cast<BamThreadPoolDecodeReadPackage *>(P);
		BamThreadPoolDecodeContextBase & contextbase = *(RP.contextbase);
		libmaus::parallel::PosixMutex & inputLock = contextbase.inputLock;
		libmaus::parallel::ScopePosixMutex slock(inputLock);

		if ( ! contextbase.readComplete.get() )
		{
			if ( contextbase.in.peek() != std::istream::traits_type::eof() )
			{
				uint64_t const nextInputBlockId = (contextbase.nextInputBlockId++);
				uint64_t const baseid = contextbase.inflateBasesFreeList.deque();
			
				std::pair<uint64_t,uint64_t> const blockmeta = contextbase.inflateBases[baseid]->readBlock(contextbase.in);
				contextbase.readCnt += 1;
			
				#if 0
				cerrmutex.lock();
				std::cerr << "got block " << nextInputBlockId << "\t(" << blockmeta.first << "," << blockmeta.second << ")" << std::endl;
				cerrmutex.unlock();
				#endif
				
				libmaus::parallel::ScopePosixMutex plock(contextbase.packageIdLock);
				BamThreadPoolDecodeDecompressPackage const BTPDDP(
					contextbase.nextPackageId++,
					RP.contextbase,
					blockmeta,
					baseid,
					nextInputBlockId
				);
					
				tpi.enque(BTPDDP);
			}
			else
			{
				contextbase.readComplete.set(true);
			}
		}
	}
};

struct BamThreadPoolDecodeDecompressPackageDispatcher : public libmaus::parallel::ThreadWorkPackageDispatcher
{
	virtual ~BamThreadPoolDecodeDecompressPackageDispatcher() {}
	virtual void dispatch(
		libmaus::parallel::ThreadWorkPackage * P, 
		libmaus::parallel::ThreadPoolInterfaceEnqueTermInterface & tpi
	)
	{
		assert ( dynamic_cast<BamThreadPoolDecodeDecompressPackage *>(P) != 0 );
		
		BamThreadPoolDecodeDecompressPackage & RP = *dynamic_cast<BamThreadPoolDecodeDecompressPackage *>(P);
		BamThreadPoolDecodeContextBase & contextbase = *(RP.contextbase);

		char * const decompressSpace = contextbase.getDecompressSpace(RP.baseid);
		contextbase.inflateBases[RP.baseid]->decompressBlock(decompressSpace,RP.blockmeta);

		#if 0
		cerrmutex.lock();
		std::cerr << "back block " << RP.blockid << std::endl;
		cerrmutex.unlock();
		#endif

		libmaus::parallel::ScopePosixMutex ldecompressLock(contextbase.decompressLock);
		contextbase.decompressCnt += 1;
		if ( contextbase.readComplete.get() && (contextbase.decompressCnt == contextbase.readCnt) )
			contextbase.decompressComplete.set(true);

		{
			// generate parse info object
			libmaus::parallel::ScopePosixMutex plock(contextbase.packageIdLock);
			BamThreadPoolDecodeBamParseQueueInfo qinfo(contextbase.nextPackageId++,
				RP.blockmeta,RP.baseid,RP.blockid);

			// enque the parse info object
			libmaus::parallel::ScopePosixMutex lbamParseQueueLock(contextbase.bamParseQueueLock);
			contextbase.bamparseQueue.push(qinfo);
			
			// check whether this is the next block for parsing
			if ( contextbase.bamParseNextBlock == contextbase.bamparseQueue.top().blockid )
			{
				BamThreadPoolDecodeBamParseQueueInfo const bqinfo = contextbase.bamparseQueue.top();
				contextbase.bamparseQueue.pop();

				BamThreadPoolDecodeBamParsePackage BTPDBPP(
					bqinfo.packageid,
					RP.contextbase,
					bqinfo.blockmeta,
					bqinfo.baseid,
					bqinfo.blockid
				);
				
				tpi.enque(BTPDBPP);
			}
		}		
	}
};

struct BamThreadPoolDecodeBamParsePackageDispatcher : public libmaus::parallel::ThreadWorkPackageDispatcher
{
	virtual ~BamThreadPoolDecodeBamParsePackageDispatcher() {}
	virtual void dispatch(
		libmaus::parallel::ThreadWorkPackage * P, 
		libmaus::parallel::ThreadPoolInterfaceEnqueTermInterface & tpi
	)
	{
		assert ( dynamic_cast<BamThreadPoolDecodeBamParsePackage *>(P) != 0 );
		
		BamThreadPoolDecodeBamParsePackage & RP = *dynamic_cast<BamThreadPoolDecodeBamParsePackage *>(P);
		BamThreadPoolDecodeContextBase & contextbase = *(RP.contextbase);
		
		#if 0
		cerrmutex.lock();
		std::cerr << "bamparse " << RP.blockid << std::endl;
		cerrmutex.unlock();
		#endif

		libmaus::parallel::ScopePosixMutex lbamParseLock(contextbase.bamParseLock);
		contextbase.bamParseCnt += 1;
		if ( contextbase.decompressComplete.get() && (contextbase.bamParseCnt == contextbase.decompressCnt) )
		{
			contextbase.bamParseComplete.set(true);
			tpi.terminate();
		}

		{
			contextbase.inflateBasesFreeList.enque(RP.baseid);
			libmaus::parallel::ScopePosixMutex plock(contextbase.packageIdLock);
			BamThreadPoolDecodeReadPackage const BTPDRP(contextbase.nextPackageId++,RP.contextbase);
			tpi.enque(BTPDRP);
		}

		{
			libmaus::parallel::ScopePosixMutex lbamParseQueueLock(contextbase.bamParseQueueLock);
			contextbase.bamParseNextBlock += 1;

			if ( contextbase.bamParseNextBlock == contextbase.bamparseQueue.top().blockid )
			{
				BamThreadPoolDecodeBamParseQueueInfo const bqinfo = contextbase.bamparseQueue.top();
				contextbase.bamparseQueue.pop();

				BamThreadPoolDecodeBamParsePackage BTPDBPP(
					bqinfo.packageid,
					RP.contextbase,
					bqinfo.blockmeta,
					bqinfo.baseid,
					bqinfo.blockid
				);
				
				tpi.enque(BTPDBPP);
			}
		}
	}
};

struct BamThreadPoolDecodeContext : public BamThreadPoolDecodeContextBase
{
	libmaus::parallel::ThreadPool & TP;

	BamThreadPoolDecodeContext(
		std::istream & rin, 
		uint64_t const numInflateBases,
		libmaus::parallel::ThreadPool & rTP
	)
	: BamThreadPoolDecodeContextBase(rin,numInflateBases), TP(rTP) 
	{
	
	}
	
	void startup()
	{
		for ( uint64_t i = 0; i < BamThreadPoolDecodeContextBase::inflateBases.size(); ++i )
		{
			libmaus::parallel::ScopePosixMutex plock(BamThreadPoolDecodeContextBase::packageIdLock);
			BamThreadPoolDecodeReadPackage const BTPDRP(BamThreadPoolDecodeContextBase::nextPackageId++,this);
			TP.enque(BTPDRP);
		}
	}
};

int main()
{
	libmaus::parallel::ThreadPool TP(8);

	BamThreadPoolDecodeReadPackageDispatcher readdispatcher;
	TP.registerDispatcher(BamThreadPoolDecodeContext::bamthreadpooldecodecontextbase_dispatcher_id_read,&readdispatcher);
	BamThreadPoolDecodeDecompressPackageDispatcher decompressdispatcher;
	TP.registerDispatcher(BamThreadPoolDecodeContext::bamthreadpooldecodecontextbase_dispatcher_id_decompress,&decompressdispatcher);
	BamThreadPoolDecodeBamParsePackageDispatcher bamparsedispatcher;
	TP.registerDispatcher(BamThreadPoolDecodeContext::bamthreadpooldecodecontextbase_dispatcher_id_bamparse,&bamparsedispatcher);

	BamThreadPoolDecodeContext context(std::cin,8,TP);
	context.startup();

#if 0
	libmaus::parallel::DummyThreadWorkPackageMeta meta;
	libmaus::parallel::PosixMutex::shared_ptr_type printmutex(new libmaus::parallel::PosixMutex);

	TP.enque(libmaus::parallel::DummyThreadWorkPackage(0,dispid,printmutex,&meta));
#endif
	
	TP.join();
}
