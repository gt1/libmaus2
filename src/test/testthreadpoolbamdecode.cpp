/*
    libmaus2
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
#include <libmaus2/parallel/ThreadPool.hpp>
#include <libmaus2/lz/BgzfInflateBase.hpp>
#include <libmaus2/parallel/LockedBool.hpp>
#include <libmaus2/bambam/BamHeader.hpp>
#include <libmaus2/bambam/BamAlignment.hpp>
#include <libmaus2/util/GetObject.hpp>
#include <libmaus2/bambam/BamAlignmentPosComparator.hpp>

libmaus2::parallel::PosixMutex cerrmutex;

struct BamThreadPoolDecodeBamParseQueueInfo
{
	uint64_t packageid;
	libmaus2::lz::BgzfInflateBase::BaseBlockInfo blockmeta;
	uint64_t baseid;
	uint64_t blockid;

	BamThreadPoolDecodeBamParseQueueInfo()
	: packageid(0), blockmeta(0,0,0,0), baseid(0), blockid(0)
	{

	}
	BamThreadPoolDecodeBamParseQueueInfo(
		uint64_t rpackageid,
		libmaus2::lz::BgzfInflateBase::BaseBlockInfo rblockmeta,
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

struct BamThreadPoolDecodeBamProcessQueueInfo
{
	uint64_t packageid;
	uint64_t baseid;
	uint64_t blockid;

	BamThreadPoolDecodeBamProcessQueueInfo()
	: packageid(0), baseid(0), blockid(0)
	{

	}
	BamThreadPoolDecodeBamProcessQueueInfo(
		uint64_t rpackageid,
		uint64_t rbaseid,
		uint64_t rblockid
	)
	: packageid(rpackageid), baseid(rbaseid), blockid(rblockid)
	{

	}

	bool operator<(BamThreadPoolDecodeBamProcessQueueInfo const & o) const
	{
		return blockid > o.blockid;
	}
};

struct BamProcessBuffer
{
	typedef BamProcessBuffer this_type;
	typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
	typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

	libmaus2::autoarray::AutoArray<uint64_t> B8;

	uint8_t * const ca;
	uint8_t * cc;

	uint64_t * const pa;
	uint64_t * pc;

	int64_t bufferid;

	void reset()
	{
		cc = ca;
		pc = pa;
	}

	BamProcessBuffer(uint64_t const bytesize)
	: B8((bytesize + sizeof(uint64_t) - 1)/sizeof(uint64_t),false),
	  ca(reinterpret_cast<uint8_t *>(B8.begin())), cc(ca),
	  pa(B8.end()), pc(pa),
	  bufferid(-1)
	{

	}

	bool put(uint8_t const * p, uint64_t const s)
	{
		uint64_t const spaceav = (reinterpret_cast<uint8_t *>(pc)-cc);

		if ( spaceav >= s + sizeof(uint64_t) )
		{
			*(--pc) = cc-ca;
			std::copy(p,p+s,cc);
			cc += s;
			return true;
		}
		else
		{
			return false;
		}
	}
};

template<typename _value_type>
struct PosixLockedQueue
{
	typedef _value_type value_type;

	libmaus2::parallel::PosixMutex lock;
	std::deque<value_type> Q;

	PosixLockedQueue()
	: lock(), Q()
	{

	}

	uint64_t size()
	{
		libmaus2::parallel::ScopePosixMutex llock(lock);
		return Q.size();
	}

	bool empty()
	{
		libmaus2::parallel::ScopePosixMutex llock(lock);
		return Q.size() == 0;
	}

	void push_back(value_type const v)
	{
		libmaus2::parallel::ScopePosixMutex llock(lock);
		Q.push_back(v);
	}

	void push_front(value_type const v)
	{
		libmaus2::parallel::ScopePosixMutex llock(lock);
		Q.push_front(v);
	}

	void pop_back()
	{
		libmaus2::parallel::ScopePosixMutex llock(lock);
		Q.pop_back();
	}

	void pop_front()
	{
		libmaus2::parallel::ScopePosixMutex llock(lock);
		Q.pop_front();
	}

	value_type front()
	{
		libmaus2::parallel::ScopePosixMutex llock(lock);
		return Q.front();
	}

	value_type back()
	{
		libmaus2::parallel::ScopePosixMutex llock(lock);
		return Q.back();
	}

	value_type dequeFront()
	{
		libmaus2::parallel::ScopePosixMutex llock(lock);
		value_type const v = Q.front();
		Q.pop_front();
		return v;
	}

	value_type dequeBack()
	{
		libmaus2::parallel::ScopePosixMutex llock(lock);
		value_type const v = Q.back();
		Q.pop_back();
		return v;
	}
};

struct BamThreadPoolDecodeContextBase
{
	enum bamthreadpooldecodecontextbase_dispatcher_ids
	{
		bamthreadpooldecodecontextbase_dispatcher_id_read = 0,
		bamthreadpooldecodecontextbase_dispatcher_id_decompress = 1,
		bamthreadpooldecodecontextbase_dispatcher_id_bamparse = 2,
		bamthreadpooldecodecontextbase_dispatcher_id_bamprocess = 3
	};

	static unsigned int const bamthreadpooldecodecontextbase_dispatcher_priority_read = 0;
	static unsigned int const bamthreadpooldecodecontextbase_dispatcher_priority_decompress = 0;
	static unsigned int const bamthreadpooldecodecontextbase_dispatcher_priority_bamparse = 0;
	static unsigned int const bamthreadpooldecodecontextbase_dispatcher_priority_bamprocess = 0;

	libmaus2::parallel::PosixMutex packageIdLock;
	uint64_t nextPackageId;

	uint64_t getNextPackageId()
	{
		libmaus2::parallel::ScopePosixMutex lpackageIdLock(packageIdLock);
		return nextPackageId++;
	}

	libmaus2::parallel::PosixMutex inputLock;
	std::istream & in;
	uint64_t readCnt;
	libmaus2::parallel::LockedBool readComplete;
	libmaus2::autoarray::AutoArray< libmaus2::lz::BgzfInflateBase::unique_ptr_type > inflateBases;
	libmaus2::autoarray::AutoArray< char > inflateDecompressSpace;
	libmaus2::parallel::SynchronousQueue<uint64_t> inflateBasesFreeList;
	uint64_t nextInputBlockId;

	libmaus2::parallel::PosixMutex decompressLock;
	uint64_t decompressCnt;
	libmaus2::parallel::LockedBool decompressComplete;

	libmaus2::parallel::PosixMutex bamParseQueueLock;
	std::priority_queue<BamThreadPoolDecodeBamParseQueueInfo> bamparseQueue;
	std::priority_queue<BamThreadPoolDecodeBamParseQueueInfo> bamparseStall;
	uint64_t bamParseNextBlock;

	libmaus2::parallel::PosixMutex bamParseLock;
	uint64_t bamParseCnt;
	libmaus2::parallel::LockedBool bamParseComplete;

	libmaus2::parallel::LockedBool haveheader;
	::libmaus2::bambam::BamHeaderParserState bamheaderparsestate;
	libmaus2::bambam::BamHeader header;

	enum bam_parser_state_type {
		bam_parser_state_read_blocklength,
		bam_parser_state_read_blockdata
	};

	bam_parser_state_type bamParserState;
	unsigned int bamBlockSizeRead;
	uint32_t bamBlockSizeValue;
	uint32_t bamBlockDataRead;
	libmaus2::bambam::BamAlignment bamGatherBuffer;

	libmaus2::autoarray::AutoArray<BamProcessBuffer::unique_ptr_type> processBuffers;
	PosixLockedQueue<uint64_t> processBuffersFreeList;
	uint64_t nextProcessBufferIdIn;
	libmaus2::parallel::PosixMutex nextProcessBufferIdOutLock;
	uint64_t nextProcessBufferIdOut;

	libmaus2::parallel::LockedBool bamProcessComplete;

	BamThreadPoolDecodeContextBase(
		std::istream & rin,
		uint64_t const numInflateBases,
		uint64_t const numProcessBuffers,
		uint64_t const processBufferSize
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
	  inflateDecompressSpace(numInflateBases * libmaus2::lz::BgzfConstants::getBgzfMaxBlockSize()),
	  inflateBasesFreeList(),
	  nextInputBlockId(0),
	  decompressLock(),
	  decompressCnt(0),
	  decompressComplete(false),
	  bamParseQueueLock(),
	  bamparseQueue(),
	  bamparseStall(),
	  bamParseNextBlock(0),
	  bamParseLock(),
	  bamParseCnt(0),
	  bamParseComplete(false),
	  haveheader(false),
	  bamheaderparsestate(),
	  header(),
	  bamParserState(bam_parser_state_read_blocklength),
	  bamBlockSizeRead(0),
	  bamBlockSizeValue(0),
	  bamBlockDataRead(0),
	  processBuffers(numProcessBuffers),
	  processBuffersFreeList(),
	  nextProcessBufferIdIn(0),
	  nextProcessBufferIdOut(0),
	  bamProcessComplete(false)
	{
		for ( uint64_t i = 0; i < inflateBases.size(); ++i )
		{
			libmaus2::lz::BgzfInflateBase::unique_ptr_type tptr(new libmaus2::lz::BgzfInflateBase);
			inflateBases[i] = UNIQUE_PTR_MOVE(tptr);
			inflateBasesFreeList.enque(i);
		}

		for ( uint64_t i = 0; i < processBuffers.size(); ++i )
		{
			BamProcessBuffer::unique_ptr_type tptr(new BamProcessBuffer(processBufferSize));
			processBuffers[i] = UNIQUE_PTR_MOVE(tptr);
			processBuffersFreeList.push_back(i);
		}
	}

	char * getDecompressSpace(uint64_t const rbaseid)
	{
		return inflateDecompressSpace.begin() + rbaseid * libmaus2::lz::BgzfConstants::getBgzfMaxBlockSize();
	}
};

struct BamThreadPoolDecodeReadPackage : public ::libmaus2::parallel::ThreadWorkPackage
{
	BamThreadPoolDecodeContextBase * contextbase;

	BamThreadPoolDecodeReadPackage() : contextbase(0) {}
	BamThreadPoolDecodeReadPackage(
		uint64_t const rpackageid,
		BamThreadPoolDecodeContextBase * rcontextbase
	)
	: ::libmaus2::parallel::ThreadWorkPackage(
		BamThreadPoolDecodeContextBase::bamthreadpooldecodecontextbase_dispatcher_priority_read,
		BamThreadPoolDecodeContextBase::bamthreadpooldecodecontextbase_dispatcher_id_read,
		rpackageid
	), contextbase(rcontextbase)
	{

	}

	virtual ::libmaus2::parallel::ThreadWorkPackage::unique_ptr_type uclone() const
	{
		::libmaus2::parallel::ThreadWorkPackage::unique_ptr_type tptr(new BamThreadPoolDecodeReadPackage(packageid,contextbase));
		return UNIQUE_PTR_MOVE(tptr);
	}
	virtual ::libmaus2::parallel::ThreadWorkPackage::shared_ptr_type sclone() const
	{
		::libmaus2::parallel::ThreadWorkPackage::shared_ptr_type sptr(new BamThreadPoolDecodeReadPackage(packageid,contextbase));
		return sptr;
	}
};

struct BamThreadPoolDecodeDecompressPackage : public ::libmaus2::parallel::ThreadWorkPackage
{
	BamThreadPoolDecodeContextBase * contextbase;

	libmaus2::lz::BgzfInflateBase::BaseBlockInfo blockmeta; // block size compressed and uncompressed
	uint64_t baseid;
	uint64_t blockid;

	BamThreadPoolDecodeDecompressPackage() : contextbase(0) {}
	BamThreadPoolDecodeDecompressPackage(
		uint64_t const rpackageid,
		BamThreadPoolDecodeContextBase * rcontextbase,
		libmaus2::lz::BgzfInflateBase::BaseBlockInfo rblockmeta,
		uint64_t rbaseid,
		uint64_t rblockid

	)
	: ::libmaus2::parallel::ThreadWorkPackage(
		BamThreadPoolDecodeContextBase::bamthreadpooldecodecontextbase_dispatcher_priority_decompress,
		BamThreadPoolDecodeContextBase::bamthreadpooldecodecontextbase_dispatcher_id_decompress,
		rpackageid
	), contextbase(rcontextbase), blockmeta(rblockmeta), baseid(rbaseid), blockid(rblockid)
	{

	}

	virtual ::libmaus2::parallel::ThreadWorkPackage::unique_ptr_type uclone() const
	{
		::libmaus2::parallel::ThreadWorkPackage::unique_ptr_type tptr(new BamThreadPoolDecodeDecompressPackage(packageid,contextbase,blockmeta,baseid,blockid));
		return UNIQUE_PTR_MOVE(tptr);
	}
	virtual ::libmaus2::parallel::ThreadWorkPackage::shared_ptr_type sclone() const
	{
		::libmaus2::parallel::ThreadWorkPackage::shared_ptr_type sptr(new BamThreadPoolDecodeDecompressPackage(packageid,contextbase,blockmeta,baseid,blockid));
		return sptr;
	}
};


struct BamThreadPoolDecodeBamParsePackage : public ::libmaus2::parallel::ThreadWorkPackage
{
	BamThreadPoolDecodeContextBase * contextbase;

	libmaus2::lz::BgzfInflateBase::BaseBlockInfo blockmeta; // block size compressed and uncompressed
	uint64_t baseid;
	uint64_t blockid;

	BamThreadPoolDecodeBamParsePackage() : contextbase(0) {}
	BamThreadPoolDecodeBamParsePackage(
		uint64_t const rpackageid,
		BamThreadPoolDecodeContextBase * rcontextbase,
		libmaus2::lz::BgzfInflateBase::BaseBlockInfo rblockmeta,
		uint64_t rbaseid,
		uint64_t rblockid
	)
	: ::libmaus2::parallel::ThreadWorkPackage(
		BamThreadPoolDecodeContextBase::bamthreadpooldecodecontextbase_dispatcher_priority_bamparse,
		BamThreadPoolDecodeContextBase::bamthreadpooldecodecontextbase_dispatcher_id_bamparse,
		rpackageid
	), contextbase(rcontextbase), blockmeta(rblockmeta), baseid(rbaseid), blockid(rblockid)
	{

	}

	virtual ::libmaus2::parallel::ThreadWorkPackage::unique_ptr_type uclone() const
	{
		::libmaus2::parallel::ThreadWorkPackage::unique_ptr_type tptr(new BamThreadPoolDecodeBamParsePackage(packageid,contextbase,blockmeta,baseid,blockid));
		return UNIQUE_PTR_MOVE(tptr);
	}
	virtual ::libmaus2::parallel::ThreadWorkPackage::shared_ptr_type sclone() const
	{
		::libmaus2::parallel::ThreadWorkPackage::shared_ptr_type sptr(new BamThreadPoolDecodeBamParsePackage(packageid,contextbase,blockmeta,baseid,blockid));
		return sptr;
	}
};

struct BamThreadPoolDecodeBamProcessPackage : public ::libmaus2::parallel::ThreadWorkPackage
{
	BamThreadPoolDecodeContextBase * contextbase;
	uint64_t baseid;
	uint64_t blockid;

	BamThreadPoolDecodeBamProcessPackage() : contextbase(0) {}
	BamThreadPoolDecodeBamProcessPackage(
		uint64_t const rpackageid,
		BamThreadPoolDecodeContextBase * rcontextbase,
		uint64_t rbaseid,
		uint64_t rblockid
	)
	: ::libmaus2::parallel::ThreadWorkPackage(
		BamThreadPoolDecodeContextBase::bamthreadpooldecodecontextbase_dispatcher_priority_bamprocess,
		BamThreadPoolDecodeContextBase::bamthreadpooldecodecontextbase_dispatcher_id_bamprocess,
		rpackageid
	), contextbase(rcontextbase), baseid(rbaseid), blockid(rblockid)
	{

	}

	virtual ::libmaus2::parallel::ThreadWorkPackage::unique_ptr_type uclone() const
	{
		::libmaus2::parallel::ThreadWorkPackage::unique_ptr_type tptr(new BamThreadPoolDecodeBamProcessPackage(packageid,contextbase,baseid,blockid));
		return UNIQUE_PTR_MOVE(tptr);
	}
	virtual ::libmaus2::parallel::ThreadWorkPackage::shared_ptr_type sclone() const
	{
		::libmaus2::parallel::ThreadWorkPackage::shared_ptr_type sptr(new BamThreadPoolDecodeBamProcessPackage(packageid,contextbase,baseid,blockid));
		return sptr;
	}
};

struct BamThreadPoolDecodeReadPackageDispatcher : public libmaus2::parallel::ThreadWorkPackageDispatcher
{
	virtual ~BamThreadPoolDecodeReadPackageDispatcher() {}
	virtual void dispatch(
		libmaus2::parallel::ThreadWorkPackage * P,
		libmaus2::parallel::ThreadPoolInterfaceEnqueTermInterface & tpi
	)
	{
		assert ( dynamic_cast<BamThreadPoolDecodeReadPackage *>(P) != 0 );

		BamThreadPoolDecodeReadPackage & RP = *dynamic_cast<BamThreadPoolDecodeReadPackage *>(P);
		BamThreadPoolDecodeContextBase & contextbase = *(RP.contextbase);
		libmaus2::parallel::PosixMutex & inputLock = contextbase.inputLock;
		libmaus2::parallel::ScopePosixMutex slock(inputLock);

		// handle empty input file
		if (
			contextbase.readCnt == 0 &&
			contextbase.in.peek() == std::istream::traits_type::eof()
		)
		{
			contextbase.readComplete.set(true);
			contextbase.decompressComplete.set(true);
			contextbase.bamParseComplete.set(true);
			contextbase.bamProcessComplete.set(true);
			tpi.terminate();
		}

		if ( ! contextbase.readComplete.get() )
		{
			assert ( contextbase.in.peek() != std::istream::traits_type::eof() );

			uint64_t const nextInputBlockId = (contextbase.nextInputBlockId++);
			uint64_t const baseid = contextbase.inflateBasesFreeList.deque();

			libmaus2::lz::BgzfInflateBase::BaseBlockInfo const blockmeta = contextbase.inflateBases[baseid]->readBlock(contextbase.in);
			contextbase.readCnt += 1;

			#if 0
			cerrmutex.lock();
			std::cerr << "got block " << nextInputBlockId << "\t(" << blockmeta.first << "," << blockmeta.second << ")" << std::endl;
			cerrmutex.unlock();
			#endif

			if ( contextbase.in.peek() == std::istream::traits_type::eof() )
			{
				contextbase.readComplete.set(true);
				cerrmutex.lock();
				std::cerr << "read complete." << std::endl;
				cerrmutex.unlock();
			}

			BamThreadPoolDecodeDecompressPackage const BTPDDP(
				contextbase.getNextPackageId(),
				RP.contextbase,
				blockmeta,
				baseid,
				nextInputBlockId
			);

			tpi.enque(BTPDDP);
		}
	}
};

struct BamThreadPoolDecodeDecompressPackageDispatcher : public libmaus2::parallel::ThreadWorkPackageDispatcher
{
	virtual ~BamThreadPoolDecodeDecompressPackageDispatcher() {}
	virtual void dispatch(
		libmaus2::parallel::ThreadWorkPackage * P,
		libmaus2::parallel::ThreadPoolInterfaceEnqueTermInterface & tpi
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

		libmaus2::parallel::ScopePosixMutex ldecompressLock(contextbase.decompressLock);
		contextbase.decompressCnt += 1;
		if ( contextbase.readComplete.get() && (contextbase.decompressCnt == contextbase.readCnt) )
		{
			contextbase.decompressComplete.set(true);

			#if 1
			cerrmutex.lock();
			std::cerr << "decompress complete." << std::endl;
			cerrmutex.unlock();
			#endif
		}

		{
			// generate parse info object
			BamThreadPoolDecodeBamParseQueueInfo qinfo(contextbase.getNextPackageId(),
				RP.blockmeta,RP.baseid,RP.blockid);

			// enque the parse info object
			libmaus2::parallel::ScopePosixMutex lbamParseQueueLock(contextbase.bamParseQueueLock);
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

struct BamThreadPoolDecodeBamParsePackageDispatcher : public libmaus2::parallel::ThreadWorkPackageDispatcher
{
	template<unsigned int len>
	static uint64_t decodeLittleEndianInteger(uint8_t const * pa)
	{
		uint64_t v = 0;
		for ( unsigned int shift = 0; shift < 8*len; shift += 8 )
			v |= static_cast<uint64_t>(*(pa++)) << shift;
		return v;
	}

	virtual ~BamThreadPoolDecodeBamParsePackageDispatcher() {}
	virtual void dispatch(
		libmaus2::parallel::ThreadWorkPackage * P,
		libmaus2::parallel::ThreadPoolInterfaceEnqueTermInterface & tpi
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

		char * const decompressSpace = contextbase.getDecompressSpace(RP.baseid);

		uint8_t const * pa = reinterpret_cast<uint8_t const *>(decompressSpace);
		uint8_t const * pc = pa + RP.blockmeta.uncompdatasize;

		if ( (! contextbase.haveheader.get()) && (pa != pc) )
		{
			::libmaus2::util::GetObject<uint8_t const *> G(pa);
			std::pair<bool,uint64_t> const P = contextbase.bamheaderparsestate.parseHeader(G,pc-pa);

			// header complete?
			if ( P.first )
			{
				contextbase.header.init(contextbase.bamheaderparsestate);
				contextbase.haveheader.set(true);
				pa += P.second;

				cerrmutex.lock();
				std::cerr << contextbase.header.text;
				cerrmutex.unlock();
			}
			else
			{
				pa += P.second;
			}
		}

		bool bufferAvailable = !(contextbase.processBuffersFreeList.empty());
		int64_t freeBufferId = bufferAvailable ? contextbase.processBuffersFreeList.dequeFront() : -1;
		std::vector<uint64_t> finishedBuffers;

		#if 0
		cerrmutex.lock();
		std::cerr << "bamparse using free buffer id " << freeBufferId << std::endl;
		cerrmutex.unlock();
		#endif

		bool stall = freeBufferId < 0;
		bool running = (pa != pc) && (!stall);

		while ( running )
		{
			assert ( contextbase.haveheader.get() );
			BamProcessBuffer * bamProcessBuffer = contextbase.processBuffers[freeBufferId].get();

			switch ( contextbase.bamParserState )
			{
				case BamThreadPoolDecodeContextBase::bam_parser_state_read_blocklength:
				{
					if ( contextbase.bamBlockSizeRead == 0 )
					{
						uint32_t lblocksize;
						while ( (pc-pa >= 4) && ((pc-pa) >= (4+(lblocksize=decodeLittleEndianInteger<4>(pa)) )) )
						{
							#if 0
							cerrmutex.lock();
							std::cerr << "lblocksize=" << lblocksize << std::endl;
							cerrmutex.unlock();
							#endif
							if ( bamProcessBuffer->put(pa+4,lblocksize) )
							{
								pa += 4 + lblocksize;
							}
							else
							{
								finishedBuffers.push_back(freeBufferId);

								bufferAvailable = !(contextbase.processBuffersFreeList.empty());
								freeBufferId = bufferAvailable ? contextbase.processBuffersFreeList.dequeFront() : -1;

								if ( freeBufferId < 0 )
								{
									running = false;
									stall = true;
									break;
								}
								else
								{
									bamProcessBuffer = contextbase.processBuffers[freeBufferId].get();
								}
							}
						}
					}
					while ( running && (pa != pc) && (contextbase.bamBlockSizeRead < 4) )
					{
						contextbase.bamBlockSizeValue |= static_cast<uint32_t>(*(pa++)) << (8*((contextbase.bamBlockSizeRead++)));
					}
					if ( running && (contextbase.bamBlockSizeRead == 4) )
					{
						contextbase.bamParserState = contextbase.bam_parser_state_read_blockdata;
						contextbase.bamBlockDataRead = 0;

						#if 0
						cerrmutex.lock();
						std::cerr << "lblocksize2=" << contextbase.bamBlockSizeValue << std::endl;
						cerrmutex.unlock();
						#endif

						if ( contextbase.bamBlockSizeValue > contextbase.bamGatherBuffer.D.size() )
							contextbase.bamGatherBuffer.D = libmaus2::bambam::BamAlignment::D_array_type(contextbase.bamBlockSizeValue);
					}
					break;
				}
				case BamThreadPoolDecodeContextBase::bam_parser_state_read_blockdata:
				{
					assert ( pa != pc );

					uint64_t const skip =
						std::min(
							static_cast<uint64_t>(pc-pa),
							static_cast<uint64_t>(contextbase.bamBlockSizeValue - contextbase.bamBlockDataRead)
						);

					std::copy(pa,pa+skip,contextbase.bamGatherBuffer.D.begin()+contextbase.bamBlockDataRead);

					contextbase.bamBlockDataRead += skip;
					pa += skip;

					if ( contextbase.bamBlockDataRead == contextbase.bamBlockSizeValue )
					{
						contextbase.bamGatherBuffer.blocksize = contextbase.bamBlockSizeValue;

						if ( bamProcessBuffer->put(
							contextbase.bamGatherBuffer.D.begin(),
							contextbase.bamGatherBuffer.blocksize)
						)
						{
							contextbase.bamBlockSizeRead = 0;
							contextbase.bamBlockSizeValue = 0;
							contextbase.bamParserState = contextbase.bam_parser_state_read_blocklength;
						}
						else
						{
							// queue finished buffer
							finishedBuffers.push_back(freeBufferId);

							// rewind to process this data again
							pa -= skip;
							contextbase.bamBlockDataRead -= skip;

							// check for next free buffer
							bufferAvailable = !(contextbase.processBuffersFreeList.empty());
							freeBufferId = bufferAvailable ? contextbase.processBuffersFreeList.dequeFront() : -1;

							// no more free buffers, stall
							if ( freeBufferId < 0 )
							{
								stall = true;
								running = false;
							}
							else
								bamProcessBuffer = contextbase.processBuffers[freeBufferId].get();
						}
					}
					break;
				}
			}

			running = running && (pa != pc) && (!stall);
		}

		if ( ! stall )
		{
			// add inflate object to free list if it is no longer required
			contextbase.inflateBasesFreeList.enque(RP.baseid);
			BamThreadPoolDecodeReadPackage const BTPDRP(contextbase.getNextPackageId(),RP.contextbase);
			tpi.enque(BTPDRP);
		}

		if ( ! stall )
		{

			libmaus2::parallel::ScopePosixMutex lbamParseLock(contextbase.bamParseLock);
			contextbase.bamParseCnt += 1;
			if ( contextbase.decompressComplete.get() && (contextbase.bamParseCnt == contextbase.decompressCnt) )
			{
				cerrmutex.lock();
				std::cerr << "bamParse complete." << std::endl;
				cerrmutex.unlock();
				contextbase.bamParseComplete.set(true);

				// queue final finished buffer
				finishedBuffers.push_back(freeBufferId);
			}
			else
			{
				// reinsert unfinished buffer
				contextbase.processBuffersFreeList.push_front(freeBufferId);
			}
		}

		if ( stall )
		{
			// move remaining data to start of buffer
			memmove(reinterpret_cast<uint8_t *>(decompressSpace),pa,pc-pa);

			RP.blockmeta.uncompdatasize = pc-pa;

			#if 0
			cerrmutex.lock();
			std::cerr << "stalling, rest " << pc-pa << std::endl;
			cerrmutex.unlock();
			#endif

			// stall package
			BamThreadPoolDecodeBamParseQueueInfo qinfo(
				RP.packageid,
				RP.blockmeta,
				RP.baseid,
				RP.blockid
			);

			// enque the parse info object in the stall queue
			libmaus2::parallel::ScopePosixMutex lbamParseQueueLock(contextbase.bamParseQueueLock);
			contextbase.bamparseStall.push(qinfo);
		}

		// queue finished buffers
		if ( finishedBuffers.size() )
		{
			for ( uint64_t i = 0; i < finishedBuffers.size(); ++i )
			{
				BamThreadPoolDecodeBamProcessQueueInfo qinfo(
					contextbase.getNextPackageId(),
					finishedBuffers[i],
					contextbase.nextProcessBufferIdIn++
				);

				BamThreadPoolDecodeBamProcessPackage procpack(
					qinfo.packageid,
					RP.contextbase,
					qinfo.baseid,
					qinfo.blockid
				);

				tpi.enque(procpack);
			}
		}

		#if 0
		cerrmutex.lock();
		std::cerr << "parse finished " << finishedBuffers.size() << " buffers" << std::endl;
		cerrmutex.unlock();
		#endif

		if ( ! stall )
		{
			// process next bam parse object if any is available
			{
				libmaus2::parallel::ScopePosixMutex lbamParseQueueLock(contextbase.bamParseQueueLock);
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
	}
};


struct BamThreadPoolDecodeBamProcessPackageDispatcher : public libmaus2::parallel::ThreadWorkPackageDispatcher
{
	virtual ~BamThreadPoolDecodeBamProcessPackageDispatcher() {}
	virtual void dispatch(
		libmaus2::parallel::ThreadWorkPackage * P,
		libmaus2::parallel::ThreadPoolInterfaceEnqueTermInterface & tpi
	)
	{
		assert ( dynamic_cast<BamThreadPoolDecodeBamProcessPackage *>(P) != 0 );

		BamThreadPoolDecodeBamProcessPackage & RP = *dynamic_cast<BamThreadPoolDecodeBamProcessPackage *>(P);
		BamThreadPoolDecodeContextBase & contextbase = *(RP.contextbase);

		#if 0
		cerrmutex.lock();
		std::cerr << "bamprocess processing " << RP.blockid << std::endl;
		cerrmutex.unlock();
		#endif

		BamProcessBuffer * processBuffer = contextbase.processBuffers[RP.baseid].get();
		std::reverse(processBuffer->pc,processBuffer->pa);
		libmaus2::bambam::BamAlignmentPosComparator BAPC(processBuffer->ca);
		std::stable_sort(processBuffer->pc,processBuffer->pa,BAPC);

		#if 0
		cerrmutex.lock();
		std::cerr << "sorted " << (processBuffer->pa-processBuffer->pc) << std::endl;
		cerrmutex.unlock();
		#endif

		contextbase.processBuffers[RP.baseid]->reset();
		contextbase.processBuffersFreeList.push_back(RP.baseid);

		{
		libmaus2::parallel::ScopePosixMutex lnextProcessBufferIdOutLock(contextbase.nextProcessBufferIdOutLock);
		contextbase.nextProcessBufferIdOut += 1;
		}

		{
			libmaus2::parallel::ScopePosixMutex lbamParseQueueLock(contextbase.bamParseQueueLock);

			if ( contextbase.bamparseStall.size() )
			{
				BamThreadPoolDecodeBamParseQueueInfo const bqinfo = contextbase.bamparseStall.top();
				contextbase.bamparseStall.pop();

				BamThreadPoolDecodeBamParsePackage BTPDBPP(
					bqinfo.packageid,
					RP.contextbase,
					bqinfo.blockmeta,
					bqinfo.baseid,
					bqinfo.blockid
				);

				cerrmutex.lock();
				std::cerr << "reinserting stalled block " << bqinfo.blockid << std::endl;
				cerrmutex.unlock();

				tpi.enque(BTPDBPP);
			}
		}

		{
		libmaus2::parallel::ScopePosixMutex lnextProcessBufferIdOutLock(contextbase.nextProcessBufferIdOutLock);
		#if 0
		cerrmutex.lock();
		std::cerr
			<< "in=" << contextbase.nextProcessBufferIdIn << " out=" << contextbase.nextProcessBufferIdOut
			<< " parseComplete=" << contextbase.bamParseComplete.get()
			<< std::endl;
		cerrmutex.unlock();
		#endif
		if (
			contextbase.bamParseComplete.get() && (contextbase.nextProcessBufferIdOut == contextbase.nextProcessBufferIdIn)
		)
		{
			contextbase.bamProcessComplete.set(true);
			std::cerr << "bamProcess complete" << std::endl;
			tpi.terminate();
		}
		}
	}
};

struct BamThreadPoolDecodeContext : public BamThreadPoolDecodeContextBase
{
	libmaus2::parallel::ThreadPool & TP;

	BamThreadPoolDecodeContext(
		std::istream & rin,
		uint64_t const numInflateBases,
		uint64_t const numProcessBuffers,
		uint64_t const processBufferSize,
		libmaus2::parallel::ThreadPool & rTP
	)
	: BamThreadPoolDecodeContextBase(rin,numInflateBases,numProcessBuffers,processBufferSize), TP(rTP)
	{

	}

	void startup()
	{
		for ( uint64_t i = 0; i < BamThreadPoolDecodeContextBase::inflateBases.size(); ++i )
		{
			BamThreadPoolDecodeReadPackage const BTPDRP(BamThreadPoolDecodeContextBase::getNextPackageId(),this);
			TP.enque(BTPDRP);
		}
	}
};

#include <libmaus2/aio/PosixFdInputStream.hpp>

int main()
{
	#if defined(_OPENMP)
	uint64_t const numthreads = omp_get_max_threads();
	#else
	uint64_t const numthreads = 1;
	#endif

	std::cerr << "numthreads=" << numthreads << std::endl;

	libmaus2::parallel::ThreadPool TP(numthreads);

	BamThreadPoolDecodeReadPackageDispatcher readdispatcher;
	TP.registerDispatcher(BamThreadPoolDecodeContext::bamthreadpooldecodecontextbase_dispatcher_id_read,&readdispatcher);
	BamThreadPoolDecodeDecompressPackageDispatcher decompressdispatcher;
	TP.registerDispatcher(BamThreadPoolDecodeContext::bamthreadpooldecodecontextbase_dispatcher_id_decompress,&decompressdispatcher);
	BamThreadPoolDecodeBamParsePackageDispatcher bamparsedispatcher;
	TP.registerDispatcher(BamThreadPoolDecodeContext::bamthreadpooldecodecontextbase_dispatcher_id_bamparse,&bamparsedispatcher);
	BamThreadPoolDecodeBamProcessPackageDispatcher bamprocessdispatcher;
	TP.registerDispatcher(BamThreadPoolDecodeContext::bamthreadpooldecodecontextbase_dispatcher_id_bamprocess,&bamprocessdispatcher);

	uint64_t numProcessBuffers = 2*numthreads;
	uint64_t processBufferSize = 64*1024*1024;

	libmaus2::aio::PosixFdInputStream PFIS(STDIN_FILENO,256*1024);
	BamThreadPoolDecodeContext context(PFIS,8*numthreads,numProcessBuffers,processBufferSize,TP);
	context.startup();

#if 0
	libmaus2::parallel::DummyThreadWorkPackageMeta meta;
	libmaus2::parallel::PosixMutex::shared_ptr_type printmutex(new libmaus2::parallel::PosixMutex);

	TP.enque(libmaus2::parallel::DummyThreadWorkPackage(0,dispid,printmutex,&meta));
#endif

	TP.join();
}
