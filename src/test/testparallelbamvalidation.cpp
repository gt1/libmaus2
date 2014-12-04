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

#include <libmaus/bambam/parallel/ControlInputInfo.hpp>
#include <libmaus/bambam/parallel/InputBlockWorkPackageReturnInterface.hpp>
#include <libmaus/bambam/parallel/InputBlockAddPendingInterface.hpp>
#include <libmaus/bambam/parallel/InputBlockWorkPackage.hpp>
#include <libmaus/parallel/NumCpus.hpp>
#include <libmaus/parallel/SimpleThreadPool.hpp>
#include <libmaus/parallel/SimpleThreadPoolWorkPackageFreeList.hpp>
#include <libmaus/bambam/parallel/InputBlockWorkPackageDispatcher.hpp>

#include <libmaus/bambam/parallel/RequeReadInterface.hpp>
#include <libmaus/bambam/parallel/DecompressBlocksWorkPackage.hpp>
#include <libmaus/bambam/parallel/DecompressBlocksWorkPackageDispatcher.hpp>

#include <libmaus/bambam/parallel/DecompressBlockWorkPackageReturnInterface.hpp>
#include <libmaus/bambam/parallel/InputBlockReturnInterface.hpp>
#include <libmaus/bambam/parallel/DecompressedBlockAddPendingInterface.hpp>
#include <libmaus/bambam/parallel/BgzfInflateZStreamBaseReturnInterface.hpp>
#include <libmaus/bambam/parallel/DecompressBlockWorkPackageDispatcher.hpp>
#include <libmaus/parallel/LockedQueue.hpp>
#include <libmaus/parallel/LockedCounter.hpp>
#include <libmaus/bambam/parallel/BgzfInflateZStreamBaseGetInterface.hpp>
#include <libmaus/bambam/parallel/ParseInfo.hpp>
#include <libmaus/bambam/parallel/DecompressedPendingObject.hpp>
#include <libmaus/bambam/parallel/DecompressedPendingObjectHeapComparator.hpp>
#include <libmaus/parallel/LockedHeap.hpp>
#include <libmaus/bambam/parallel/ParseBlockWorkPackage.hpp>
#include <libmaus/bambam/parallel/AlignmentBufferAllocator.hpp>
#include <libmaus/bambam/parallel/ParseBlockWorkPackageDispatcher.hpp>

#include <libmaus/bambam/parallel/DecompressedBlockReturnInterface.hpp>
#include <libmaus/bambam/parallel/ParsedBlockAddPendingInterface.hpp>
#include <libmaus/bambam/parallel/ParsedBlockStallInterface.hpp>
#include <libmaus/bambam/parallel/ParsePackageReturnInterface.hpp>
#include <libmaus/bambam/parallel/ValidateBlockFragmentWorkPackage.hpp>
#include <libmaus/bambam/parallel/ValidationFragment.hpp>
#include <libmaus/bambam/parallel/ValidateBlockFragmentPackageReturnInterface.hpp>
#include <libmaus/bambam/parallel/ValidateBlockFragmentAddPendingInterface.hpp>
#include <libmaus/bambam/parallel/ValidateBlockFragmentWorkPackageDispatcher.hpp>

namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{
			struct ValidationControl :
				public InputBlockWorkPackageReturnInterface,
				public InputBlockAddPendingInterface,
				public DecompressBlockWorkPackageReturnInterface,
				public InputBlockReturnInterface,
				public DecompressedBlockAddPendingInterface,
				public BgzfInflateZStreamBaseReturnInterface,
				public RequeReadInterface,
				public DecompressBlocksWorkPackageReturnInterface,
				public BgzfInflateZStreamBaseGetInterface,
				public DecompressedBlockReturnInterface,
				public ParsedBlockAddPendingInterface,
				public ParsedBlockStallInterface,
				public ParsePackageReturnInterface,
				public ValidateBlockFragmentPackageReturnInterface,
				public ValidateBlockFragmentAddPendingInterface
			{
				static unsigned int getInputBlockCountShift()
				{
					return 8;
				}
				
				static unsigned int getInputBlockCount()
				{
					return 1ull << getInputBlockCountShift();
				}
							
				libmaus::parallel::LockedBool decodingFinished;
				
				libmaus::parallel::SimpleThreadPool & STP;
				libmaus::parallel::LockedFreeList<libmaus::lz::BgzfInflateZStreamBase> zstreambases;

                                InputBlockWorkPackageDispatcher IBWPD;
                                uint64_t const IBWPDid;
                                DecompressBlocksWorkPackageDispatcher DBWPD;
                                uint64_t const  DBWPDid;
				ParseBlockWorkPackageDispatcher PBWPD;
				uint64_t const PBWPDid;
				ValidateBlockFragmentWorkPackageDispatcher VBFWPD;
				uint64_t const VBFWPDid;

				ControlInputInfo controlInputInfo;

				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<InputBlockWorkPackage> readWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<DecompressBlockWorkPackage> decompressBlockWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<DecompressBlocksWorkPackage> decompressBlocksWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<ParseBlockWorkPackage> parseBlockWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<ValidateBlockFragmentWorkPackage> validateBlockFragmentWorkPackages;
				
				libmaus::parallel::LockedQueue<ControlInputInfo::input_block_type *> decompressPendingQueue;
				
				libmaus::parallel::PosixSpinLock decompressLock;
				libmaus::parallel::LockedFreeList<DecompressedBlock> decompressBlockFreeList;
				
				libmaus::parallel::LockedCounter bgzfin;
				libmaus::parallel::LockedCounter bgzfde;
				libmaus::parallel::LockedBool decompressFinished;
				
				libmaus::parallel::SynchronousCounter<uint64_t> inputBlockReturnCount;
				libmaus::parallel::PosixSpinLock requeReadPendingLock;
				libmaus::parallel::LockedBool requeReadPending;
				
				libmaus::bambam::parallel::ParseInfo parseInfo;

				// decompressed blocks ready to be parsed
				libmaus::parallel::LockedHeap<DecompressedPendingObject,DecompressedPendingObjectHeapComparator> parsePending;
				libmaus::parallel::PosixSpinLock parsePendingLock;

				// id of next block to be parsed
				uint64_t volatile nextDecompressedBlockToBeParsed;
				libmaus::parallel::PosixSpinLock nextDecompressedBlockToBeParsedLock;

				// stall slot for parsing BAM records
				AlignmentBuffer * volatile parseStallSlot;
				libmaus::parallel::PosixSpinLock parseStallSlotLock;

				// free list for alignment buffers
				libmaus::parallel::LockedFreeList<AlignmentBuffer,AlignmentBufferAllocator> parseBlockFreeList;

				libmaus::parallel::LockedBool lastParseBlockSeen;
				libmaus::parallel::LockedCounter readsParsed;
				uint64_t volatile readsParsedLastPrint;
				libmaus::parallel::PosixSpinLock readsParsedLastPrintLock;
				libmaus::parallel::SynchronousCounter<uint64_t> nextParseBufferId;

				std::map<uint64_t,AlignmentBuffer *> mutable validationActive;
				std::map<uint64_t,uint64_t> mutable validationFragmentsPerId;
				libmaus::parallel::PosixSpinLock validationActiveLock;
				libmaus::parallel::LockedCounter readsValidated;
				libmaus::parallel::LockedBool lastParseBlockValidated;

				static uint64_t getParseBufferSize()
				{
					return 8*1024*1024;
				}

				ValidationControl(
					libmaus::parallel::SimpleThreadPool & rSTP,
					std::istream & in
				)
				: 
					decodingFinished(false),
					STP(rSTP), 
					zstreambases(STP.getNumThreads()),
					IBWPD(*this,*this,*this),
					IBWPDid(STP.getNextDispatcherId()),
					DBWPD(*this,*this,*this,*this,*this),
					DBWPDid(STP.getNextDispatcherId()),
					PBWPD(*this,*this,*this,*this,*this),
					PBWPDid(STP.getNextDispatcherId()),
					VBFWPD(*this,*this),
					VBFWPDid(STP.getNextDispatcherId()),
					controlInputInfo(in,0,getInputBlockCount()),
					decompressBlockFreeList(getInputBlockCount() * STP.getNumThreads() * 2),
					bgzfin(0),
					bgzfde(0),
					decompressFinished(false),
					requeReadPending(false),
					nextDecompressedBlockToBeParsed(0),
					parseStallSlot(0),
					parseBlockFreeList(STP.getNumThreads(),AlignmentBufferAllocator(getParseBufferSize(),2 /* pointer multiplier */)),
					lastParseBlockSeen(false),
					lastParseBlockValidated(false)
				{
					STP.registerDispatcher(IBWPDid,&IBWPD);
					STP.registerDispatcher(DBWPDid,&DBWPD);
					STP.registerDispatcher(PBWPDid,&PBWPD);
					STP.registerDispatcher(VBFWPDid,&VBFWPD);
				}


				void enqueReadPackage()
				{
					InputBlockWorkPackage * package = readWorkPackages.getPackage();
					*package = InputBlockWorkPackage(10 /* priority */, &controlInputInfo,IBWPDid);
					STP.enque(package);
				}

				void checkEnqueReadPackage()
				{
					if ( ! controlInputInfo.getEOF() )
						enqueReadPackage();
				}
				
				void requeRead()
				{
					libmaus::parallel::ScopePosixSpinLock llock(requeReadPendingLock);
					requeReadPending.set(true);
				}
				
				void waitDecompressFinished()
				{
					while ( ! decompressFinished.get() )
						sleep(1);
				}

				void waitDecodingFinished()
				{
					while ( ! decodingFinished.get() )
					{
						sleep(1);
						// STP.printStateHistogram(std::cerr);
					}
				}

				void putBgzfInflateZStreamBaseReturn(libmaus::lz::BgzfInflateZStreamBase * decoder)
				{
					zstreambases.put(decoder);
				}
				
				libmaus::lz::BgzfInflateZStreamBase * getBgzfInflateZStreamBase()
				{
					return zstreambases.get();
				}

				// work package return routines				
				void putInputBlockWorkPackage(InputBlockWorkPackage * package) { readWorkPackages.returnPackage(package); }
				void putDecompressBlockWorkPackage(DecompressBlockWorkPackage * package) { decompressBlockWorkPackages.returnPackage(package); }				
				void putDecompressBlocksWorkPackage(DecompressBlocksWorkPackage * package) { decompressBlocksWorkPackages.returnPackage(package); }
				void putReturnParsePackage(ParseBlockWorkPackage * package) { parseBlockWorkPackages.returnPackage(package); }
				void putReturnValidateBlockFragmentPackage(ValidateBlockFragmentWorkPackage * package) { validateBlockFragmentWorkPackages.returnPackage(package); }
				
				// return input block after decompression
				void putInputBlockReturn(ControlInputInfo::input_block_type * block) 
				{ 					
					uint64_t const freecnt = controlInputInfo.inputBlockFreeList.putAndCount(block);
					uint64_t const capacity = controlInputInfo.inputBlockFreeList.capacity();
					
					bool req = false;
					
					{
						libmaus::parallel::ScopePosixSpinLock llock(requeReadPendingLock);
						bool const pending = requeReadPending.get();
						
						if ( (pending && freecnt >= (capacity>>2)) )
						{
							req = true;
							requeReadPending.set(false);
						}
					}

					if ( req )
						checkEnqueReadPackage();
				}
				
				static uint64_t getDecompressBatchSize()
				{
					return 4;
				}

				void checkEnqueDecompress()
				{
					std::vector<ControlInputInfo::input_block_type *> ginblocks;
					std::vector<DecompressedBlock *> goutblocks;
					
					{
						libmaus::parallel::ScopePosixSpinLock plock(decompressPendingQueue.getLock());

						uint64_t const psize = decompressPendingQueue.sizeUnlocked();
							
						if ( ! psize )
							return;
								
						bool const forcerun = decompressPendingQueue.backUnlocked()->final;
						if ( (! forcerun) && (psize < getDecompressBatchSize()) )
							return;
							
						libmaus::parallel::ScopePosixSpinLock flock(decompressBlockFreeList.getLock());
						uint64_t const fsize = decompressBlockFreeList.freeUnlocked();
						if ( (! forcerun) && (fsize < getDecompressBatchSize()) )
							return;
							
						uint64_t const csize = std::min(psize,fsize);
						for ( uint64_t i = 0; i < csize; ++i )
						{
							ginblocks.push_back(decompressPendingQueue.dequeFrontUnlocked());
							goutblocks.push_back(decompressBlockFreeList.getUnlocked());					
						}
					}
				
					if ( ginblocks.size() )
					{
						std::vector<ControlInputInfo::input_block_type *> inblocks;
						std::vector<DecompressedBlock *> outblocks;

						uint64_t const loops = (ginblocks.size() + getDecompressBatchSize() - 1)/getDecompressBatchSize();
						
						for ( uint64_t l = 0; l < loops; ++l )
						{
							uint64_t const low = l * getDecompressBatchSize();
							uint64_t const high = std::min(low + getDecompressBatchSize(),static_cast<uint64_t>(ginblocks.size()));
							
							std::vector<ControlInputInfo::input_block_type *> inblocks(ginblocks.begin()+low,ginblocks.begin()+high);
							std::vector<DecompressedBlock *> outblocks(goutblocks.begin()+low,goutblocks.begin()+high);

							DecompressBlocksWorkPackage * package = decompressBlocksWorkPackages.getPackage();

							package->setData(0 /* prio */,inblocks,outblocks,DBWPDid);

							STP.enque(package);
						}
					}
				}

				void putInputBlockAddPending(
					std::deque<ControlInputInfo::input_block_type *>::iterator ita,
					std::deque<ControlInputInfo::input_block_type *>::iterator ite)
				{
					bgzfin += ite-ita;
					{
						libmaus::parallel::ScopePosixSpinLock llock(decompressPendingQueue.getLock());
						for ( 
							std::deque<ControlInputInfo::input_block_type *>::iterator itc = ita;
							itc != ite; ++itc )
							decompressPendingQueue.push_backUnlocked(*itc);
					}

					checkEnqueDecompress();
				}

				void putInputBlockAddPending(ControlInputInfo::input_block_type * package) 
				{
					bgzfin.increment();
					decompressPendingQueue.push_back(package);
					checkEnqueDecompress();
				}

				void checkParsePendingList()
				{
					libmaus::parallel::ScopePosixSpinLock slock(nextDecompressedBlockToBeParsedLock);
					libmaus::parallel::ScopePosixSpinLock alock(parseStallSlotLock);
					libmaus::parallel::ScopePosixSpinLock plock(parsePendingLock);

					#if 0
					STP.getGlobalLock().lock();
					std::cerr << "check pending, parsePending.size()=" << parsePending.size() << " stall slot " 
						<< parseStallSlot 
						<< " next id "
						<< (parsePending.size() ? parsePending.top().first : -1)
						<< std::endl;
					STP.getGlobalLock().unlock();
					#endif
	
					AlignmentBuffer * algnbuffer = 0;
	
					#if 0
					{
					std::ostringstream ostr;
					ostr << "checkParsePendingList stall slot " << parseStallSlot;
					STP.addLogStringWithThreadId(ostr.str());
					}
					#endif
				
					// is there a buffer in the stall slot?					
					if ( 
						parsePending.size() &&
						(parsePending.top().first == nextDecompressedBlockToBeParsed) &&
						(algnbuffer=parseStallSlot)
					)
					{					
						parseStallSlot = 0;
											
						DecompressedPendingObject obj = parsePending.pop();		
	
						#if 0
						STP.addLogStringWithThreadId("erasing stall slot for block id" + libmaus::util::NumberSerialisation::formatNumber(obj.second->blockid,0));
						#endif
						
						ParseBlockWorkPackage * package = parseBlockWorkPackages.getPackage();
						*package = ParseBlockWorkPackage(
							0 /* prio */,
							obj.second,
							algnbuffer,
							&parseInfo,
							PBWPDid
						);
						STP.enque(package);
					}
					// do we have a buffer in the free list
					else if ( 
						parsePending.size() && 
						(parsePending.top().first == nextDecompressedBlockToBeParsed) &&
						(algnbuffer = parseBlockFreeList.getIf())
					)
					{
						algnbuffer->reset();
						algnbuffer->id = nextParseBufferId++;
					
						DecompressedPendingObject obj = parsePending.pop();		
						
						#if 0
						STP.addLogStringWithThreadId("using free block for block id" + libmaus::util::NumberSerialisation::formatNumber(obj.second->blockid,0));
						#endif
											
						ParseBlockWorkPackage * package = parseBlockWorkPackages.getPackage();
						*package = ParseBlockWorkPackage(
							0 /* prio */,
							obj.second,
							algnbuffer,
							&parseInfo,
							PBWPDid
						);
						STP.enque(package);
					}
					#if 1
					else
					{
						STP.addLogStringWithThreadId("checkParsePendingList no action");			
					}
					#endif
				}

				void putDecompressedBlockReturn(libmaus::bambam::parallel::DecompressedBlock* block)
				{
					decompressBlockFreeList.put(block);
					checkEnqueDecompress();				

					{
					libmaus::parallel::ScopePosixSpinLock slock(nextDecompressedBlockToBeParsedLock);
					nextDecompressedBlockToBeParsed += 1;
					}
					
					checkParsePendingList();
				}

				void putParsedBlockStall(AlignmentBuffer * algn)
				{
					{
						libmaus::parallel::ScopePosixSpinLock slock(parseStallSlotLock);
						
						// parse stall slot should be empty
						if ( parseStallSlot )
							STP.printLog(std::cerr);
										
						assert ( parseStallSlot == 0 );

						parseStallSlot = algn;
					}
					
					checkParsePendingList();
				}

				void putParsedBlockAddPending(AlignmentBuffer * algn)
				{
					readsParsed += algn->fill();

					{
						libmaus::parallel::ScopePosixSpinLock sreadsParsedLastPrintLock(readsParsedLastPrintLock);
						
						if ( ((readsParsed >> 20) != (readsParsedLastPrint >> 20)) || algn->final )
						{
							libmaus::parallel::ScopePosixSpinLock SPSL(STP.getGlobalLock());
							std::cerr << readsParsed << std::endl;
							readsParsedLastPrint = readsParsed;
						}				
					}
					
					if ( algn->final )
						lastParseBlockSeen.set(true);
					
					uint64_t const f = algn->fill();
					uint64_t const readsPerPackage = (f + STP.getNumThreads() - 1)/STP.getNumThreads();
					uint64_t const validationPackages = std::max((f + readsPerPackage - 1)/readsPerPackage, static_cast<uint64_t>(1));
					
					{
						libmaus::parallel::ScopePosixSpinLock llock(validationActiveLock);
						validationActive[algn->id] = algn;
						validationFragmentsPerId[algn->id] = validationPackages;
					}

					for ( uint64_t p = 0; p < validationPackages; ++p )
					{
						uint64_t const low = std::min(p*readsPerPackage,f);
						uint64_t const high = std::min((p+1)*readsPerPackage,f);
						ValidateBlockFragmentWorkPackage * package = validateBlockFragmentWorkPackages.getPackage();
						*package = ValidateBlockFragmentWorkPackage(0/*prio*/,ValidationFragment(low,high,algn),VBFWPDid);
						STP.enque(package);
					}						
				}

				void putDecompressedBlockAddPending(DecompressedBlock * block)
				{
					uint64_t cnt = bgzfde.increment();
					
					if ( controlInputInfo.getEOF() && (static_cast<uint64_t>(bgzfin) == cnt) )
						decompressFinished.set(true);

					{
						// put block in pending list
						libmaus::parallel::ScopePosixSpinLock slock(parsePendingLock);
						parsePending.push(DecompressedPendingObject(block->blockid,block));
					}
					
					checkParsePendingList();					
				}
				
				void validateBlockFragmentFinished(ValidationFragment & V, bool const ok)
				{
					readsValidated += (V.high-V.low);
				
					AlignmentBuffer * algn = V.buffer;
					bool returnbuffer = false;
					
					{
						libmaus::parallel::ScopePosixSpinLock llock(validationActiveLock);
						if ( -- validationFragmentsPerId[algn->id] == 0 )
							returnbuffer = true;
					}
					
					if ( returnbuffer )
					{
						parseBlockFreeList.put(algn);
						checkParsePendingList();			
					}
					
					if ( lastParseBlockSeen.get() && readsValidated == readsParsed )
					{
						lastParseBlockValidated.set(true);
					}

					if ( lastParseBlockValidated.get() )
						decodingFinished.set(true);		
				}
			};
		}
	}
}

#include <libmaus/aio/PosixFdInputStream.hpp>

int main()
{
	uint64_t const numlogcpus = libmaus::parallel::NumCpus::getNumLogicalProcessors();
	libmaus::parallel::SimpleThreadPool STP(numlogcpus);
	libmaus::aio::PosixFdInputStream in(STDIN_FILENO);
	libmaus::bambam::parallel::ValidationControl VC(STP,in);
	VC.checkEnqueReadPackage();
	VC.waitDecodingFinished();
	STP.terminate();
}
