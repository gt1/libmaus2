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
#if ! defined(LIBMAUS_BAMBAM_BAMDECOMPRESSIONCONTROL_HPP)
#define LIBMAUS_BAMBAM_BAMDECOMPRESSIONCONTROL_HPP

#include <libmaus/bambam/BamAlignment.hpp>
#include <libmaus/bambam/BamAlignmentDecoderBase.hpp>
#include <libmaus/bambam/BamHeaderParserState.hpp>
#include <libmaus/bambam/BamHeaderLowMem.hpp>
#include <libmaus/lz/BgzfInflateBase.hpp>
#include <libmaus/lz/BgzfInflateHeaderBase.hpp>
#include <libmaus/parallel/LockedCounter.hpp>
#include <libmaus/parallel/LockedFreeList.hpp>
#include <libmaus/parallel/LockedGrowingFreeList.hpp>
#include <libmaus/parallel/LockedHeap.hpp>
#include <libmaus/parallel/LockedQueue.hpp>
#include <libmaus/parallel/PosixMutex.hpp>
#include <libmaus/parallel/PosixSpinLock.hpp>
#include <libmaus/parallel/SimpleThreadPool.hpp>
#include <libmaus/parallel/SimpleThreadPoolWorkPackageFreeList.hpp>
#include <libmaus/parallel/SimpleThreadWorkPackage.hpp>
#include <libmaus/parallel/SimpleThreadWorkPackageDispatcher.hpp>
#include <libmaus/parallel/SynchronousCounter.hpp>
#include <libmaus/sorting/ParallelStableSort.hpp>
#include <libmaus/timing/RealTimeClock.hpp>
#include <libmaus/util/FreeList.hpp>
#include <libmaus/util/GetObject.hpp>
#include <libmaus/util/GrowingFreeList.hpp>
#include <libmaus/lz/CompressorObjectFreeListAllocator.hpp>
#include <libmaus/lz/SnappyCompressorObjectFreeListAllocator.hpp>
#include <libmaus/lz/ZlibCompressorObjectFreeListAllocator.hpp>

#include <stack>
#include <csignal>

namespace libmaus
{
	namespace bambam
	{
		struct BamParallelDecodingInputBlock
		{
			typedef BamParallelDecodingInputBlock this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
		
			//! input data
			libmaus::lz::BgzfInflateHeaderBase inflateheaderbase;
			//! size of block payload
			uint64_t volatile payloadsize;
			//! compressed data
			libmaus::autoarray::AutoArray<char> C;
			//! size of decompressed data
			uint64_t volatile uncompdatasize;
			//! true if this is the last block in the stream
			bool volatile final;
			//! stream id
			uint64_t volatile streamid;
			//! block id
			uint64_t volatile blockid;
			//! crc32
			uint32_t volatile crc;
			
			BamParallelDecodingInputBlock() 
			: 
				inflateheaderbase(),
				payloadsize(0),
				C(libmaus::lz::BgzfConstants::getBgzfMaxBlockSize(),false), 
				uncompdatasize(0),
				final(false) ,
				streamid(0),
				blockid(0),
				crc(0)
			{}
			
			template<typename stream_type>
			void readBlock(stream_type & stream)
			{
				payloadsize = inflateheaderbase.readHeader(stream);
				
				// read block
				stream.read(C.begin(),payloadsize + 8);

				if ( stream.gcount() != static_cast<int64_t>(payloadsize + 8) )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "BamParallelDecodingInputBlock::readBlock(): unexpected eof" << std::endl;
					se.finish(false);
					throw se;
				}
				
				crc = 
					(static_cast<uint32_t>(static_cast<uint8_t>(C[payloadsize+0])) <<  0) |
					(static_cast<uint32_t>(static_cast<uint8_t>(C[payloadsize+1])) <<  8) |
					(static_cast<uint32_t>(static_cast<uint8_t>(C[payloadsize+2])) << 16) |
					(static_cast<uint32_t>(static_cast<uint8_t>(C[payloadsize+3])) << 24)
				;
								
				uncompdatasize = 
					(static_cast<uint64_t>(static_cast<uint8_t>(C[payloadsize+4])) << 0)
					|
					(static_cast<uint64_t>(static_cast<uint8_t>(C[payloadsize+5])) << 8)
					|
					(static_cast<uint64_t>(static_cast<uint8_t>(C[payloadsize+6])) << 16)
					|
					(static_cast<uint64_t>(static_cast<uint8_t>(C[payloadsize+7])) << 24);

				if ( uncompdatasize > libmaus::lz::BgzfConstants::getBgzfMaxBlockSize() )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "BamParallelDecodingInputBlock::readBlock(): uncompressed size is too large";
					se.finish(false);
					throw se;									
				}
				
