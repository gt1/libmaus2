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

#include <libmaus2/bambam/parallel/ControlInputInfo.hpp>
#include <libmaus2/bambam/parallel/InputBlockWorkPackageReturnInterface.hpp>
#include <libmaus2/bambam/parallel/InputBlockAddPendingInterface.hpp>
#include <libmaus2/bambam/parallel/InputBlockWorkPackage.hpp>
#include <libmaus2/parallel/NumCpus.hpp>
#include <libmaus2/parallel/SimpleThreadPool.hpp>
#include <libmaus2/parallel/SimpleThreadPoolWorkPackageFreeList.hpp>
#include <libmaus2/bambam/parallel/InputBlockWorkPackageDispatcher.hpp>

#include <libmaus2/bambam/parallel/RequeReadInterface.hpp>
#include <libmaus2/bambam/parallel/DecompressBlocksWorkPackage.hpp>
#include <libmaus2/bambam/parallel/DecompressBlocksWorkPackageDispatcher.hpp>

#include <libmaus2/bambam/parallel/DecompressBlockWorkPackageReturnInterface.hpp>
#include <libmaus2/bambam/parallel/InputBlockReturnInterface.hpp>
#include <libmaus2/bambam/parallel/DecompressedBlockAddPendingInterface.hpp>
#include <libmaus2/bambam/parallel/BgzfInflateZStreamBaseReturnInterface.hpp>
#include <libmaus2/bambam/parallel/DecompressBlockWorkPackageDispatcher.hpp>
#include <libmaus2/parallel/LockedQueue.hpp>
#include <libmaus2/parallel/LockedCounter.hpp>
#include <libmaus2/bambam/parallel/BgzfInflateZStreamBaseGetInterface.hpp>
#include <libmaus2/bambam/parallel/ParseInfo.hpp>
#include <libmaus2/bambam/parallel/DecompressedPendingObject.hpp>
#include <libmaus2/bambam/parallel/DecompressedPendingObjectHeapComparator.hpp>
#include <libmaus2/parallel/LockedHeap.hpp>
#include <libmaus2/bambam/parallel/ParseBlockWorkPackage.hpp>
#include <libmaus2/bambam/parallel/AlignmentBufferAllocator.hpp>
#include <libmaus2/bambam/parallel/AlignmentBufferTypeInfo.hpp>
#include <libmaus2/bambam/parallel/ParseBlockWorkPackageDispatcher.hpp>

#include <libmaus2/bambam/parallel/DecompressedBlockReturnInterface.hpp>
#include <libmaus2/bambam/parallel/ParsedBlockAddPendingInterface.hpp>
#include <libmaus2/bambam/parallel/ParsedBlockStallInterface.hpp>
#include <libmaus2/bambam/parallel/ParsePackageReturnInterface.hpp>
#include <libmaus2/bambam/parallel/ValidateBlockFragmentWorkPackage.hpp>
#include <libmaus2/bambam/parallel/ValidationFragment.hpp>
#include <libmaus2/bambam/parallel/ValidateBlockFragmentPackageReturnInterface.hpp>
#include <libmaus2/bambam/parallel/ValidateBlockFragmentAddPendingInterface.hpp>
#include <libmaus2/bambam/parallel/ValidateBlockFragmentWorkPackageDispatcher.hpp>

#include <libmaus2/bambam/parallel/DecompressedBlockAllocator.hpp>
#include <libmaus2/bambam/parallel/DecompressedBlockTypeInfo.hpp>

#include <libmaus2/lz/BgzfInflateZStreamBaseAllocator.hpp>
#include <libmaus2/lz/BgzfInflateZStreamBaseTypeInfo.hpp>

#include <libmaus2/bambam/parallel/ChecksumsInterfaceGetInterface.hpp>
#include <libmaus2/bambam/parallel/ChecksumsInterfacePutInterface.hpp>

