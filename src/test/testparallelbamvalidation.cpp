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
				public BgzfInflateZStreamBaseGetInterface
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

				ControlInputInfo controlInputInfo;

				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<InputBlockWorkPackage> readWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<DecompressBlockWorkPackage> decompressBlockWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<DecompressBlocksWorkPackage> decompressBlocksWorkPackages;
				
				libmaus::parallel::LockedQueue<ControlInputInfo::input_block_type *> decompressPendingQueue;
				
				libmaus::parallel::PosixSpinLock decompressLock;
				libmaus::parallel::LockedFreeList<DecompressedBlock> decompressBlockFreeList;
				
				libmaus::parallel::LockedCounter bgzfin;
				libmaus::parallel::LockedCounter bgzfde;
				libmaus::parallel::LockedBool decompressFinished;
				
				libmaus::parallel::SynchronousCounter<uint64_t> inputBlockReturnCount;
				libmaus::parallel::PosixSpinLock requeReadPendingLock;
				libmaus::parallel::LockedBool requeReadPending;
				
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
					controlInputInfo(in,0,getInputBlockCount()),
					decompressBlockFreeList(getInputBlockCount() * STP.getNumThreads() * 2),
					bgzfin(0),
					bgzfde(0),
					decompressFinished(false),
					requeReadPending(false)
				{
					STP.registerDispatcher(IBWPDid,&IBWPD);
					STP.registerDispatcher(DBWPDid,&DBWPD);
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
				
				// return input block after decompression
				void putInputBlockReturn(ControlInputInfo::input_block_type * block) 
				{ 					
					uint64_t const freecnt = controlInputInfo.inputBlockFreeList.putAndCount(block);
					uint64_t const capacity = controlInputInfo.inputBlockFreeList.capacity();
					
					bool req = false;
					
					{
						bool const pending = requeReadPending.get();
						
						libmaus::parallel::ScopePosixSpinLock llock(requeReadPendingLock);
						if ( 
							(freecnt == capacity) 
							|| 
							(pending && freecnt >= (capacity>>2))
						)
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

				void putDecompressedBlockAddPending(DecompressedBlock * block)
				{
					uint64_t cnt = bgzfde.increment();
					
					if ( controlInputInfo.getEOF() && (static_cast<uint64_t>(bgzfin) == cnt) )
						decompressFinished.set(true);

					decompressBlockFreeList.put(block);
					checkEnqueDecompress();

					if ( (cnt % (16*1024)) == 0 )
					{
						libmaus::parallel::ScopePosixSpinLock glock(STP.getGlobalLock());
						std::cerr << "cnt=" << cnt << std::endl;
					}
					
					if ( decompressFinished.get() )
					{
						decodingFinished.set(true);
					}
				}
			};
		}
	}
}

int main()
{
	uint64_t const numlogcpus = libmaus::parallel::NumCpus::getNumLogicalProcessors();
	libmaus::parallel::SimpleThreadPool STP(numlogcpus);
	libmaus::bambam::parallel::ValidationControl VC(STP,std::cin);
	VC.checkEnqueReadPackage();
	VC.waitDecodingFinished();
	STP.terminate();
}