				final = (uncompdatasize == 0) && (stream.peek() < 0);
			}
		};

		struct BamParallelDecodingControlInputInfo
		{
			typedef BamParallelDecodingInputBlock input_block_type;
		
			libmaus::parallel::PosixSpinLock readLock;
			std::istream & istr;
			bool volatile eof;
			uint64_t volatile streamid;
			uint64_t volatile blockid;
			libmaus::parallel::LockedFreeList<input_block_type> inputBlockFreeList;

			BamParallelDecodingControlInputInfo(
				std::istream & ristr, 
				uint64_t const rstreamid,
				uint64_t const rfreelistsize
			) : istr(ristr), eof(false), streamid(rstreamid), blockid(0), inputBlockFreeList(rfreelistsize) {}
			
			bool getEOF()
			{
				libmaus::parallel::ScopePosixSpinLock llock(readLock);
				return eof;
			}
		};
		
		struct BamParallelDecodingInputBlockWorkPackage : public libmaus::parallel::SimpleThreadWorkPackage
		{
			typedef BamParallelDecodingInputBlockWorkPackage this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
		
			BamParallelDecodingControlInputInfo * inputinfo;

			BamParallelDecodingInputBlockWorkPackage()
			: 
				libmaus::parallel::SimpleThreadWorkPackage(), 
				inputinfo(0)
			{
			}
			
			BamParallelDecodingInputBlockWorkPackage(
				uint64_t const rpriority, BamParallelDecodingControlInputInfo * rinputinfo,
				uint64_t const rreadDispatcherId
			)
			: libmaus::parallel::SimpleThreadWorkPackage(rpriority,rreadDispatcherId), inputinfo(rinputinfo)
			{
			}
		
			char const * getPackageName() const
			{
				return "BamParallelDecodingInputBlockWorkPackage";
			}
		};
		
		// return input block work package
		struct BamParallelDecodingInputBlockWorkPackageReturnInterface
		{
			virtual ~BamParallelDecodingInputBlockWorkPackageReturnInterface() {}
			virtual void putBamParallelDecodingInputBlockWorkPackage(BamParallelDecodingInputBlockWorkPackage * package) = 0;
		};

		// add input block pending decompression
		struct BamParallelDecodingInputBlockAddPendingInterface
		{
			virtual ~BamParallelDecodingInputBlockAddPendingInterface() {}
			virtual void putBamParallelDecodingInputBlockAddPending(BamParallelDecodingControlInputInfo::input_block_type * package) = 0;
		};
		
		/**
		 * compressed block input dispatcher
		 **/
		struct BamParallelDecodingInputBlockWorkPackageDispatcher : public libmaus::parallel::SimpleThreadWorkPackageDispatcher
		{
			BamParallelDecodingInputBlockWorkPackageReturnInterface & returnInterface;
			BamParallelDecodingInputBlockAddPendingInterface & addPending;
			
			BamParallelDecodingInputBlockWorkPackageDispatcher(
				BamParallelDecodingInputBlockWorkPackageReturnInterface & rreturnInterface,
				BamParallelDecodingInputBlockAddPendingInterface & raddPending
			) : returnInterface(rreturnInterface), addPending(raddPending)
			{
			
			}
		
			virtual void dispatch(
				libmaus::parallel::SimpleThreadWorkPackage * P, 
				libmaus::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & tpi
			)
			{
				// get type cast work package pointer
				BamParallelDecodingInputBlockWorkPackage * BP = dynamic_cast<BamParallelDecodingInputBlockWorkPackage *>(P);
				assert ( BP );
				
				// vector of blocks to pass on for decompression
				std::vector<BamParallelDecodingControlInputInfo::input_block_type *> blockPassVector;
				
				// try to get lock
				if ( BP->inputinfo->readLock.trylock() )
				{
					// free lock at end of scope
					libmaus::parallel::ScopePosixSpinLock llock(BP->inputinfo->readLock,true /* pre locked */);

					// do not run if eof is already set
					bool running = !BP->inputinfo->eof;
					
					while ( running )
					{
						// try to get a free buffer
						BamParallelDecodingControlInputInfo::input_block_type * block = 
							BP->inputinfo->inputBlockFreeList.getIf();
						
						// if we have a buffer then read the next bgzf block
						if ( block )
						{
							// read
							block->readBlock(BP->inputinfo->istr);
							// copy stream id
							block->streamid = BP->inputinfo->streamid;
							// set next block id
							block->blockid = BP->inputinfo->blockid++;
							
							if ( block->final )
							{
								running = false;
								BP->inputinfo->eof = true;
							}

							blockPassVector.push_back(block);
						}
						else
						{
							running = false;
						}
					}
				}

				// return the work package
				returnInterface.putBamParallelDecodingInputBlockWorkPackage(BP);

				// enque decompress requests
				for ( uint64_t i = 0; i < blockPassVector.size(); ++i )
					addPending.putBamParallelDecodingInputBlockAddPending(blockPassVector[i]);
			}		
		};
		
		struct BamParallelDecodingDecompressedBlock
		{
			typedef BamParallelDecodingDecompressedBlock this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

			//! decompressed data
			libmaus::autoarray::AutoArray<char> D;
			//! size of uncompressed data
			uint64_t uncompdatasize;
			//! next byte pointer
			char const * P;
			//! true if this is the last block in the stream
			bool final;
			//! stream id
			uint64_t streamid;
			//! block id
			uint64_t blockid;
			
			BamParallelDecodingDecompressedBlock() 
			: 
				D(libmaus::lz::BgzfConstants::getBgzfMaxBlockSize(),false), 
				uncompdatasize(0), 
				P(0),
				final(false),
				streamid(0),
				blockid(0)
			{}
			
			/**
			 * return true iff stored crc matches computed crc
			 **/
			uint32_t computeCrc() const
			{
				uint32_t crc = crc32(0L,Z_NULL,0);
				crc = crc32(crc,reinterpret_cast<Bytef const*>(P),uncompdatasize);
				return crc;
			}

			uint64_t decompressBlock(
				libmaus::lz::BgzfInflateZStreamBase * decoder,
				char * in,
				unsigned int const inlen,
				unsigned int const outlen
			)
			{
				decoder->zdecompress(reinterpret_cast<uint8_t *>(in),inlen,D.begin(),outlen);
				return outlen;
			}

			uint64_t decompressBlock(
				libmaus::lz::BgzfInflateZStreamBase * decoder,
				BamParallelDecodingInputBlock * inblock
			)
			{
				uncompdatasize = inblock->uncompdatasize;
				final = inblock->final;
				P = D.begin();
				return decompressBlock(decoder,inblock->C.begin(),inblock->payloadsize,uncompdatasize);
			}

			uint64_t decompressBlock(
				libmaus::parallel::LockedFreeList<libmaus::lz::BgzfInflateZStreamBase> & deccont,
				char * in,
				unsigned int const inlen,
				unsigned int const outlen
			)
			{
				libmaus::lz::BgzfInflateZStreamBase * decoder = deccont.get();				
				uint64_t const r = decompressBlock(decoder,in,inlen,outlen);
				deccont.put(decoder);
				return r;
			}
			
			uint64_t decompressBlock(
				libmaus::parallel::LockedFreeList<libmaus::lz::BgzfInflateZStreamBase> & deccont,
				BamParallelDecodingInputBlock & inblock
			)
			{
				uncompdatasize = inblock.uncompdatasize;
				final = inblock.final;
				P = D.begin();
				return decompressBlock(deccont,inblock.C.begin(),inblock.payloadsize,uncompdatasize);
			}
		};

		struct BamParallelDecodingDecompressBlockWorkPackage : public libmaus::parallel::SimpleThreadWorkPackage
		{
			typedef BamParallelDecodingDecompressBlockWorkPackage this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
		
			BamParallelDecodingControlInputInfo::input_block_type * inputblock;
			BamParallelDecodingDecompressedBlock * outputblock;
			libmaus::lz::BgzfInflateZStreamBase * decoder;

			BamParallelDecodingDecompressBlockWorkPackage()
			: 
				libmaus::parallel::SimpleThreadWorkPackage(), 
				inputblock(0),
				outputblock(0),
				decoder(0)
			{
			}
			
			BamParallelDecodingDecompressBlockWorkPackage(
				uint64_t const rpriority, 
				BamParallelDecodingControlInputInfo::input_block_type * rinputblock,
				BamParallelDecodingDecompressedBlock * routputblock,
				libmaus::lz::BgzfInflateZStreamBase * rdecoder,
				uint64_t const rdecompressDispatcherId
			)
			: libmaus::parallel::SimpleThreadWorkPackage(rpriority,rdecompressDispatcherId), inputblock(rinputblock), outputblock(routputblock), decoder(rdecoder)
			{
			}
		
			char const * getPackageName() const
			{
				return "BamParallelDecodingDecompressBlockWorkPackage";
			}
		};

		// return decompress block work package
		struct BamParallelDecodingDecompressBlockWorkPackageReturnInterface
		{
			virtual ~BamParallelDecodingDecompressBlockWorkPackageReturnInterface() {}
			virtual void putBamParallelDecodingDecompressBlockWorkPackage(BamParallelDecodingDecompressBlockWorkPackage * package) = 0;
		};

		// return input block after decompression
		struct BamParallelDecodingInputBlockReturnInterface
		{
			virtual ~BamParallelDecodingInputBlockReturnInterface() {}
			virtual void putBamParallelDecodingInputBlockReturn(BamParallelDecodingControlInputInfo::input_block_type * block) = 0;
		};
		
		// add decompressed block to pending list
		struct BamParallelDecodingDecompressedBlockAddPendingInterface
		{
			virtual ~BamParallelDecodingDecompressedBlockAddPendingInterface() {}
			virtual void putBamParallelDecodingDecompressedBlockAddPending(BamParallelDecodingDecompressedBlock * block) = 0;
		};

		// return a bgzf decoder object		
		struct BamParallelDecodingBgzfInflateZStreamBaseReturnInterface
		{
			virtual ~BamParallelDecodingBgzfInflateZStreamBaseReturnInterface() {}
			virtual void putBamParallelDecodingBgzfInflateZStreamBaseReturn(libmaus::lz::BgzfInflateZStreamBase * decoder) = 0;
		};

		// dispatcher for block decompression
		struct BamParallelDecodingDecompressBlockWorkPackageDispatcher : public libmaus::parallel::SimpleThreadWorkPackageDispatcher
		{
			BamParallelDecodingDecompressBlockWorkPackageReturnInterface & packageReturnInterface;
			BamParallelDecodingInputBlockReturnInterface & inputBlockReturnInterface;
			BamParallelDecodingDecompressedBlockAddPendingInterface & decompressedBlockPendingInterface;
			BamParallelDecodingBgzfInflateZStreamBaseReturnInterface & decoderReturnInterface;

			BamParallelDecodingDecompressBlockWorkPackageDispatcher(
				BamParallelDecodingDecompressBlockWorkPackageReturnInterface & rpackageReturnInterface,
				BamParallelDecodingInputBlockReturnInterface & rinputBlockReturnInterface,
				BamParallelDecodingDecompressedBlockAddPendingInterface & rdecompressedBlockPendingInterface,
				BamParallelDecodingBgzfInflateZStreamBaseReturnInterface & rdecoderReturnInterface

			) : packageReturnInterface(rpackageReturnInterface), inputBlockReturnInterface(rinputBlockReturnInterface),
			    decompressedBlockPendingInterface(rdecompressedBlockPendingInterface), decoderReturnInterface(rdecoderReturnInterface)
			{
			
			}
		
			virtual void dispatch(
				libmaus::parallel::SimpleThreadWorkPackage * P, 
				libmaus::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & tpi
			)
			{
				BamParallelDecodingDecompressBlockWorkPackage * BP = dynamic_cast<BamParallelDecodingDecompressBlockWorkPackage *>(P);
				assert ( BP );
				
				// decompress the block
				BP->outputblock->decompressBlock(BP->decoder,BP->inputblock);

				// compute crc of uncompressed data
				uint32_t const crc = BP->outputblock->computeCrc();

				// check crc
				if ( crc != BP->inputblock->crc )
				{
					tpi.getGlobalLock().lock();
					std::cerr << "crc failed for block " << BP->inputblock->blockid 
						<< " expecting " << std::hex << BP->inputblock->crc << std::dec
						<< " got " << std::hex << crc << std::dec << std::endl;
					tpi.getGlobalLock().unlock();
					
					libmaus::exception::LibMausException lme;
					lme.getStream() << "BamParallelDecodingDecompressBlockWorkPackageDispatcher: corrupt input data (crc mismatch)\n";
					lme.finish();
					throw lme;
				}
				
				// set stream and block id
				BP->outputblock->streamid = BP->inputblock->streamid;
				BP->outputblock->blockid = BP->inputblock->blockid;

				// return zstream base (decompressor)
				decoderReturnInterface.putBamParallelDecodingBgzfInflateZStreamBaseReturn(BP->decoder);
				// return input block
				inputBlockReturnInterface.putBamParallelDecodingInputBlockReturn(BP->inputblock);
				// mark output block as pending
				decompressedBlockPendingInterface.putBamParallelDecodingDecompressedBlockAddPending(BP->outputblock);
				// return work meta package
				packageReturnInterface.putBamParallelDecodingDecompressBlockWorkPackage(BP);
			}		
		};

		struct BamParallelDecodingPushBackSpace
		{
			libmaus::util::GrowingFreeList<libmaus::bambam::BamAlignment> algnFreeList;
			std::stack<libmaus::bambam::BamAlignment *> putbackStack;

			BamParallelDecodingPushBackSpace()
			: algnFreeList(), putbackStack()
			{
			
			}
			
			void push(uint8_t const * D, uint64_t const bs)
			{
				libmaus::bambam::BamAlignment * algn = algnFreeList.get();
				algn->copyFrom(D,bs);
				putbackStack.push(algn);
			}
			
			bool empty() const
			{
				return putbackStack.empty();
			}
			
			libmaus::bambam::BamAlignment * top()
			{
				assert ( ! empty() );
				return putbackStack.top();
			}
			
			void pop()
			{
				putbackStack.pop();
			}
		};
		
		struct BamParallelDecodingAlignmentBuffer
		{
			typedef uint64_t pointer_type;
		
			// data
			libmaus::autoarray::AutoArray<uint8_t> A;
			
			// text (block data) insertion pointer
			uint8_t * pA;
			// alignment block pointer insertion marker
			pointer_type * pP;
			
			// pointer difference for insertion (insert this many pointers for each alignment)
			uint64_t volatile pointerdif;
			
			// is this the last block for the stream?
			bool volatile final;
			
			// absolute low mark of this buffer block
			uint64_t volatile low;
			
			// alignment free list
			libmaus::util::GrowingFreeList<libmaus::bambam::BamAlignment> freelist;
			
			// MQ filter (for fixmate operation)
			libmaus::bambam::BamAuxFilterVector const MQfilter;
			// MS filter 
			libmaus::bambam::BamAuxFilterVector const MSfilter;
			// MC filter 
			libmaus::bambam::BamAuxFilterVector const MCfilter;
			// MT filter 
			libmaus::bambam::BamAuxFilterVector const MTfilter;
			// MQ,MS,MC,MT filter
			libmaus::bambam::BamAuxFilterVector const MQMSMCMTfilter;
			
			std::deque<libmaus::bambam::BamAlignment *> stallBuffer;
			
			void pushFrontStallBuffer(libmaus::bambam::BamAlignment * algn)
			{
				stallBuffer.push_front(algn);
			}

			void pushBackStallBuffer(libmaus::bambam::BamAlignment * algn)
			{
				stallBuffer.push_back(algn);
			}
			
			libmaus::bambam::BamAlignment * popStallBuffer()
			{
				if ( stallBuffer.size() )
				{
					libmaus::bambam::BamAlignment * algn = stallBuffer.front();
					stallBuffer.pop_front();
					return algn;
				}
				else
				{
					return 0;
				}
			}
						
			void returnAlignments(std::vector<libmaus::bambam::BamAlignment *> & algns)
			{
				for ( uint64_t i = 0; i < algns.size(); ++i )
					returnAlignment(algns[i]);
			}
			
			void returnAlignment(libmaus::bambam::BamAlignment * algn)
			{
				freelist.put(algn);
			}
			
			uint64_t extractNextSameName(std::deque<libmaus::bambam::BamAlignment *> & algns)
			{
				uint64_t const c = nextSameName();
				
				algns.resize(c);
				
				for ( uint64_t i = 0; i < c; ++i )
				{
					algns[i] = freelist.get();
					
					uint64_t len = getNextLength();
					uint8_t const * p = getNextData();
					
					if ( len > algns[i]->D.size() )
						algns[i]->D = libmaus::bambam::BamAlignment::D_array_type(len,false);
						
					algns[i]->blocksize = len;
					memcpy(algns[i]->D.begin(),p,len);
					
					advance();
				}
				
				return c;
			}
			
			uint64_t nextSameName() const
			{
				if ( ! hasNext() )
					return 0;
				
				char const * refname = libmaus::bambam::BamAlignmentDecoderBase::getReadName(A.begin() + *pP + 4);
				uint64_t c = 1;
				
				while ( 
					((pP + c) != reinterpret_cast<pointer_type const *>(A.end())) &&
					(strcmp(libmaus::bambam::BamAlignmentDecoderBase::getReadName(A.begin() + pP[c] + 4),refname) == 0)
				)
				{
					++c;
				}
				
				return c;			
			}
			
			bool hasNext() const
			{
				return pP != reinterpret_cast<pointer_type const *>(A.end());
			}
			
			void advance(uint64_t const c = 1)
			{
				assert ( c <= (reinterpret_cast<pointer_type const *>(A.end())-pP) );
				pP += c;
			}

			uint32_t decodeLength(uint64_t const off) const
			{
				uint8_t const * B = A.begin() + off;
				
				return
					(static_cast<uint32_t>(B[0]) << 0)  |
					(static_cast<uint32_t>(B[1]) << 8)  |
					(static_cast<uint32_t>(B[2]) << 16) |
					(static_cast<uint32_t>(B[3]) << 24) ;
			}
			
			uint32_t getNextLength() const
			{
				return decodeLength(*pP);
			}

			uint8_t const * getNextData() const
			{
				return A.begin() + *pP + 4;
			}
			
			static std::vector<std::string> getMQMSMCMTFilterVector()
			{
				std::vector<std::string> V;
				V.push_back(std::string("MQ"));
				V.push_back(std::string("MS"));
				V.push_back(std::string("MC"));
				V.push_back(std::string("MT"));
				return V;
			}
			
			BamParallelDecodingAlignmentBuffer(uint64_t const buffersize, uint64_t const rpointerdif = 1)
			: A(buffersize,false), pA(A.begin()), pP(reinterpret_cast<pointer_type *>(A.end())), pointerdif(rpointerdif), final(false), low(0),
			  MQfilter(std::vector<std::string>(1,std::string("MQ"))),
			  MSfilter(std::vector<std::string>(1,std::string("MS"))),
			  MCfilter(std::vector<std::string>(1,std::string("MC"))),
			  MTfilter(std::vector<std::string>(1,std::string("MT"))),
			  MQMSMCMTfilter(getMQMSMCMTFilterVector())
			{
				assert ( pointerdif >= 1 );
			}
			
			bool empty() const
			{
				return pA == A.begin();
			}
			
			uint64_t fill() const
			{
				return (reinterpret_cast<pointer_type const *>(A.end()) - pP);
			}
			
			void reset()
			{
				low = 0;
				pA = A.begin();
				pP = reinterpret_cast<pointer_type *>(A.end());
			}
			
			uint64_t free() const
			{
				return reinterpret_cast<uint8_t *>(pP) - pA;
			}
			
			
			void reorder()
			{
				uint64_t const pref = (reinterpret_cast<pointer_type *>(A.end()) - pP) / pointerdif;
			
				// compact pointer array if pointerdif>1
				if ( pointerdif > 1 )
				{
					// start at the end
					pointer_type * i = reinterpret_cast<pointer_type *>(A.end());
					pointer_type * j = reinterpret_cast<pointer_type *>(A.end());
					
					// work back to front
					while ( i != pP )
					{
						i -= pointerdif;
						j -= 1;
						
						*j = *i;
					}
					
					// reset pointer
					pP = j;
				}

				// we filled the pointers from back to front so reverse their order
				std::reverse(pP,reinterpret_cast<pointer_type *>(A.end()));
				
				uint64_t const postf = reinterpret_cast<pointer_type *>(A.end()) - pP;
				
				assert ( pref == postf );
			}
			
			uint64_t removeLastName(BamParallelDecodingPushBackSpace & BPDPBS)
			{
				uint64_t const c = countLastName();
				uint64_t const f = fill();
				
				for ( uint64_t i = 0; i < c; ++i )
				{
					uint64_t const p = pP[f-i-1];
					BPDPBS.push(A.begin()+p+4,decodeLength(p));
				}

				// start at the end
				pointer_type * i = reinterpret_cast<pointer_type *>(A.end())-c;
				pointer_type * j = reinterpret_cast<pointer_type *>(A.end());
				
				// move pointers to back
				while ( i != pP )
				{
					--i;
					--j;
					*j = *i;
				}
				
				pP = j;
				
				return c;
			}
			
			uint64_t countLastName() const
			{
				uint64_t const f = fill();
				
				if ( f <= 1 )
					return f;
				
				char const * lastname = libmaus::bambam::BamAlignmentDecoderBase::getReadName(A.begin() + pP[f-1] + 4);
				
				uint64_t s = 1;
				
				while ( 
					s < f 
					&&
					strcmp(
						lastname,
						libmaus::bambam::BamAlignmentDecoderBase::getReadName(A.begin() + pP[f-s-1] + 4)	
					) == 0
				)
					++s;
				
				#if 0	
				for ( uint64_t i = 0; i < s; ++i )
				{
					std::cerr << "[" << i << "]=" << libmaus::bambam::BamAlignmentDecoderBase::getReadName(A.begin() + pP[f-i-1] + 4) << std::endl;
				}
				#endif
				
				if ( s != f )
				{
					assert (
						strcmp(
							lastname,
							libmaus::bambam::BamAlignmentDecoderBase::getReadName(A.begin() + pP[f-s-1] + 4)
						)
						!= 
						0
					);
				}
					
				return s;
			}
			
			bool checkValidPacked() const
			{
				uint64_t const f = fill();
				bool ok = true;
				
				for ( uint64_t i = 0; ok && i < f; ++i )
				{
					libmaus::bambam::libmaus_bambam_alignment_validity val = 
						libmaus::bambam::BamAlignmentDecoderBase::valid(
							A.begin() + pP[i] + 4,
							decodeLength(pP[i])
					);
				
					ok = ok && ( val == libmaus::bambam::libmaus_bambam_alignment_validity_ok );
				}
				
				return ok;		
			}

			bool checkValidUnpacked() const
			{
				// number of elements in buffer
				uint64_t const f = fill() / pointerdif;
				bool ok = true;
				
				for ( uint64_t i = 0; ok && i < f; ++i )
				{
					libmaus::bambam::libmaus_bambam_alignment_validity val = 
						libmaus::bambam::BamAlignmentDecoderBase::valid(
							A.begin() + pP[pointerdif*i] + 4,
							decodeLength(pP[pointerdif*i])
					);
				
					ok = ok && ( val == libmaus::bambam::libmaus_bambam_alignment_validity_ok );
				}
				
				return ok;		
			}
			
			bool put(char const * p, uint64_t const n)
			{
				if ( n + sizeof(uint32_t) + pointerdif * sizeof(pointer_type) <= free() )
				{
					// store pointer
					pP -= pointerdif;
					*pP = pA-A.begin();

					// store length
					*(pA++) = (n >>  0) & 0xFF;
					*(pA++) = (n >>  8) & 0xFF;
					*(pA++) = (n >> 16) & 0xFF;
					*(pA++) = (n >> 24) & 0xFF;
					// copy alignment data
					// std::copy(p,p+n,reinterpret_cast<char *>(pA));
					memcpy(pA,p,n);
					pA += n;
														
					return true;
				}
				else
				{
					return false;
				}
			}
		};

		struct BamParallelDecodingAlignmentRewriteBuffer
		{
			typedef uint64_t pointer_type;
		
			// data
			libmaus::autoarray::AutoArray<uint8_t> A;
			
			// text (block data) insertion pointer
			uint8_t * pA;
			// alignment block pointer insertion marker
			pointer_type * pP;
			
			// pointer difference for insertion (insert this many pointers for each alignment)
			uint64_t volatile pointerdif;
			
			uint32_t decodeLength(uint64_t const off) const
			{
				uint8_t const * B = A.begin() + off;
				
				return
					(static_cast<uint32_t>(B[0]) << 0)  |
					(static_cast<uint32_t>(B[1]) << 8)  |
					(static_cast<uint32_t>(B[2]) << 16) |
					(static_cast<uint32_t>(B[3]) << 24) ;
			}

			int64_t decodeCoordinate(uint64_t const off) const
			{
				uint8_t const * B = A.begin() + off + sizeof(uint32_t);
				
				return
					static_cast<int64_t>
					((static_cast<uint64_t>(B[0]) << 0)  |
					(static_cast<uint64_t>(B[1]) << 8)  |
					(static_cast<uint64_t>(B[2]) << 16) |
					(static_cast<uint64_t>(B[3]) << 24) |
					(static_cast<uint64_t>(B[4]) << 32) |
					(static_cast<uint64_t>(B[5]) << 40) |
					(static_cast<uint64_t>(B[6]) << 48) |
					(static_cast<uint64_t>(B[7]) << 56)) 
					
					;
			}
						
			BamParallelDecodingAlignmentRewriteBuffer(uint64_t const buffersize, uint64_t const rpointerdif = 1)
			: A(buffersize,false), pA(A.begin()), pP(reinterpret_cast<pointer_type *>(A.end())), pointerdif(rpointerdif)
			{
				assert ( pointerdif >= 1 );
			}
			
			bool empty() const
			{
				return pA == A.begin();
			}
			
			uint64_t fill() const
			{
				return (reinterpret_cast<pointer_type const *>(A.end()) - pP);
			}
			
			void reset()
			{
				pA = A.begin();
				pP = reinterpret_cast<pointer_type *>(A.end());
			}
			
			uint64_t free() const
			{
				return reinterpret_cast<uint8_t *>(pP) - pA;
			}
			
			void reorder()
			{
				uint64_t const pref = (reinterpret_cast<pointer_type *>(A.end()) - pP) / pointerdif;
			
				// compact pointer array if pointerdif>1
				if ( pointerdif > 1 )
				{
					// start at the end
					pointer_type * i = reinterpret_cast<pointer_type *>(A.end());
					pointer_type * j = reinterpret_cast<pointer_type *>(A.end());
					
					// work back to front
					while ( i != pP )
					{
						i -= pointerdif;
						j -= 1;
						
						*j = *i;
					}
					
					// reset pointer
					pP = j;
				}

				// we filled the pointers from back to front so reverse their order
				std::reverse(pP,reinterpret_cast<pointer_type *>(A.end()));
				
				uint64_t const postf = reinterpret_cast<pointer_type *>(A.end()) - pP;
				
				assert ( pref == postf );
			}
			
			std::pair<pointer_type *,uint64_t> getPointerArray()
			{
				return std::pair<pointer_type *,uint64_t>(pP,fill());
			}
			
			std::pair<pointer_type *,uint64_t> getAuxPointerArray()
			{
				uint64_t const numauxpointers = (pointerdif - 1)*fill();
				return std::pair<pointer_type *,uint64_t>(pP - numauxpointers,numauxpointers);
			}
			
			bool put(char const * p, uint64_t const n)
			{
				if ( n + sizeof(uint32_t) + pointerdif * sizeof(pointer_type) <= free() )
				{
					// store pointer
					pP -= pointerdif;
					*pP = pA-A.begin();

					// store length
					*(pA++) = (n >>  0) & 0xFF;
					*(pA++) = (n >>  8) & 0xFF;
					*(pA++) = (n >> 16) & 0xFF;
					*(pA++) = (n >> 24) & 0xFF;
					// copy alignment data
					// std::copy(p,p+n,reinterpret_cast<char *>(pA));
					memcpy(pA,p,n);
					pA += n;
														
					return true;
				}
				else
				{
					return false;
				}
			}

			bool put(char const * p, uint64_t const n, int64_t const coord)
			{
				if ( n + sizeof(uint32_t) + sizeof(uint64_t) + pointerdif * sizeof(pointer_type) <= free() )
				{
					// store pointer
					pP -= pointerdif;
					*pP = pA-A.begin();

					// store length
					*(pA++) = (n >>  0) & 0xFF;
					*(pA++) = (n >>  8) & 0xFF;
					*(pA++) = (n >> 16) & 0xFF;
					*(pA++) = (n >> 24) & 0xFF;
					
					// store coordinate
					uint64_t const ucoord = coord;
					*(pA++) = (ucoord >>  0) & 0xFF;
					*(pA++) = (ucoord >>  8) & 0xFF;
					*(pA++) = (ucoord >> 16) & 0xFF;
					*(pA++) = (ucoord >> 24) & 0xFF;
					*(pA++) = (ucoord >> 32) & 0xFF;
					*(pA++) = (ucoord >> 40) & 0xFF;
					*(pA++) = (ucoord >> 48) & 0xFF;
					*(pA++) = (ucoord >> 56) & 0xFF;
					
					// copy alignment data
					// std::copy(p,p+n,reinterpret_cast<char *>(pA));
					memcpy(pA,p,n);
					pA += n;
														
					return true;
				}
				else
				{
					return false;
				}
			}
		};

		struct BamAlignmentRewriteBufferPosComparator
		{
			BamParallelDecodingAlignmentRewriteBuffer * buffer;
			uint8_t const * text;
			
			BamAlignmentRewriteBufferPosComparator(BamParallelDecodingAlignmentRewriteBuffer * rbuffer)
			: buffer(rbuffer), text(buffer->A.begin())
			{
			
			}
			
			bool operator()(BamParallelDecodingAlignmentRewriteBuffer::pointer_type A, BamParallelDecodingAlignmentRewriteBuffer::pointer_type B) const
			{
				uint8_t const * pa = text + A + (sizeof(uint32_t) + sizeof(uint64_t));
				uint8_t const * pb = text + B + (sizeof(uint32_t) + sizeof(uint64_t));

				int32_t const refa = ::libmaus::bambam::BamAlignmentDecoderBase::getRefID(pa);
				int32_t const refb = ::libmaus::bambam::BamAlignmentDecoderBase::getRefID(pb);
			
				if ( refa != refb )
					return  static_cast<uint32_t>(refa) < static_cast<uint32_t>(refb);

				int32_t const posa = ::libmaus::bambam::BamAlignmentDecoderBase::getPos(pa);
				int32_t const posb = ::libmaus::bambam::BamAlignmentDecoderBase::getPos(pb);
				
				return posa < posb;
			}
		};

		struct BamAlignmentRewriteBufferCoordinateComparator
		{
			BamParallelDecodingAlignmentRewriteBuffer * buffer;
			
			BamAlignmentRewriteBufferCoordinateComparator(BamParallelDecodingAlignmentRewriteBuffer * rbuffer) : buffer(rbuffer) {}
			
			bool operator()(BamParallelDecodingAlignmentRewriteBuffer::pointer_type A, BamParallelDecodingAlignmentRewriteBuffer::pointer_type B) const
			{
				return buffer->decodeCoordinate(A) < buffer->decodeCoordinate(B);
			}
		};

		
		struct BamParallelDecodingParseInfo
		{
			typedef BamParallelDecodingParseInfo this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			BamParallelDecodingPushBackSpace BPDPBS;
			
			libmaus::bambam::BamHeaderParserState BHPS;
			bool volatile headerComplete;
			libmaus::bambam::BamHeaderLowMem::unique_ptr_type Pheader;

			libmaus::autoarray::AutoArray<char> concatBuffer;
			unsigned int volatile concatBufferFill;
			
			enum parser_state_type {
				parser_state_read_blocklength,
				parser_state_read_block
			};
			
			parser_state_type volatile parser_state;
			unsigned int volatile blocklengthread;
			uint32_t volatile blocklen;
			uint64_t volatile parseacc;
			
			BamParallelDecodingParseInfo()
			: BPDPBS(), BHPS(), headerComplete(false),
			  concatBuffer(), concatBufferFill(0), 
			  parser_state(parser_state_read_blocklength),
			  blocklengthread(0), blocklen(0), parseacc(0)
			{
			
			}
			
			static uint32_t getLE4(char const * A)
			{
				unsigned char const * B = reinterpret_cast<unsigned char const *>(A);
				
				return
					(static_cast<uint32_t>(B[0]) << 0)  |
					(static_cast<uint32_t>(B[1]) << 8)  |
					(static_cast<uint32_t>(B[2]) << 16) |
					(static_cast<uint32_t>(B[3]) << 24) ;
			}
			
			void putBackLastName(BamParallelDecodingAlignmentBuffer & algnbuf)
			{
				algnbuf.removeLastName(BPDPBS);
			}
			
			bool putBackBufferEmpty() const
			{
				return BPDPBS.empty();
			}

			/**
			 * parsed decompressed bam block into algnbuf
			 *
			 * @param block decompressed bam block data
			 * @param algnbuf alignment buffer
			 * @return true if block was fully processed
			 **/
			bool parseBlock(
				BamParallelDecodingDecompressedBlock & block,
				BamParallelDecodingAlignmentBuffer & algnbuf
			)
			{
				while ( ! headerComplete )
				{
					libmaus::util::GetObject<uint8_t const *> G(reinterpret_cast<uint8_t const *>(block.P));
					std::pair<bool,uint64_t> Q = BHPS.parseHeader(G,block.uncompdatasize);
					
					block.P += Q.second;
					block.uncompdatasize -= Q.second;
					
					if ( Q.first )
					{
						headerComplete = true;
						libmaus::bambam::BamHeaderLowMem::unique_ptr_type Theader(
							libmaus::bambam::BamHeaderLowMem::constructFromText(
								BHPS.text.begin(),
								BHPS.text.begin()+BHPS.l_text
							)
						);
						Pheader = UNIQUE_PTR_MOVE(Theader);						
					}
				}

				// check put back buffer
				while ( ! BPDPBS.empty() )
				{
					libmaus::bambam::BamAlignment * talgn = BPDPBS.top();
					
					if ( ! (algnbuf.put(reinterpret_cast<char const *>(talgn->D.begin()),talgn->blocksize)) )
						return false;
						
					BPDPBS.pop();
				}
				
				// concat buffer contains data
				if ( concatBufferFill )
				{
					// parser state should be reading block
					assert ( parser_state == parser_state_read_block );
					
					// number of bytes to copy
					uint64_t const tocopy = std::min(
						static_cast<uint64_t>(blocklen - concatBufferFill),
						static_cast<uint64_t>(block.uncompdatasize)
					);
					
					// make sure there is sufficient space
					if ( concatBufferFill + tocopy > concatBuffer.size() )
						concatBuffer.resize(concatBufferFill + tocopy);

					// copy bytes
					std::copy(block.P,block.P+tocopy,concatBuffer.begin()+concatBufferFill);
					
					// adjust pointers
					concatBufferFill += tocopy;
					block.uncompdatasize -= tocopy;
					block.P += tocopy;
					
					if ( concatBufferFill == blocklen )
					{
						if ( ! (algnbuf.put(concatBuffer.begin(),concatBufferFill)) )
							return false;

						concatBufferFill = 0;
						parser_state = parser_state_read_blocklength;
						blocklengthread = 0;
						blocklen = 0;
					}
				}
				
				while ( block.uncompdatasize )
				{
					switch ( parser_state )
					{
						case parser_state_read_blocklength:
						{
							while ( 
								(!blocklengthread) && 
								(block.uncompdatasize >= 4) &&
								(
									block.uncompdatasize >= 4 + (blocklen = getLE4(block.P))
								)
							)
							{
								if ( ! (algnbuf.put(block.P+4,blocklen)) )
									return false;

								// skip
								blocklengthread = 0;
								block.uncompdatasize -= (blocklen+4);
								block.P += blocklen+4;
								blocklen = 0;
							}
							
							if ( block.uncompdatasize )
							{
								while ( blocklengthread < 4 && block.uncompdatasize )
								{
									blocklen |= static_cast<uint32_t>(*reinterpret_cast<unsigned char const *>(block.P)) << (blocklengthread*8);
									block.P++;
									block.uncompdatasize--;
									blocklengthread++;
								}
								
								if ( blocklengthread == 4 )
								{
									parser_state = parser_state_read_block;
								}
							}

							break;
						}
						case parser_state_read_block:
						{
							uint64_t const tocopy = std::min(
								static_cast<uint64_t>(blocklen),
								static_cast<uint64_t>(block.uncompdatasize)
							);
							if ( concatBufferFill + tocopy > concatBuffer.size() )
								concatBuffer.resize(concatBufferFill + tocopy);
							
							std::copy(
								block.P,
								block.P+tocopy,
								concatBuffer.begin()+concatBufferFill
							);
							
							concatBufferFill += tocopy;
							block.P += tocopy;
							block.uncompdatasize -= tocopy;
							
							// handle alignment if complete
							if ( concatBufferFill == blocklen )
							{
								if ( ! (algnbuf.put(concatBuffer.begin(),concatBufferFill)) )
									return false;
				
								blocklen = 0;
								blocklengthread = 0;
								parser_state = parser_state_read_blocklength;
								concatBufferFill = 0;
							}
						
							break;
						}
					}
				}
				
				return true;
			}
		};

		struct BamParallelDecodingParseBlockWorkPackage : public libmaus::parallel::SimpleThreadWorkPackage
		{
			typedef BamParallelDecodingParseBlockWorkPackage this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
		
			BamParallelDecodingDecompressedBlock * decompressedblock;
			BamParallelDecodingAlignmentBuffer * parseBlock;
			BamParallelDecodingParseInfo * parseInfo;

			BamParallelDecodingParseBlockWorkPackage()
			: 
				libmaus::parallel::SimpleThreadWorkPackage(), 
				decompressedblock(0),
				parseBlock(0),
				parseInfo(0)
			{
			}
			
			BamParallelDecodingParseBlockWorkPackage(
				uint64_t const rpriority, 
				BamParallelDecodingDecompressedBlock * rdecompressedblock,
				BamParallelDecodingAlignmentBuffer * rparseBlock,
				BamParallelDecodingParseInfo * rparseInfo,
				uint64_t const rparseDispatcherId
			)
			: libmaus::parallel::SimpleThreadWorkPackage(rpriority,rparseDispatcherId), 
			  decompressedblock(rdecompressedblock), parseBlock(rparseBlock), parseInfo(rparseInfo)
			{
			}
		
			char const * getPackageName() const
			{
				return "BamParallelDecodingParseBlockWorkPackage";
			}
		};

		struct BamParallelDecodingDecompressedPendingObject : std::pair<uint64_t, libmaus::bambam::BamParallelDecodingDecompressedBlock *>
		{
			typedef std::pair<uint64_t, libmaus::bambam::BamParallelDecodingDecompressedBlock *> base_type;
		
			BamParallelDecodingDecompressedPendingObject() : base_type(0,0) {}
			BamParallelDecodingDecompressedPendingObject(
				uint64_t const rid,
				libmaus::bambam::BamParallelDecodingDecompressedBlock * robj
			) : base_type(rid,robj) {}
		};
		
		struct BamParallelDecodingDecompressedPendingObjectHeapComparator
		{
			bool operator()(
				BamParallelDecodingDecompressedPendingObject const & A,
				BamParallelDecodingDecompressedPendingObject const & B
			)
			{
				return A.first > B.first;
			}
		};
		
		struct BamParallelDecodingAlignmentBufferAllocator
		{
			uint64_t bufferSize;
			uint64_t mult;
			
			BamParallelDecodingAlignmentBufferAllocator()
			: bufferSize(0), mult(0)
			{
			
			}
			
			BamParallelDecodingAlignmentBufferAllocator(uint64_t const rbufferSize, uint64_t const rmult)
			: bufferSize(rbufferSize), mult(rmult)
			{
			
			}
			
			BamParallelDecodingAlignmentBuffer * operator()() const
			{
				return new BamParallelDecodingAlignmentBuffer(bufferSize,mult);
			}
		};

		struct BamParallelDecodingAlignmentRewriteBufferAllocator
		{
			uint64_t bufferSize;
			uint64_t mult;
			
			BamParallelDecodingAlignmentRewriteBufferAllocator()
			: bufferSize(0), mult(0)
			{
			
			}
			
			BamParallelDecodingAlignmentRewriteBufferAllocator(uint64_t const rbufferSize, uint64_t const rmult)
			: bufferSize(rbufferSize), mult(rmult)
			{
			
			}
			
			BamParallelDecodingAlignmentRewriteBuffer * operator()() const
			{
				return new BamParallelDecodingAlignmentRewriteBuffer(bufferSize,mult);
			}
		};

		// add decompressed block to free list
		struct BamParallelDecodingDecompressedBlockReturnInterface
		{
			virtual ~BamParallelDecodingDecompressedBlockReturnInterface() {}
			virtual void putBamParallelDecodingDecompressedBlockReturn(BamParallelDecodingDecompressedBlock * block) = 0;
		};
		
		// add parsed block to pending list
		struct BamParallelDecodingParsedBlockAddPendingInterface
		{
			virtual ~BamParallelDecodingParsedBlockAddPendingInterface() {}
			virtual void putBamParallelDecodingParsedBlockAddPending(BamParallelDecodingAlignmentBuffer * algn) = 0;
		};

		// put unfinished parse block in stall slot
		struct BamParallelDecodingParsedBlockStallInterface
		{
			virtual ~BamParallelDecodingParsedBlockStallInterface() {}
			virtual void putBamParallelDecodingParsedBlockStall(BamParallelDecodingAlignmentBuffer * algn) = 0;
		};
		
		struct BamParallelDecodingParsePackageReturnInterface
		{
			virtual ~BamParallelDecodingParsePackageReturnInterface() {}
			virtual void putBamParallelDecodingReturnParsePackage(BamParallelDecodingParseBlockWorkPackage * package) = 0;
		};

		// dispatcher for block parsing
		struct BamParallelDecodingParseBlockWorkPackageDispatcher : public libmaus::parallel::SimpleThreadWorkPackageDispatcher
		{
			BamParallelDecodingDecompressedBlockAddPendingInterface & addDecompressedPendingInterface;
			BamParallelDecodingDecompressedBlockReturnInterface     & returnDecompressedInterface;
			BamParallelDecodingParsedBlockAddPendingInterface       & addParsedPendingInterface;
			BamParallelDecodingParsedBlockStallInterface            & parseStallInterface;
			BamParallelDecodingParsePackageReturnInterface          & packageReturnInterface;

			BamParallelDecodingParseBlockWorkPackageDispatcher(
				BamParallelDecodingDecompressedBlockAddPendingInterface & raddDecompressedPendingInterface,
				BamParallelDecodingDecompressedBlockReturnInterface     & rreturnDecompressedInterface,
				BamParallelDecodingParsedBlockAddPendingInterface       & raddParsedPendingInterface,
				BamParallelDecodingParsedBlockStallInterface            & rparseStallInterface,
				BamParallelDecodingParsePackageReturnInterface          & rpackageReturnInterface
			) : addDecompressedPendingInterface(raddDecompressedPendingInterface),
			    returnDecompressedInterface(rreturnDecompressedInterface),
			    addParsedPendingInterface(raddParsedPendingInterface),
			    parseStallInterface(rparseStallInterface),
			    packageReturnInterface(rpackageReturnInterface)
			{
			
			}
		
			virtual void dispatch(
				libmaus::parallel::SimpleThreadWorkPackage * P, 
				libmaus::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & tpi
			)
			{
				BamParallelDecodingParseBlockWorkPackage * BP = dynamic_cast<BamParallelDecodingParseBlockWorkPackage *>(P);
				assert ( BP );
				
				// tpi.addLogStringWithThreadId("BamParallelDecodingParseBlockWorkPackageDispatcher::dispatch() block id " + libmaus::util::NumberSerialisation::formatNumber(BP->decompressedblock->blockid,0));
				
				// can we parse all information in the decompressed input block?
				if ( BP->parseInfo->parseBlock(*(BP->decompressedblock),*(BP->parseBlock)) )
				{
					// tpi.addLogStringWithThreadId("BamParallelDecodingParseBlockWorkPackageDispatcher::dispatch() parseBlock true");
					
					// if this is the last input block
					if ( BP->decompressedblock->final )
					{
						// post process block (reorder pointers to original input order)
						BP->parseBlock->reorder();
						BP->parseBlock->final = true;
						BP->parseBlock->low   = BP->parseInfo->parseacc;
						BP->parseInfo->parseacc += BP->parseBlock->fill();
						addParsedPendingInterface.putBamParallelDecodingParsedBlockAddPending(BP->parseBlock);						
					}
					// otherwise parse block might not be full yet, stall it
					else
					{
						// std::cerr << "stalling on " << BP->decompressedblock->blockid << std::endl;
						parseStallInterface.putBamParallelDecodingParsedBlockStall(BP->parseBlock);
					}
					
					// return decompressed input block (implies we are ready for the next one)
					returnDecompressedInterface.putBamParallelDecodingDecompressedBlockReturn(BP->decompressedblock);
				}
				else
				{
					// tpi.addLogStringWithThreadId("BamParallelDecodingParseBlockWorkPackageDispatcher::dispatch() parseBlock false");
					
					// post process block (reorder pointers to original input order)
					BP->parseBlock->reorder();
					// put back last name
					BP->parseInfo->putBackLastName(*(BP->parseBlock));
					// not the last one
					BP->parseBlock->final = false;
					// set low
					BP->parseBlock->low   = BP->parseInfo->parseacc;
					// increase number of reads seen
					BP->parseInfo->parseacc += BP->parseBlock->fill();
					
					// parse block is full, add it to pending list
					addParsedPendingInterface.putBamParallelDecodingParsedBlockAddPending(BP->parseBlock);
					// decompressed input block still has more data, mark it as pending again
					addDecompressedPendingInterface.putBamParallelDecodingDecompressedBlockAddPending(BP->decompressedblock);
				}

				// return the work package				
				packageReturnInterface.putBamParallelDecodingReturnParsePackage(BP);				
			}		
		};

		struct BamParallelDecodingValidateBlockWorkPackage : public libmaus::parallel::SimpleThreadWorkPackage
		{
			typedef BamParallelDecodingValidateBlockWorkPackage this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
		
			BamParallelDecodingAlignmentBuffer * parseBlock;

			BamParallelDecodingValidateBlockWorkPackage() : libmaus::parallel::SimpleThreadWorkPackage(), parseBlock(0) {}
			
			BamParallelDecodingValidateBlockWorkPackage(
				uint64_t const rpriority, 
				BamParallelDecodingAlignmentBuffer * rparseBlock,
				uint64_t const rparseDispatcherId
			)
			: libmaus::parallel::SimpleThreadWorkPackage(rpriority,rparseDispatcherId), parseBlock(rparseBlock)
			{
			}
		
			char const * getPackageName() const
			{
				return "BamParallelDecodingValidateBlockWorkPackage";
			}
		};

		struct BamParallelDecodingValidatePackageReturnInterface
		{
			virtual ~BamParallelDecodingValidatePackageReturnInterface() {}
			virtual void putBamParallelDecodingReturnValidatePackage(BamParallelDecodingValidateBlockWorkPackage * package) = 0;
		};

		// add validate block to pending list
		struct BamParallelDecodingValidateBlockAddPendingInterface
		{
			virtual ~BamParallelDecodingValidateBlockAddPendingInterface() {}
			virtual void putBamParallelDecodingValidatedBlockAddPending(BamParallelDecodingAlignmentBuffer * algn, bool const ok) = 0;
		};

		// dispatcher for block validation
		struct BamParallelDecodingValidateBlockWorkPackageDispatcher : public libmaus::parallel::SimpleThreadWorkPackageDispatcher
		{
			BamParallelDecodingValidatePackageReturnInterface   & packageReturnInterface;
			BamParallelDecodingValidateBlockAddPendingInterface & addValidatedPendingInterface;

			BamParallelDecodingValidateBlockWorkPackageDispatcher(
				BamParallelDecodingValidatePackageReturnInterface   & rpackageReturnInterface,
				BamParallelDecodingValidateBlockAddPendingInterface & raddValidatedPendingInterface

			) : packageReturnInterface(rpackageReturnInterface), addValidatedPendingInterface(raddValidatedPendingInterface)
			{
			}
		
			virtual void dispatch(
				libmaus::parallel::SimpleThreadWorkPackage * P, 
				libmaus::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & tpi
			)
			{
				BamParallelDecodingValidateBlockWorkPackage * BP = dynamic_cast<BamParallelDecodingValidateBlockWorkPackage *>(P);
				assert ( BP );

				bool const ok = BP->parseBlock->checkValidPacked();
								
				addValidatedPendingInterface.putBamParallelDecodingValidatedBlockAddPending(BP->parseBlock,ok);
								
				// return the work package				
				packageReturnInterface.putBamParallelDecodingReturnValidatePackage(BP);				
			}		
		};

		struct BamParallelDecodingRewriteBlockWorkPackage : public libmaus::parallel::SimpleThreadWorkPackage
		{
			typedef BamParallelDecodingRewriteBlockWorkPackage this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
		
			BamParallelDecodingAlignmentBuffer * parseBlock;
			BamParallelDecodingAlignmentRewriteBuffer * rewriteBlock;

			BamParallelDecodingRewriteBlockWorkPackage() : libmaus::parallel::SimpleThreadWorkPackage(), parseBlock(0) {}
			
			BamParallelDecodingRewriteBlockWorkPackage(
				uint64_t const rpriority, 
				BamParallelDecodingAlignmentBuffer * rparseBlock,
				BamParallelDecodingAlignmentRewriteBuffer * rrewriteBlock,
				uint64_t const rparseDispatcherId
			)
			: libmaus::parallel::SimpleThreadWorkPackage(rpriority,rparseDispatcherId), parseBlock(rparseBlock), rewriteBlock(rrewriteBlock)
			{
			}
		
			char const * getPackageName() const
			{
				return "BamParallelDecodingRewriteBlockWorkPackage";
			}
		};
		
		// return a rewrite work package
		struct BamParallelDecodingRewritePackageReturnInterface
		{	
			virtual ~BamParallelDecodingRewritePackageReturnInterface() {}
			virtual void putBamParallelDecodingReturnRewritePackage(BamParallelDecodingRewriteBlockWorkPackage * package) = 0;
		};
		
		// return a completely processed alignment buffer
		struct BamParallelDecodingAlignmentBufferReturnInterface
		{
			virtual ~BamParallelDecodingAlignmentBufferReturnInterface() {}
			virtual void putBamParallelDecodingReturnAlignmentBuffer(BamParallelDecodingAlignmentBuffer * buffer) = 0;
		};
		
		// reinsert an alignment buffer which still has more data
		struct BamParallelDecodingAlignmentBufferReinsertInterface
		{
			virtual ~BamParallelDecodingAlignmentBufferReinsertInterface() {}
			virtual void putBamParallelDecodingReinsertAlignmentBuffer(BamParallelDecodingAlignmentBuffer * buffer) = 0;
		};
		
		// add a completed rewrite buffer to the pending list
		struct BamParallelDecodingAlignmentRewriteBufferAddPendingInterface
		{
			virtual ~BamParallelDecodingAlignmentRewriteBufferAddPendingInterface() {}
			virtual void putBamParallelDecodingAlignmentRewriteBufferAddPending(BamParallelDecodingAlignmentRewriteBuffer * buffer) = 0;
		};
		
		// reinsert an alignment rewrite buffer which has more space
		struct BamParallelDecodingAlignmentRewriteBufferReinsertForFillingInterface
		{
			virtual ~BamParallelDecodingAlignmentRewriteBufferReinsertForFillingInterface() {}
			virtual void putBamParallelDecodingAlignmentRewriteBufferReinsertForFilling(BamParallelDecodingAlignmentRewriteBuffer * buffer) = 0;
		};

		// dispatcher for block rewriting
		struct BamParallelDecodingRewriteBlockWorkPackageDispatcher : public libmaus::parallel::SimpleThreadWorkPackageDispatcher
		{
			BamParallelDecodingRewritePackageReturnInterface & packageReturnInterface;
			BamParallelDecodingAlignmentBufferReturnInterface & alignmentBufferReturnInterface;
			BamParallelDecodingAlignmentBufferReinsertInterface & alignmentBufferReinsertInterface;
			BamParallelDecodingAlignmentRewriteBufferAddPendingInterface & alignmentRewriteBufferAddPendingInterface;
			BamParallelDecodingAlignmentRewriteBufferReinsertForFillingInterface & alignmentRewriteBufferReinsertForFillingInterface;
			
			BamParallelDecodingRewriteBlockWorkPackageDispatcher(
				BamParallelDecodingRewritePackageReturnInterface & rpackageReturnInterface,
				BamParallelDecodingAlignmentBufferReturnInterface & ralignmentBufferReturnInterface,
				BamParallelDecodingAlignmentBufferReinsertInterface & ralignmentBufferReinsertInterface,
				BamParallelDecodingAlignmentRewriteBufferAddPendingInterface & ralignmentRewriteBufferAddPendingInterface,
				BamParallelDecodingAlignmentRewriteBufferReinsertForFillingInterface & ralignmentRewriteBufferReinsertForFillingInterface
			) : packageReturnInterface(rpackageReturnInterface), 
			    alignmentBufferReturnInterface(ralignmentBufferReturnInterface),
			    alignmentBufferReinsertInterface(ralignmentBufferReinsertInterface), 
			    alignmentRewriteBufferAddPendingInterface(ralignmentRewriteBufferAddPendingInterface),
			    alignmentRewriteBufferReinsertForFillingInterface(ralignmentRewriteBufferReinsertForFillingInterface)
			{
			}
		
			virtual void dispatch(
				libmaus::parallel::SimpleThreadWorkPackage * P, 
				libmaus::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & tpi
			)
			{
				BamParallelDecodingRewriteBlockWorkPackage * BP = dynamic_cast<BamParallelDecodingRewriteBlockWorkPackage *>(P);
				assert ( BP );
				
				libmaus::bambam::BamAlignment * stallAlgn = 0;
				bool rewriteBlockFull = false;

				while ( (!rewriteBlockFull) && (stallAlgn=BP->parseBlock->popStallBuffer()) )
				{
					if ( BP->rewriteBlock->put(reinterpret_cast<char const *>(stallAlgn->D.begin()),stallAlgn->blocksize) )
					{
						BP->parseBlock->returnAlignment(stallAlgn);
					}
					else
					{
						BP->parseBlock->pushFrontStallBuffer(stallAlgn);
						rewriteBlockFull = true;
					}
				}
				
				// if there is still space in the rewrite block
				if ( ! rewriteBlockFull )
				{
					std::deque<libmaus::bambam::BamAlignment *> algns;
					
					while ( 
						(!rewriteBlockFull)
						&&
						BP->parseBlock->extractNextSameName(algns) 
					)
					{
						assert ( algns.size() );
						
						int64_t i1 = -1;
						int64_t i2 = -1;
						
						for ( uint64_t i = 0; i < algns.size(); ++i )
						{
							uint16_t const flags = algns[i]->getFlags();
							
							if ( 
								(! (flags&libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FPAIRED))
								||
								(flags & (libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FSECONDARY|libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FSUPPLEMENTARY) )
							)
							{
								continue;
							}
							else if ( 
								(flags&libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FREAD1)
								&&
								(!(flags&libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FREAD2))
							)
							{
								i1 = i;
							}
							else if ( 
								(flags&libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FREAD2)
								&&
								(!(flags&libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FREAD1))
							)
							{
								i2 = i;
							}
						}
						
						if ( (i1 != -1) && (i2 != -1) )
						{
							libmaus::bambam::BamAlignment & a1 = *(algns[i1]);
							libmaus::bambam::BamAlignment & a2 = *(algns[i2]);
						
							a1.filterOutAux(BP->parseBlock->MQMSMCMTfilter);
							a2.filterOutAux(BP->parseBlock->MQMSMCMTfilter);
							
							if ( algns[i1]->blocksize + 6 * (3+sizeof(int32_t)) > algns[i1]->D.size() )
								a1.D.resize(algns[i1]->blocksize + 8 * (3+sizeof(int32_t)));
							if ( algns[i2]->blocksize + 6 * (3+sizeof(int32_t)) > algns[i2]->D.size() )
								a2.D.resize(algns[i2]->blocksize + 8 * (3+sizeof(int32_t)));
							
							libmaus::bambam::BamAlignment::fixMateInformationPreFiltered(a1,a2);
							libmaus::bambam::BamAlignment::addMateBaseScorePreFiltered(a1,a2);
							libmaus::bambam::BamAlignment::addMateCoordinatePreFiltered(a1,a2);
							libmaus::bambam::BamAlignment::addMateTagPreFiltered(a1,a2,"MT");
						}
					
						while ( algns.size() )
						{
							libmaus::bambam::BamAlignment * algn = algns.front();
							int64_t const coordinate = algn->getCoordinate();
							
							if ( BP->rewriteBlock->put(reinterpret_cast<char const *>(algn->D.begin()),algn->blocksize,coordinate) )
							{
								BP->parseBlock->returnAlignment(algn);
								algns.pop_front();
							}
							else
							{
								// block is full, stall rest of the alignments
								rewriteBlockFull = true;
								
								for ( uint64_t i = 0; i < algns.size(); ++i )
									BP->parseBlock->pushBackStallBuffer(algns[i]);
									
								algns.resize(0);
							}
						}					
					}
				}
				
				// force block pass if this is the final parse block
				// rewriteBlockFull = rewriteBlockFull || BP->parseBlock->final;
				
				// if rewrite block is now full
				if ( rewriteBlockFull )
				{
					// finalise block by compactifying pointers
					BP->rewriteBlock->reorder();
					// add rewrite block to pending list
					alignmentRewriteBufferAddPendingInterface.putBamParallelDecodingAlignmentRewriteBufferAddPending(BP->rewriteBlock);
					// reinsert parse block
					alignmentBufferReinsertInterface.putBamParallelDecodingReinsertAlignmentBuffer(BP->parseBlock);
				}
				// input buffer is empty
				else
				{
					// reinsert rewrite buffer for more filling
					alignmentRewriteBufferReinsertForFillingInterface.putBamParallelDecodingAlignmentRewriteBufferReinsertForFilling(BP->rewriteBlock);
					// return empty input buffer
					BP->parseBlock->reset();
					alignmentBufferReturnInterface.putBamParallelDecodingReturnAlignmentBuffer(BP->parseBlock);
				}

				#if 0
				// add algnbuffer to the free list
				parseBlockFreeList.put(algn);

				// see if we can parse the next block now
				checkParsePendingList();
				#endif
				
				// return the work package				
				packageReturnInterface.putBamParallelDecodingReturnRewritePackage(BP);				
			}		
		};

		struct BamAlignmentRewritePosSortContextBaseBlockSortedInterface
		{
			virtual ~BamAlignmentRewritePosSortContextBaseBlockSortedInterface() {}
			virtual void baseBlockSorted() = 0;
		};
		
		struct BamAlignmentRewritePosSortContextMergePackageFinished
		{
			virtual ~BamAlignmentRewritePosSortContextMergePackageFinished() {}
			virtual void mergePackageFinished() = 0;
		};
		
		template<typename _order_type>
		struct BamAlignmentRewritePosSortBaseSortPackage : public libmaus::parallel::SimpleThreadWorkPackage
		{
			typedef _order_type order_type;
			typedef BamParallelDecodingAlignmentRewriteBuffer::pointer_type * iterator;
		
			typedef BamAlignmentRewritePosSortBaseSortPackage<order_type> this_type;
			typedef typename libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
		
			typedef libmaus::sorting::ParallelStableSort::BaseSortRequest<iterator,order_type> request_type;
			request_type * request;
			BamAlignmentRewritePosSortContextBaseBlockSortedInterface * blockSortedInterface;

			BamAlignmentRewritePosSortBaseSortPackage() : libmaus::parallel::SimpleThreadWorkPackage(), request(0) {}
			
			BamAlignmentRewritePosSortBaseSortPackage(
				uint64_t const rpriority, 
				request_type * rrequest,
				BamAlignmentRewritePosSortContextBaseBlockSortedInterface * rblockSortedInterface,
				uint64_t const rdispatcherId
			)
			: libmaus::parallel::SimpleThreadWorkPackage(rpriority,rdispatcherId), request(rrequest), blockSortedInterface(rblockSortedInterface)
			{
			}
		
			char const * getPackageName() const
			{
				return "BamAlignmentRewritePosSortBaseSortPackage";
			}
		};

		template<typename _order_type>
		struct BamParallelDecodingBaseSortWorkPackageReturnInterface
		{
			typedef _order_type order_type;
			
			virtual ~BamParallelDecodingBaseSortWorkPackageReturnInterface() {}
			virtual void putBamParallelDecodingBaseSortWorkPackage(BamAlignmentRewritePosSortBaseSortPackage<order_type> * package) = 0;
		};

		// dispatcher for block base sorting
		template<typename _order_type>
		struct BamParallelDecodingBaseSortWorkPackageDispatcher : public libmaus::parallel::SimpleThreadWorkPackageDispatcher
		{
			typedef _order_type order_type;
			
			BamParallelDecodingBaseSortWorkPackageReturnInterface<order_type> & packageReturnInterface;
			
			BamParallelDecodingBaseSortWorkPackageDispatcher(
				BamParallelDecodingBaseSortWorkPackageReturnInterface<order_type> & rpackageReturnInterface
			) : packageReturnInterface(rpackageReturnInterface) 
			{
			}
		
			virtual void dispatch(
				libmaus::parallel::SimpleThreadWorkPackage * P, 
				libmaus::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & tpi
			)
			{
				BamAlignmentRewritePosSortBaseSortPackage<order_type> * BP = dynamic_cast<BamAlignmentRewritePosSortBaseSortPackage<order_type> *>(P);
				assert ( BP );
				
				BP->request->dispatch();
				BP->blockSortedInterface->baseBlockSorted();
				
				// return the work package				
				packageReturnInterface.putBamParallelDecodingBaseSortWorkPackage(BP);				
			}		
		};

		template<typename _order_type>
		struct BamAlignmentRewritePosMergeSortPackage : public libmaus::parallel::SimpleThreadWorkPackage
		{
			typedef BamParallelDecodingAlignmentRewriteBuffer::pointer_type * iterator;
			typedef _order_type order_type;
			
			typedef BamAlignmentRewritePosMergeSortPackage<order_type> this_type;
			typedef typename libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
						
			typedef libmaus::sorting::ParallelStableSort::MergeRequest<iterator,order_type> request_type;
		
			request_type * request;
			BamAlignmentRewritePosSortContextMergePackageFinished * mergedInterface;

			BamAlignmentRewritePosMergeSortPackage() : libmaus::parallel::SimpleThreadWorkPackage(), request(0) {}
			
			BamAlignmentRewritePosMergeSortPackage(
				uint64_t const rpriority, 
				request_type * rrequest,
				BamAlignmentRewritePosSortContextMergePackageFinished * rmergedInterface,
				uint64_t const rdispatcherId
			)
			: libmaus::parallel::SimpleThreadWorkPackage(rpriority,rdispatcherId), request(rrequest), mergedInterface(rmergedInterface)
			{
			}
		
			char const * getPackageName() const
			{
				return "BamAlignmentRewritePosMergeSortPackage";
			}
		};
		
		template<typename order_type>
		struct BamParallelDecodingBaseMergeSortWorkPackageReturnInterface
		{
			virtual ~BamParallelDecodingBaseMergeSortWorkPackageReturnInterface() {}
			virtual void putBamParallelDecodingMergeSortWorkPackage(BamAlignmentRewritePosMergeSortPackage<order_type> * package) = 0;
		};

		// dispatcher for block merging
		template<typename _order_type>
		struct BamParallelDecodingMergeSortWorkPackageDispatcher : public libmaus::parallel::SimpleThreadWorkPackageDispatcher
		{
			typedef _order_type order_type;
		
			BamParallelDecodingBaseMergeSortWorkPackageReturnInterface<order_type> & packageReturnInterface;
			
			BamParallelDecodingMergeSortWorkPackageDispatcher(
				BamParallelDecodingBaseMergeSortWorkPackageReturnInterface<order_type> & rpackageReturnInterface
			) : packageReturnInterface(rpackageReturnInterface) 
			{
			}
		
			virtual void dispatch(
				libmaus::parallel::SimpleThreadWorkPackage * P, 
				libmaus::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & tpi
			)
			{
				BamAlignmentRewritePosMergeSortPackage<order_type> * BP = dynamic_cast<BamAlignmentRewritePosMergeSortPackage<order_type> *>(P);
				assert ( BP );
				
				BP->request->dispatch();
				BP->mergedInterface->mergePackageFinished();
				
				// return the work package				
				packageReturnInterface.putBamParallelDecodingMergeSortWorkPackage(BP);				
			}		
		};

		struct BamParallelDecodingSortFinishedInterface
		{
			virtual ~BamParallelDecodingSortFinishedInterface() {}
			virtual void putBamParallelDecodingSortFinished(BamParallelDecodingAlignmentRewriteBuffer * buffer) = 0;
		};

		template<typename _order_type>
		struct BamAlignmentRewritePosSortContext : 
			public BamAlignmentRewritePosSortContextBaseBlockSortedInterface,
			public BamAlignmentRewritePosSortContextMergePackageFinished
		{
			typedef _order_type order_type;
			typedef BamAlignmentRewritePosSortContext<order_type> this_type;
			typedef typename libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			typedef BamParallelDecodingAlignmentRewriteBuffer::pointer_type * iterator;
		
			BamParallelDecodingAlignmentRewriteBuffer * const buffer;
			order_type comparator;
			libmaus::sorting::ParallelStableSort::ParallelSortControl<iterator,order_type> PSC;
			libmaus::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & STPI;

			typename libmaus::sorting::ParallelStableSort::MergeLevels<iterator,order_type>::level_type * volatile level;
			libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<BamAlignmentRewritePosMergeSortPackage<order_type> > & mergeSortPackages;
			uint64_t const mergeSortDispatcherId;
			
			BamParallelDecodingSortFinishedInterface & sortFinishedInterface;
			
			enum state_type
			{
				state_base_sort, state_merge_plan, state_merge_execute, state_copy_back, state_done
			};
			
			state_type volatile state;
			
			BamAlignmentRewritePosSortContext(
				BamParallelDecodingAlignmentRewriteBuffer * rbuffer,
				uint64_t const numthreads,
				libmaus::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & rSTPI,
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<BamAlignmentRewritePosMergeSortPackage<order_type> > & rmergeSortPackages,
				uint64_t const rmergeSortDispatcherId,
				BamParallelDecodingSortFinishedInterface & rsortFinishedInterface
			)
			: buffer(rbuffer), comparator(buffer), PSC(				
				buffer->getPointerArray().first,
				buffer->getPointerArray().first + buffer->fill(),
				buffer->getAuxPointerArray().first,
				buffer->getAuxPointerArray().first + buffer->fill(),
				comparator,
				numthreads,
				true /* copy back */), 
				STPI(rSTPI),
				level(PSC.mergeLevels.levels.size() ? &(PSC.mergeLevels.levels[0]) : 0),
				mergeSortPackages(rmergeSortPackages),
				mergeSortDispatcherId(rmergeSortDispatcherId),
				sortFinishedInterface(rsortFinishedInterface),
				state((buffer->fill() > 1) ? state_base_sort : state_done)
			{
			
			}
			
			void enqueBaseSortPackages(
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<BamAlignmentRewritePosSortBaseSortPackage<order_type> > & baseSortPackages,
				uint64_t const baseSortDispatcher
			)
			{
				for ( uint64_t i = 0; i < PSC.baseSortRequests.baseSortRequests.size(); ++i )
				{
					BamAlignmentRewritePosSortBaseSortPackage<order_type> * package = baseSortPackages.getPackage();
					*package = BamAlignmentRewritePosSortBaseSortPackage<order_type>(
						0 /* prio */,
						&(PSC.baseSortRequests.baseSortRequests[i]),
						this,
						baseSortDispatcher
					);
					
					STPI.enque(package);
				}
			}
							
			/* make sure result is in place of original data */
			void copyBack()
			{
				state = state_copy_back;
				
				if ( PSC.needCopyBack )
				{
					std::copy(PSC.context.in,PSC.context.in+PSC.context.n,PSC.context.out);
				}
				
				state = state_done;

				sortFinishedInterface.putBamParallelDecodingSortFinished(buffer);
			}
			
			void planMerge()
			{
				state = state_merge_plan;
				assert ( level );
				level->dispatch();
				executeMerge();
			}
			
			void executeMerge()
			{
				state = state_merge_execute;
				
				assert ( level );
				for ( uint64_t i = 0; i < level->mergeRequests.size(); ++i )
				{
					BamAlignmentRewritePosMergeSortPackage<order_type> * package = mergeSortPackages.getPackage();
					*package = BamAlignmentRewritePosMergeSortPackage<order_type>(
						0 /* prio */,
						&(level->mergeRequests[i]),
						this,
						mergeSortDispatcherId
					);
					STPI.enque(package);
				}
			}

			virtual void baseBlockSorted()
			{
				if ( (++PSC.baseSortRequests.requestsFinished) == PSC.baseSortRequests.baseSortRequests.size() )
				{
					if ( PSC.mergeLevels.levels.size() )
					{
						planMerge();
					}
					else
					{
						copyBack();
					}
				}
			}

			virtual void mergePackageFinished()
			{
				if ( (++(level->requestsFinished)) == level->mergeRequests.size() )
				{
					level = level->next;
					
					if ( level )
						planMerge();
					else
						copyBack();
				}
			}
		};

		// BamAlignmentRewriteBufferPosComparator
		template<typename _order_type>
		struct BamParallelDecodingControl :
			public BamParallelDecodingInputBlockWorkPackageReturnInterface,
			public BamParallelDecodingInputBlockAddPendingInterface,
			public BamParallelDecodingDecompressBlockWorkPackageReturnInterface,
			public BamParallelDecodingInputBlockReturnInterface,
			public BamParallelDecodingDecompressedBlockAddPendingInterface,
			public BamParallelDecodingBgzfInflateZStreamBaseReturnInterface,
			public BamParallelDecodingDecompressedBlockReturnInterface,
			public BamParallelDecodingParsedBlockAddPendingInterface,
			public BamParallelDecodingParsedBlockStallInterface,
			public BamParallelDecodingParsePackageReturnInterface,
			public BamParallelDecodingValidatePackageReturnInterface,
			public BamParallelDecodingValidateBlockAddPendingInterface,
			public BamParallelDecodingRewritePackageReturnInterface,
			public BamParallelDecodingAlignmentBufferReturnInterface,
			public BamParallelDecodingAlignmentBufferReinsertInterface,
			public BamParallelDecodingAlignmentRewriteBufferAddPendingInterface,
			public BamParallelDecodingAlignmentRewriteBufferReinsertForFillingInterface,
			public BamParallelDecodingBaseSortWorkPackageReturnInterface<_order_type>,
			public BamParallelDecodingBaseMergeSortWorkPackageReturnInterface<_order_type>,
			public BamParallelDecodingSortFinishedInterface
		{
			typedef _order_type order_type;
		
			typedef BamParallelDecodingControl<order_type> this_type;
			typedef typename libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

			typedef libmaus::bambam::BamParallelDecodingDecompressedBlock decompressed_block_type;
			
			libmaus::parallel::SimpleThreadPool & STP;
			
			BamParallelDecodingInputBlockWorkPackageDispatcher readDispatcher;
			uint64_t const readDispatcherId;
			
			BamParallelDecodingDecompressBlockWorkPackageDispatcher decompressDispatcher;
			uint64_t const decompressDispatcherId;

			BamParallelDecodingParseBlockWorkPackageDispatcher parseDispatcher;
			uint64_t const parseDispatcherId;

			BamParallelDecodingValidateBlockWorkPackageDispatcher validateDispatcher;
			uint64_t const validateDispatcherId;
			
			BamParallelDecodingRewriteBlockWorkPackageDispatcher rewriteDispatcher;
			uint64_t const rewriteDispatcherId;
			
			BamParallelDecodingBaseSortWorkPackageDispatcher<order_type> baseSortDispatcher;
			uint64_t const baseSortDispatcherId;
			
			BamParallelDecodingMergeSortWorkPackageDispatcher<order_type> mergeSortDispatcher;
			uint64_t const mergeSortDispatcherId;
			
			uint64_t const numthreads;

			BamParallelDecodingControlInputInfo inputinfo;

			libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<BamParallelDecodingInputBlockWorkPackage> readWorkPackages;
			libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<BamParallelDecodingDecompressBlockWorkPackage> decompressWorkPackages;
			libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<BamParallelDecodingParseBlockWorkPackage> parseWorkPackages;
			libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<BamParallelDecodingValidateBlockWorkPackage> validateWorkPackages;
			libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<BamParallelDecodingRewriteBlockWorkPackage> rewriteWorkPackages;
			libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<BamAlignmentRewritePosSortBaseSortPackage<order_type> > baseSortPackages;
			libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<BamAlignmentRewritePosMergeSortPackage<order_type> > mergeSortPackages;

			// free list for bgzf decompressors
			libmaus::parallel::LockedFreeList<libmaus::lz::BgzfInflateZStreamBase>::unique_ptr_type Pdecoders;
			libmaus::parallel::LockedFreeList<libmaus::lz::BgzfInflateZStreamBase> & decoders;
			
			// free list for decompressed blocks
			libmaus::parallel::LockedFreeList<decompressed_block_type> decompressedBlockFreeList;
			
			// free list for alignment buffers
			libmaus::parallel::LockedFreeList<BamParallelDecodingAlignmentBuffer,BamParallelDecodingAlignmentBufferAllocator> parseBlockFreeList;
			
			// free list for rewrite buffers
			libmaus::parallel::LockedFreeList<BamParallelDecodingAlignmentRewriteBuffer,BamParallelDecodingAlignmentRewriteBufferAllocator> rewriteBlockFreeList;

			// input blocks ready to be decompressed
			libmaus::parallel::LockedQueue<BamParallelDecodingControlInputInfo::input_block_type *> decompressPending;
			
			// decompressed blocks ready to be parsed
			libmaus::parallel::LockedHeap<BamParallelDecodingDecompressedPendingObject,BamParallelDecodingDecompressedPendingObjectHeapComparator> parsePending;
			libmaus::parallel::PosixSpinLock parsePendingLock;

			// id of next block to be parsed
			uint64_t volatile nextDecompressedBlockToBeParsed;
			libmaus::parallel::PosixSpinLock nextDecompressedBlockToBeParsedLock;
			
			BamParallelDecodingAlignmentBuffer * volatile parseStallSlot;
			libmaus::parallel::PosixSpinLock parseStallSlotLock;

			BamParallelDecodingParseInfo parseInfo;
			
			libmaus::parallel::LockedBool lastParseBlockSeen;
			libmaus::parallel::SynchronousCounter<uint64_t> readsParsed;
			libmaus::parallel::PosixSpinLock readsParsedLock;
			uint64_t volatile readsParsedLastPrint;

			libmaus::parallel::SynchronousCounter<uint64_t> parseBlocksSeen;
			libmaus::parallel::SynchronousCounter<uint64_t> parseBlocksValidated;

			libmaus::parallel::LockedBool lastParseBlockValidated;
			
			libmaus::parallel::SynchronousCounter<uint64_t> parseBlocksRewritten;
			
			libmaus::parallel::LockedBool lastParseBlockRewritten;

			// list of alignment blocks ready for rewriting			
			libmaus::parallel::LockedQueue<BamParallelDecodingAlignmentBuffer *> rewritePending;

			libmaus::parallel::PosixSpinLock readsRewrittenLock;
			libmaus::parallel::SynchronousCounter<uint64_t> readsRewritten;
			
			// list of rewritten blocks to be sorted
			libmaus::parallel::LockedQueue<BamParallelDecodingAlignmentRewriteBuffer *> sortPending;
			
			// blocks currently being sorted
			std::map < BamParallelDecodingAlignmentRewriteBuffer *, typename BamAlignmentRewritePosSortContext<order_type>::shared_ptr_type > sortActive;
			libmaus::parallel::PosixSpinLock sortActiveLock;

			libmaus::parallel::SynchronousCounter<uint64_t> numSortBlocksIn;
			libmaus::parallel::SynchronousCounter<uint64_t> numSortBlocksOut;

			libmaus::parallel::LockedBool lastSortBlockFinished;
			libmaus::parallel::SynchronousCounter<uint64_t> readsSorted;

			virtual void putBamParallelDecodingSortFinished(BamParallelDecodingAlignmentRewriteBuffer * buffer)
			{
				typename BamAlignmentRewritePosSortContext<order_type>::shared_ptr_type context;
				
				{
					libmaus::parallel::ScopePosixSpinLock slock(sortActiveLock);
					typename std::map < BamParallelDecodingAlignmentRewriteBuffer *, typename BamAlignmentRewritePosSortContext<order_type>::shared_ptr_type >::iterator ita =
						sortActive.find(buffer);
					assert ( ita != sortActive.end() );
					context = ita->second;
					sortActive.erase(ita);
				}
				
				#if 0
				{
					uint64_t const f = buffer->fill();
					for ( uint64_t i = 1; i < f; ++i )
					{
						uint8_t const * pa = buffer->A.begin() + buffer->pP[i-1] + sizeof(uint32_t) + sizeof(uint64_t);
						uint8_t const * pb = buffer->A.begin() + buffer->pP[  i] + sizeof(uint32_t) + sizeof(uint64_t);

						int32_t const refa = ::libmaus::bambam::BamAlignmentDecoderBase::getRefID(pa);
						int32_t const refb = ::libmaus::bambam::BamAlignmentDecoderBase::getRefID(pb);
			
						if ( refa != refb )
						{
							assert ( static_cast<uint32_t>(refa) < static_cast<uint32_t>(refb) );
						}
						else
						{
							int32_t const posa = ::libmaus::bambam::BamAlignmentDecoderBase::getPos(pa);
							int32_t const posb = ::libmaus::bambam::BamAlignmentDecoderBase::getPos(pb);
							
							assert ( posa <= posb );
						}
					}
				}
				#endif
				
				readsSorted += buffer->fill();

				// return buffer
				buffer->reset();
				rewriteBlockFreeList.put(buffer);
				// check if we can process another buffer now
				checkRewritePending();
				
				numSortBlocksOut++;
				
				if ( 
					lastParseBlockRewritten.get()
					&&
					static_cast<uint64_t>(numSortBlocksIn) == static_cast<uint64_t>(numSortBlocksOut)
				)
				{
					lastSortBlockFinished.set(true);
				}
			}

			virtual void putBamParallelDecodingMergeSortWorkPackage(BamAlignmentRewritePosMergeSortPackage<order_type> * package)
			{
				mergeSortPackages.returnPackage(package);
			}

			virtual void putBamParallelDecodingBaseSortWorkPackage(BamAlignmentRewritePosSortBaseSortPackage<order_type> * package)
			{
				baseSortPackages.returnPackage(package);
			}

			virtual void putBamParallelDecodingReinsertAlignmentBuffer(BamParallelDecodingAlignmentBuffer * buffer)
			{
				#if 0
				STP.getGlobalLock().lock();
				std::cerr << "reinserting alignment buffer" << std::endl;
				STP.getGlobalLock().unlock();
				#endif
				
				// reinsert into pending queue
				rewritePending.push_front(buffer);
				// check if we can process this buffer
				checkRewritePending();
			}

			/*
			 * add a pending full rewrite buffer
			 */
			virtual void putBamParallelDecodingAlignmentRewriteBufferAddPending(BamParallelDecodingAlignmentRewriteBuffer * buffer)
			{
				// zzz process buffer here
				#if 0
				STP.getGlobalLock().lock();
				std::cerr << "finished rewrite buffer of size " << buffer->fill() << std::endl;
				STP.getGlobalLock().unlock();
				#endif

				{
					libmaus::parallel::PosixSpinLock lreadsRewrittenLock(readsRewrittenLock);
					readsRewritten += buffer->fill();
				}

				typename BamAlignmentRewritePosSortContext<order_type>::shared_ptr_type sortContext(
					new BamAlignmentRewritePosSortContext<order_type>(
						buffer,
						STP.getNumThreads(),
						STP,
						mergeSortPackages,
						mergeSortDispatcherId,
						*this
					)
				);
				
				{
				libmaus::parallel::ScopePosixSpinLock slock(sortActiveLock);
				sortActive[buffer] = sortContext;
				}
				
				numSortBlocksIn++;
				sortContext->enqueBaseSortPackages(baseSortPackages,baseSortDispatcherId);
			}

			/*
			 * reinsert a rewrite buffer for more filling
			 */
			virtual void putBamParallelDecodingAlignmentRewriteBufferReinsertForFilling(BamParallelDecodingAlignmentRewriteBuffer * buffer)
			{
				#if 0
				STP.getGlobalLock().lock();
				std::cerr << "reinserting rewrite buffer" << std::endl;
				STP.getGlobalLock().unlock();
				#endif
				
				// return buffer
				rewriteBlockFreeList.put(buffer);
				// check whether we can process another one
				checkRewritePending();
			}
			
			/*
			 * return a fully processed parse buffer
			 */
			virtual void putBamParallelDecodingReturnAlignmentBuffer(BamParallelDecodingAlignmentBuffer * buffer)
			{
				#if 0
				STP.getGlobalLock().lock();
				std::cerr << "parse buffer finished" << std::endl;
				STP.getGlobalLock().unlock();
				#endif
				
				parseBlocksRewritten += 1;
				
				if ( lastParseBlockValidated.get() && static_cast<uint64_t>(parseBlocksRewritten) == static_cast<uint64_t>(parseBlocksValidated) )
				{
					lastParseBlockRewritten.set(true);
					
					// check whether any rewrite blocks are in the free list
					std::vector < BamParallelDecodingAlignmentRewriteBuffer * > reblocks;
					while ( ! rewriteBlockFreeList.empty() )
						reblocks.push_back(rewriteBlockFreeList.get());

					// number of blocks queued for sorting
					uint64_t queued = 0;
					
					for ( uint64_t i = 0; i < reblocks.size(); ++i )
						if ( reblocks[i]->fill() )
						{
							reblocks[i]->reorder();
							putBamParallelDecodingAlignmentRewriteBufferAddPending(reblocks[i]);
							queued++;
						}
						else
						{
							rewriteBlockFreeList.put(reblocks[i]);
						}
					
					#if 0
					STP.getGlobalLock().lock();
					std::cerr << "all parse buffers finished" << std::endl;
					STP.getGlobalLock().unlock();
					#endif
					
					// ensure at least one block is passed so lastParseBlockRewritten==true is observed downstream
					if ( ! queued )
					{
						libmaus::parallel::PosixSpinLock lreadsParsed(readsParsedLock);

						assert ( reblocks.size() );
						assert ( ! reblocks[0]->fill() );
						reblocks[0]->reorder();
						putBamParallelDecodingAlignmentRewriteBufferAddPending(reblocks[0]);
					}
				}
						
				buffer->reset();
				parseBlockFreeList.put(buffer);
				checkParsePendingList();
			}

			virtual void putBamParallelDecodingReturnValidatePackage(BamParallelDecodingValidateBlockWorkPackage * package)
			{
				validateWorkPackages.returnPackage(package);
			}

			virtual void putBamParallelDecodingReturnParsePackage(BamParallelDecodingParseBlockWorkPackage * package)
			{
				parseWorkPackages.returnPackage(package);
			}

			virtual void putBamParallelDecodingReturnRewritePackage(BamParallelDecodingRewriteBlockWorkPackage * package)
			{
				rewriteWorkPackages.returnPackage(package);
			}

			virtual void putBamParallelDecodingParsedBlockStall(BamParallelDecodingAlignmentBuffer * algn)
			{
				{
					libmaus::parallel::ScopePosixSpinLock slock(parseStallSlotLock);
					
					#if 0
					{
					std::ostringstream ostr;
					ostr << "setting stall slot to " << algn;
					STP.addLogStringWithThreadId(ostr.str());
					}
					#endif
					
					if ( parseStallSlot )
						STP.printLog(std::cerr );
									
					assert ( parseStallSlot == 0 );
					parseStallSlot = algn;
				}
				
				checkParsePendingList();
			}

			// return a read work package to the free list
			void putBamParallelDecodingInputBlockWorkPackage(BamParallelDecodingInputBlockWorkPackage * package)
			{
				readWorkPackages.returnPackage(package);
			}

			// return decompress block work package
			virtual void putBamParallelDecodingDecompressBlockWorkPackage(BamParallelDecodingDecompressBlockWorkPackage * package)
			{
				decompressWorkPackages.returnPackage(package);
			}

			// check whether we can decompress packages			
			void checkDecompressPendingList()
			{
				bool running = true;
				
				while ( running )
				{
					// XXX ZZZ lock for decompressPending
					BamParallelDecodingControlInputInfo::input_block_type * inputblock = 0;
					libmaus::lz::BgzfInflateZStreamBase * decoder = 0;
					decompressed_block_type * outputblock = 0;
					
					bool ok = true;
					ok = ok && decompressPending.tryDequeFront(inputblock);
					ok = ok && ((decoder = decoders.getIf()) != 0);
					ok = ok && ((outputblock = decompressedBlockFreeList.getIf()) != 0);
					
					if ( ok )
					{
						BamParallelDecodingDecompressBlockWorkPackage * package = decompressWorkPackages.getPackage();
						*package = BamParallelDecodingDecompressBlockWorkPackage(0,inputblock,outputblock,decoder,decompressDispatcherId);
						STP.enque(package);
					}
					else
					{
						running = false;
						
						if ( outputblock )
							decompressedBlockFreeList.put(outputblock);
						if ( decoder )
							decoders.put(decoder);
						if ( inputblock )
							decompressPending.push_front(inputblock);
					}
				}
			}

			// add a compressed bgzf block to the pending list
			void putBamParallelDecodingInputBlockAddPending(BamParallelDecodingControlInputInfo::input_block_type * block)
			{
				// put package in the pending list
				decompressPending.push_back(block);
				// check whether we have sufficient resources to decompress next package
				checkDecompressPendingList();
			}

			void checkParsePendingList()
			{
				libmaus::parallel::ScopePosixSpinLock slock(nextDecompressedBlockToBeParsedLock);
				libmaus::parallel::ScopePosixSpinLock alock(parseStallSlotLock);
				libmaus::parallel::ScopePosixSpinLock plock(parsePendingLock);

				BamParallelDecodingAlignmentBuffer * algnbuffer = 0;

				#if 0
				{
				std::ostringstream ostr;
				ostr << "checkParsePendingList stall slot " << parseStallSlot;
				STP.addLogStringWithThreadId(ostr.str());
				}
				#endif
								
				if ( 
					parsePending.size() &&
					(parsePending.top().first == nextDecompressedBlockToBeParsed) &&
					(algnbuffer=parseStallSlot)
				)
				{					
					parseStallSlot = 0;
										
					BamParallelDecodingDecompressedPendingObject obj = parsePending.pop();		

					#if 0
					STP.addLogStringWithThreadId("erasing stall slot for block id" + libmaus::util::NumberSerialisation::formatNumber(obj.second->blockid,0));
					#endif
					
					BamParallelDecodingParseBlockWorkPackage * package = parseWorkPackages.getPackage();
					*package = BamParallelDecodingParseBlockWorkPackage(
						0 /* prio */,
						obj.second,
						algnbuffer,
						&parseInfo,
						parseDispatcherId
					);
					STP.enque(package);
				}
				else if ( 
					parsePending.size() && 
					(parsePending.top().first == nextDecompressedBlockToBeParsed) &&
					(algnbuffer = parseBlockFreeList.getIf())
				)
				{
					BamParallelDecodingDecompressedPendingObject obj = parsePending.pop();		
					
					#if 0
					STP.addLogStringWithThreadId("using free block for block id" + libmaus::util::NumberSerialisation::formatNumber(obj.second->blockid,0));
					#endif
										
					BamParallelDecodingParseBlockWorkPackage * package = parseWorkPackages.getPackage();
					*package = BamParallelDecodingParseBlockWorkPackage(
						0 /* prio */,
						obj.second,
						algnbuffer,
						&parseInfo,
						parseDispatcherId
					);
					STP.enque(package);
				}
				#if 0			
				else
				{
					STP.addLogStringWithThreadId("checkParsePendingList no action");			
				}
				#endif
			}

			// return a decompressed block after parsing
			virtual void putBamParallelDecodingDecompressedBlockReturn(BamParallelDecodingDecompressedBlock * block)
			{
				#if 0
				STP.addLogStringWithThreadId("returning block " + libmaus::util::NumberSerialisation::formatNumber(block->blockid,0));
				#endif
			
				// add block to the free list
				decompressedBlockFreeList.put(block);

				// ready for next block
				{
				libmaus::parallel::ScopePosixSpinLock slock(nextDecompressedBlockToBeParsedLock);
				nextDecompressedBlockToBeParsed += 1;
				}

				// check whether we have sufficient resources to decompress next package
				checkDecompressPendingList();			

				// see if we can parse the next block now
				checkParsePendingList();
			}
			
			virtual void putBamParallelDecodingParsedBlockAddPending(BamParallelDecodingAlignmentBuffer * algn)
			{
				parseBlocksSeen += 1;
					
				if ( algn->final )
					lastParseBlockSeen.set(true);
			
				BamParallelDecodingValidateBlockWorkPackage * package = validateWorkPackages.getPackage();
				*package = BamParallelDecodingValidateBlockWorkPackage(
					0 /* prio */,
					algn,
					validateDispatcherId
				);
				STP.enque(package);
			}

			void checkRewritePending()
			{
				BamParallelDecodingAlignmentBuffer * inbuffer = 0;
				bool const pendingok = rewritePending.tryDequeFront(inbuffer);
				BamParallelDecodingAlignmentRewriteBuffer * outbuffer = rewriteBlockFreeList.getIf();
				
				if ( pendingok && (outbuffer != 0) )
				{
					#if 0
					STP.getGlobalLock().lock();
					std::cerr << "checkRewritePending" << std::endl;
					STP.getGlobalLock().unlock();
					#endif
										
					BamParallelDecodingRewriteBlockWorkPackage * package = rewriteWorkPackages.getPackage();

					*package = BamParallelDecodingRewriteBlockWorkPackage(
						0 /* prio */,
						inbuffer,
						outbuffer,
						rewriteDispatcherId
					);

					STP.enque(package);
				}
				else
				{
					if ( outbuffer )
						rewriteBlockFreeList.put(outbuffer);
					if ( pendingok )
						rewritePending.push_front(inbuffer);
				}
			}

			virtual void putBamParallelDecodingValidatedBlockAddPending(BamParallelDecodingAlignmentBuffer * algn, bool const ok)
			{
				if ( ! ok )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "Input validation failed.\n";
					lme.finish();
					throw lme;
				}
			
				readsParsed += algn->fill();

				#if 0
				STP.addLogStringWithThreadId("seen " + libmaus::util::NumberSerialisation::formatNumber(readsParsed,0));	
				#endif

				{
					libmaus::parallel::ScopePosixSpinLock slock(readsParsedLock);
					
					if ( static_cast<uint64_t>(readsParsed) / (1024*1024) != readsParsedLastPrint/(1024*1024) )
					{
						STP.getGlobalLock().lock();				
						std::cerr << "seen " << readsParsed << " low " << algn->low << std::endl;
						STP.getGlobalLock().unlock();
					}
					
					readsParsedLastPrint = static_cast<uint64_t>(readsParsed);
				}
				
				parseBlocksValidated += 1;
				if ( lastParseBlockSeen.get() && parseBlocksValidated == parseBlocksSeen )
					lastParseBlockValidated.set(true);
					
				rewritePending.push_back(algn);
				
				checkRewritePending();
			}

			// add decompressed block to pending list
			virtual void putBamParallelDecodingDecompressedBlockAddPending(BamParallelDecodingDecompressedBlock * block)
			{
				{
				libmaus::parallel::ScopePosixSpinLock plock(parsePendingLock);
				parsePending.push(BamParallelDecodingDecompressedPendingObject(block->blockid,block));
				}

				checkParsePendingList();
			}

			// return a bgzf decoder object		
			virtual void putBamParallelDecodingBgzfInflateZStreamBaseReturn(libmaus::lz::BgzfInflateZStreamBase * decoder)
			{
				// add decoder to the free list
				decoders.put(decoder);
				// check whether we have sufficient resources to decompress next package
				checkDecompressPendingList();
			}

			// return input block after decompression
			virtual void putBamParallelDecodingInputBlockReturn(BamParallelDecodingControlInputInfo::input_block_type * block)
			{
				// put package in the free list
				inputinfo.inputBlockFreeList.put(block);

				// enque next read
				BamParallelDecodingInputBlockWorkPackage * package = readWorkPackages.getPackage();
				*package = BamParallelDecodingInputBlockWorkPackage(0 /* priority */, &inputinfo,readDispatcherId);
				STP.enque(package);
			}

			BamParallelDecodingControl(
				libmaus::parallel::SimpleThreadPool & rSTP, 
				std::istream & ristr, 
				uint64_t const rstreamid,
				uint64_t const rparsebuffersize,
				uint64_t const rrewritebuffersize,
				uint64_t const rnumrewritebuffers
			)
			: 
				STP(rSTP),
				readDispatcher(*this,*this),
				readDispatcherId(STP.getNextDispatcherId()),
				decompressDispatcher(*this,*this,*this,*this),
				decompressDispatcherId(STP.getNextDispatcherId()),
				parseDispatcher(*this,*this,*this,*this,*this),
				parseDispatcherId(STP.getNextDispatcherId()),
				validateDispatcher(*this,*this),
				validateDispatcherId(STP.getNextDispatcherId()),
				rewriteDispatcher(*this,*this,*this,*this,*this),
				rewriteDispatcherId(STP.getNextDispatcherId()),
				baseSortDispatcher(*this),
				baseSortDispatcherId(STP.getNextDispatcherId()),
				mergeSortDispatcher(*this),
				mergeSortDispatcherId(STP.getNextDispatcherId()),
				numthreads(STP.getNumThreads()),
				inputinfo(ristr,rstreamid,numthreads),
				readWorkPackages(),
				Pdecoders(new libmaus::parallel::LockedFreeList<libmaus::lz::BgzfInflateZStreamBase>(numthreads)),
				decoders(*Pdecoders),
				decompressedBlockFreeList(numthreads),
				parseBlockFreeList(numthreads,BamParallelDecodingAlignmentBufferAllocator(rparsebuffersize,2)),
				rewriteBlockFreeList(rnumrewritebuffers,BamParallelDecodingAlignmentRewriteBufferAllocator(rrewritebuffersize,2)),
				nextDecompressedBlockToBeParsed(0),
				parseStallSlot(0),
				lastParseBlockSeen(false),
				readsParsed(0),
				readsParsedLastPrint(0),
				parseBlocksSeen(0),
				parseBlocksValidated(0),
				lastParseBlockValidated(false),
				parseBlocksRewritten(0),
				lastParseBlockRewritten(false),
				numSortBlocksIn(0),
				numSortBlocksOut(0),
				lastSortBlockFinished(false)
			{
				STP.registerDispatcher(readDispatcherId,&readDispatcher);
				STP.registerDispatcher(decompressDispatcherId,&decompressDispatcher);
				STP.registerDispatcher(parseDispatcherId,&parseDispatcher);
				STP.registerDispatcher(validateDispatcherId,&validateDispatcher);
				STP.registerDispatcher(rewriteDispatcherId,&rewriteDispatcher);
				STP.registerDispatcher(baseSortDispatcherId,&baseSortDispatcher);
				STP.registerDispatcher(mergeSortDispatcherId,&mergeSortDispatcher);
			}
			
			void serialTestDecode(std::ostream & out)
			{
				libmaus::timing::RealTimeClock rtc; rtc.start();
			
				bool running = true;
				BamParallelDecodingParseInfo BPDP;
				// BamParallelDecodingAlignmentBuffer BPDAB(2*1024ull*1024ull*1024ull,2);
				// BamParallelDecodingAlignmentBuffer BPDAB(1024ull*1024ull,2);
				BamParallelDecodingAlignmentBuffer BPDAB(1024ull*1024ull,2);
				uint64_t pcnt = 0;
				uint64_t cnt = 0;
				
				// read blocks until no more input available
				while ( running )
				{
					BamParallelDecodingControlInputInfo::input_block_type * inblock = inputinfo.inputBlockFreeList.get();	
					inblock->readBlock(inputinfo.istr);

					decompressed_block_type * outblock = decompressedBlockFreeList.get();
										
					outblock->decompressBlock(decoders,*inblock);

					running = running && (!(outblock->final));

					while ( ! BPDP.parseBlock(*outblock,BPDAB) )
					{
						//BPDAB.checkValidUnpacked();
						BPDAB.reorder();
						// BPDAB.checkValidPacked();
						
						BPDP.putBackLastName(BPDAB);

						cnt += BPDAB.fill();
								
						BPDAB.reset();
						
						if ( cnt / (1024*1024) != pcnt / (1024*1024) )
						{
							std::cerr << "cnt=" << cnt << " als/sec=" << (static_cast<double>(cnt) / rtc.getElapsedSeconds()) << std::endl;
							pcnt = cnt;			
						}
					}
										
					// out.write(outblock->P,outblock->uncompdatasize);
					
					decompressedBlockFreeList.put(outblock);
					inputinfo.inputBlockFreeList.put(inblock);	
				}

				while ( ! BPDP.putBackBufferEmpty() )
				{				
					decompressed_block_type * outblock = decompressedBlockFreeList.get();
					assert ( ! outblock->uncompdatasize );
					
					while ( ! BPDP.parseBlock(*outblock,BPDAB) )
					{
						//BPDAB.checkValidUnpacked();
						BPDAB.reorder();
						// BPDAB.checkValidPacked();
						
						BPDP.putBackLastName(BPDAB);

						cnt += BPDAB.fill();
						
						BPDAB.reset();

						if ( cnt / (1024*1024) != pcnt / (1024*1024) )
						{
							std::cerr << "cnt=" << cnt << " als/sec=" << (static_cast<double>(cnt) / rtc.getElapsedSeconds()) << std::endl;
							pcnt = cnt;			
						}
					}
											
					decompressedBlockFreeList.put(outblock);
				}

				BPDAB.reorder();
				cnt += BPDAB.fill();
				BPDAB.reset();
				
				std::cerr << "cnt=" << cnt << " als/sec=" << (static_cast<double>(cnt) / rtc.getElapsedSeconds()) << std::endl;
			}
			
			static void serialTestDecode1(std::istream & in, std::ostream & out)
			{
				libmaus::parallel::SimpleThreadPool STP(1);
				BamParallelDecodingControl BPDC(STP,in,0/*streamid*/,1024*1024 /* parse buffer size */,1024*1024*1024 /* rewrite buffer size */,4);
				BPDC.serialTestDecode(out);
			}
			
			void enqueReadPackage()
			{
				BamParallelDecodingInputBlockWorkPackage * package = readWorkPackages.getPackage();
				*package = BamParallelDecodingInputBlockWorkPackage(0 /* priority */, &inputinfo,readDispatcherId);
				STP.enque(package);
			}

			static BamParallelDecodingControl * sigobj;

			static void sigusr1(int)
			{
				std::cerr << "decompressPending.size()=" << sigobj->decompressPending.size() << "\n";
				std::cerr << "parsePending.size()=" << sigobj->parsePending.size() << "\n";
				std::cerr << "rewritePending.size()=" << sigobj->rewritePending.size() << "\n";
				std::cerr << "decoders.empty()=" << sigobj->decoders.empty() << "\n";
				std::cerr << "decompressedBlockFreeList.empty()=" << sigobj->decompressedBlockFreeList.empty() << "\n";
				std::cerr << "parseBlockFreeList.empty()=" << sigobj->parseBlockFreeList.empty() << "\n";
				std::cerr << "rewriteBlockFreeList.empty()=" << sigobj->rewriteBlockFreeList.empty() << "\n";
				sigobj->STP.printStateHistogram(std::cerr);
			}

			static void serialParallelDecode1(std::istream & in)
			{
				try
				{
					// libmaus::parallel::SimpleThreadPool STP(12);
					libmaus::parallel::SimpleThreadPool STP(24);
					BamParallelDecodingControl BPDC(STP,in,0/*streamid*/,1024*1024 /* parse buffer size */, 1024*1024*1024 /* rewrite buffer size */,4);
					
					sigobj = & BPDC;
					signal(SIGUSR1,sigusr1);
					
					BPDC.enqueReadPackage();
				
					while ( (!STP.isInPanicMode()) && (! BPDC.lastSortBlockFinished.get()) )
					{
						sleep(1);
					}
				
					STP.terminate();
				
					std::cerr << "number of reads parsed    " << BPDC.readsParsed << std::endl;
					std::cerr << "number of reads rewritten " << BPDC.readsRewritten << std::endl;
					std::cerr << "number of reads sorted " << BPDC.readsSorted << std::endl;
				}
				catch(std::exception const & ex)
				{
					std::cerr << ex.what() << std::endl;
				}
			}
		};	

		template<>		
		BamParallelDecodingControl<BamAlignmentRewriteBufferPosComparator> * BamParallelDecodingControl<BamAlignmentRewriteBufferPosComparator>::sigobj = 0;
	}
}
#endif