namespace libmaus2
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
				public ValidateBlockFragmentAddPendingInterface,
				public ChecksumsInterfaceGetInterface,
				public ChecksumsInterfacePutInterface
			{
				static unsigned int getInputBlockCountShift()
				{
					return 8;
				}
				
				static unsigned int getInputBlockCount()
				{
					return 1ull << getInputBlockCountShift();
				}
							
				libmaus2::parallel::LockedBool decodingFinished;
				
				libmaus2::parallel::SimpleThreadPool & STP;
				libmaus2::parallel::LockedFreeList<
					libmaus2::lz::BgzfInflateZStreamBase,
					libmaus2::lz::BgzfInflateZStreamBaseAllocator,
					libmaus2::lz::BgzfInflateZStreamBaseTypeInfo
					> zstreambases;

                                InputBlockWorkPackageDispatcher IBWPD;
                                uint64_t const IBWPDid;
                                DecompressBlocksWorkPackageDispatcher DBWPD;
                                uint64_t const  DBWPDid;
				ParseBlockWorkPackageDispatcher PBWPD;
				uint64_t const PBWPDid;
				ValidateBlockFragmentWorkPackageDispatcher VBFWPD;
				uint64_t const VBFWPDid;

				ControlInputInfo controlInputInfo;

				libmaus2::parallel::SimpleThreadPoolWorkPackageFreeList<InputBlockWorkPackage> readWorkPackages;
				libmaus2::parallel::SimpleThreadPoolWorkPackageFreeList<DecompressBlockWorkPackage> decompressBlockWorkPackages;
				libmaus2::parallel::SimpleThreadPoolWorkPackageFreeList<DecompressBlocksWorkPackage> decompressBlocksWorkPackages;
				libmaus2::parallel::SimpleThreadPoolWorkPackageFreeList<ParseBlockWorkPackage> parseBlockWorkPackages;
				libmaus2::parallel::SimpleThreadPoolWorkPackageFreeList<ValidateBlockFragmentWorkPackage> validateBlockFragmentWorkPackages;
				
				libmaus2::parallel::LockedQueue<ControlInputInfo::input_block_type::shared_ptr_type> decompressPendingQueue;
				
				libmaus2::parallel::PosixSpinLock decompressLock;
				libmaus2::parallel::LockedFreeList<DecompressedBlock,DecompressedBlockAllocator,DecompressedBlockTypeInfo> decompressBlockFreeList;
				
				libmaus2::parallel::LockedCounter bgzfin;
				libmaus2::parallel::LockedCounter bgzfde;
				libmaus2::parallel::LockedBool decompressFinished;
				
				libmaus2::parallel::SynchronousCounter<uint64_t> inputBlockReturnCount;
				libmaus2::parallel::PosixSpinLock requeReadPendingLock;
				libmaus2::parallel::LockedBool requeReadPending;
				
				libmaus2::bambam::parallel::ParseInfo parseInfo;

				// decompressed blocks ready to be parsed
				libmaus2::parallel::LockedHeap<DecompressedPendingObject,DecompressedPendingObjectHeapComparator> parsePending;
				libmaus2::parallel::PosixSpinLock parsePendingLock;

				// id of next block to be parsed
				uint64_t volatile nextDecompressedBlockToBeParsed;
				libmaus2::parallel::PosixSpinLock nextDecompressedBlockToBeParsedLock;

				// stall slot for parsing BAM records
				AlignmentBuffer::shared_ptr_type parseStallSlot;
				libmaus2::parallel::PosixSpinLock parseStallSlotLock;

				// free list for alignment buffers
				libmaus2::parallel::LockedFreeList<AlignmentBuffer,AlignmentBufferAllocator,AlignmentBufferTypeInfo> parseBlockFreeList;

				libmaus2::parallel::LockedBool lastParseBlockSeen;
				libmaus2::parallel::LockedCounter readsParsed;
				libmaus2::parallel::LockedCounter blocksParsed;
				uint64_t volatile readsParsedLastPrint;
				libmaus2::parallel::PosixSpinLock readsParsedLastPrintLock;
				libmaus2::parallel::SynchronousCounter<uint64_t> nextParseBufferId;

				std::map<uint64_t,AlignmentBuffer::shared_ptr_type> validationActive;
				std::map<uint64_t,uint64_t> validationFragmentsPerId;
				std::map<uint64_t,bool> validationOk;
				libmaus2::parallel::PosixSpinLock validationActiveLock;
				libmaus2::parallel::LockedCounter readsValidated;
				libmaus2::parallel::LockedCounter blocksValidated;
				libmaus2::parallel::LockedBool lastParseBlockValidated;

				static uint64_t getParseBufferSize()
				{
					return 8*1024*1024;
				}
				
				ChecksumsInterface::shared_ptr_type getSeqChecksumsObject()
				{
					return ChecksumsInterface::shared_ptr_type();
				}
				
				void returnSeqChecksumsObject(ChecksumsInterface::shared_ptr_type)
				{
				}
				

				ValidationControl(
					libmaus2::parallel::SimpleThreadPool & rSTP,
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
					VBFWPD(*this,*this,*this,*this),
					VBFWPDid(STP.getNextDispatcherId()),
					controlInputInfo(in,0,getInputBlockCount()),
					decompressBlockFreeList(getInputBlockCount() * STP.getNumThreads() * 2),
					bgzfin(0),
					bgzfde(0),
					decompressFinished(false),
					requeReadPending(false),
					nextDecompressedBlockToBeParsed(0),
					parseStallSlot(),
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
					libmaus2::parallel::ScopePosixSpinLock llock(requeReadPendingLock);
					requeReadPending.set(true);
				}
				
				void waitDecompressFinished()
				{
					while ( ! decompressFinished.get() )
						sleep(1);
				}

				void waitDecodingFinished()
				{
					while ( ( ! decodingFinished.get() ) && (!STP.isInPanicMode()) )
					{
						sleep(1);
						// STP.printStateHistogram(std::cerr);
					}
				}

				void putBgzfInflateZStreamBaseReturn(libmaus2::lz::BgzfInflateZStreamBase::shared_ptr_type decoder)
				{
					zstreambases.put(decoder);
				}
				
				libmaus2::lz::BgzfInflateZStreamBase::shared_ptr_type getBgzfInflateZStreamBase()
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
				void putInputBlockReturn(ControlInputInfo::input_block_type::shared_ptr_type block) 
				{ 					
					uint64_t const freecnt = controlInputInfo.inputBlockFreeList.putAndCount(block);
					uint64_t const capacity = controlInputInfo.inputBlockFreeList.capacity();
					
					bool req = false;
					
					{
						libmaus2::parallel::ScopePosixSpinLock llock(requeReadPendingLock);
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
					std::vector<ControlInputInfo::input_block_type::shared_ptr_type> ginblocks;
					std::vector<DecompressedBlock::shared_ptr_type> goutblocks;
					
					{
						libmaus2::parallel::ScopePosixSpinLock plock(decompressPendingQueue.getLock());

						uint64_t const psize = decompressPendingQueue.sizeUnlocked();
							
						if ( ! psize )
							return;
								
						bool const forcerun = decompressPendingQueue.backUnlocked()->final;
						if ( (! forcerun) && (psize < getDecompressBatchSize()) )
							return;
							
						libmaus2::parallel::ScopePosixSpinLock flock(decompressBlockFreeList.getLock());
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
						std::vector<ControlInputInfo::input_block_type::shared_ptr_type> inblocks;
						std::vector<DecompressedBlock::shared_ptr_type> outblocks;

						uint64_t const loops = (ginblocks.size() + getDecompressBatchSize() - 1)/getDecompressBatchSize();
						
						for ( uint64_t l = 0; l < loops; ++l )
						{
							uint64_t const low = l * getDecompressBatchSize();
							uint64_t const high = std::min(low + getDecompressBatchSize(),static_cast<uint64_t>(ginblocks.size()));
							
							std::vector<ControlInputInfo::input_block_type::shared_ptr_type> inblocks(ginblocks.begin()+low,ginblocks.begin()+high);
							std::vector<DecompressedBlock::shared_ptr_type> outblocks(goutblocks.begin()+low,goutblocks.begin()+high);

							DecompressBlocksWorkPackage * package = decompressBlocksWorkPackages.getPackage();

							package->setData(0 /* prio */,inblocks,outblocks,DBWPDid);

							STP.enque(package);
						}
					}
				}

				void putInputBlockAddPending(
					std::deque<ControlInputInfo::input_block_type::shared_ptr_type>::iterator ita,
					std::deque<ControlInputInfo::input_block_type::shared_ptr_type>::iterator ite)
				{
					bgzfin += ite-ita;
					{
						libmaus2::parallel::ScopePosixSpinLock llock(decompressPendingQueue.getLock());
						for ( 
							std::deque<ControlInputInfo::input_block_type::shared_ptr_type>::iterator itc = ita;
							itc != ite; ++itc )
							decompressPendingQueue.push_backUnlocked(*itc);
					}

					checkEnqueDecompress();
				}

				void putInputBlockAddPending(ControlInputInfo::input_block_type::shared_ptr_type package) 
				{
					bgzfin.increment();
					decompressPendingQueue.push_back(package);
					checkEnqueDecompress();
				}

				void checkParsePendingList()
				{
					libmaus2::parallel::ScopePosixSpinLock slock(nextDecompressedBlockToBeParsedLock);
					libmaus2::parallel::ScopePosixSpinLock alock(parseStallSlotLock);
					libmaus2::parallel::ScopePosixSpinLock plock(parsePendingLock);

					#if 0
					STP.getGlobalLock().lock();
					std::cerr << "check pending, parsePending.size()=" << parsePending.size() << " stall slot " 
						<< parseStallSlot 
						<< " next id "
						<< (parsePending.size() ? parsePending.top().first : -1)
						<< std::endl;
					STP.getGlobalLock().unlock();
					#endif
	
					AlignmentBuffer::shared_ptr_type algnbuffer = AlignmentBuffer::shared_ptr_type();
	
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
						parseStallSlot = AlignmentBuffer::shared_ptr_type();
											
						DecompressedPendingObject obj = parsePending.pop();		
	
						#if 0
						STP.addLogStringWithThreadId("erasing stall slot for block id" + libmaus2::util::NumberSerialisation::formatNumber(obj.second->blockid,0));
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
						STP.addLogStringWithThreadId("using free block for block id" + libmaus2::util::NumberSerialisation::formatNumber(obj.second->blockid,0));
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

				void putDecompressedBlockReturn(libmaus2::bambam::parallel::DecompressedBlock::shared_ptr_type block)
				{
					decompressBlockFreeList.put(block);
					checkEnqueDecompress();				

					{
					libmaus2::parallel::ScopePosixSpinLock slock(nextDecompressedBlockToBeParsedLock);
					nextDecompressedBlockToBeParsed += 1;
					}
					
					checkParsePendingList();
				}

				void putParsedBlockStall(AlignmentBuffer::shared_ptr_type algn)
				{
					{
						libmaus2::parallel::ScopePosixSpinLock slock(parseStallSlotLock);
						
						// parse stall slot should be empty
						if ( parseStallSlot )
							STP.printLog(std::cerr);
										
						assert ( parseStallSlot == 0 );

						parseStallSlot = algn;
					}
					
					checkParsePendingList();
				}

				void putParsedBlockAddPending(AlignmentBuffer::shared_ptr_type algn)
				{
					readsParsed += algn->fill();
					blocksParsed += 1;

					{
						libmaus2::parallel::ScopePosixSpinLock sreadsParsedLastPrintLock(readsParsedLastPrintLock);
						
						if ( ((readsParsed >> 20) != (readsParsedLastPrint >> 20)) || algn->final )
						{
							libmaus2::parallel::ScopePosixSpinLock SPSL(STP.getGlobalLock());
							std::cerr << readsParsed << std::endl;
							readsParsedLastPrint = readsParsed;
						}				
					}
					
					if ( algn->final )
						lastParseBlockSeen.set(true);
					
					uint64_t const f = algn->fill();
					uint64_t const readsPerPackage = std::max((f + STP.getNumThreads() - 1)/STP.getNumThreads(),static_cast<uint64_t>(1));
					uint64_t const validationPackages = std::max((f + readsPerPackage - 1)/readsPerPackage, static_cast<uint64_t>(1));
					
					{
						libmaus2::parallel::ScopePosixSpinLock llock(validationActiveLock);
						validationActive[algn->id] = algn;
						validationFragmentsPerId[algn->id] = validationPackages;
						validationOk[algn->id] = true;
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

				void putDecompressedBlockAddPending(DecompressedBlock::shared_ptr_type block)
				{
					uint64_t cnt = bgzfde.increment();
					
					if ( controlInputInfo.getEOF() && (static_cast<uint64_t>(bgzfin) == cnt) )
						decompressFinished.set(true);

					{
						// put block in pending list
						libmaus2::parallel::ScopePosixSpinLock slock(parsePendingLock);
						parsePending.push(DecompressedPendingObject(block->blockid,block));
					}
					
					checkParsePendingList();					
				}
				
				void validateBlockFragmentFinished(ValidationFragment & V, bool const ok)
				{
					readsValidated += (V.high-V.low);
				
					AlignmentBuffer::shared_ptr_type algn = V.buffer;
					bool returnbuffer = false;
					
					{
						libmaus2::parallel::ScopePosixSpinLock llock(validationActiveLock);
						validationOk[algn->id] = validationOk[algn->id] && ok;
						
						if ( -- validationFragmentsPerId[algn->id] == 0 )
						{
							bool const gok = validationOk.find(algn->id)->second;
							if ( ! gok )
							{
								libmaus2::exception::LibMausException lme;
								lme.getStream() << "Validation failed." << std::endl;
								lme.finish();
								throw lme;
							}
						
							validationFragmentsPerId.erase(validationFragmentsPerId.find(algn->id));
							validationActive.erase(validationActive.find(algn->id));
							validationOk.erase(validationOk.find(algn->id));
							returnbuffer = true;
						}
					}
					
					if ( returnbuffer )
					{
						blocksValidated += 1;
						parseBlockFreeList.put(algn);
						checkParsePendingList();			
					}
					
					if ( lastParseBlockSeen.get() && blocksValidated == blocksParsed )
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

#include <libmaus2/aio/PosixFdInputStream.hpp>

int main()
{
	try
	{
		uint64_t const numlogcpus = libmaus2::parallel::NumCpus::getNumLogicalProcessors();
		libmaus2::parallel::SimpleThreadPool STP(numlogcpus);
		libmaus2::aio::PosixFdInputStream in(STDIN_FILENO);
		libmaus2::bambam::parallel::ValidationControl VC(STP,in);
		VC.checkEnqueReadPackage();
		VC.waitDecodingFinished();
		STP.terminate();
		STP.join();
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
