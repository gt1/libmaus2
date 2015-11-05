/*
    libmaus2
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2015 Genome Research Limited

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
#if ! defined(LIBMAUS2_BAMBAM_PARALLEL_FASTQTOBAMCONTROL_HPP)
#define LIBMAUS2_BAMBAM_PARALLEL_FASTQTOBAMCONTROL_HPP

#include <libmaus2/bambam/parallel/FastqParsePackageDispatcher.hpp>
#include <libmaus2/bambam/parallel/FastqInputPackageDispatcher.hpp>
#include <libmaus2/bambam/parallel/GenericInputControlBlockCompressionWorkPackageDispatcher.hpp>
#include <libmaus2/bambam/parallel/GenericInputControlBlocksWritePackageDispatcher.hpp>
#include <libmaus2/parallel/SimpleThreadPool.hpp>
#include <libmaus2/parallel/SimpleThreadPoolWorkPackageFreeList.hpp>
#include <libmaus2/lz/BgzfDeflateOutputBufferBaseAllocator.hpp>
#include <libmaus2/lz/BgzfDeflateOutputBufferBaseTypeInfo.hpp>
#include <libmaus2/parallel/LockedGrowingFreeList.hpp>
#include <libmaus2/lz/BgzfDeflateZStreamBaseAllocator.hpp>
#include <libmaus2/lz/BgzfDeflateZStreamBaseTypeInfo.hpp>
#include <libmaus2/bambam/parallel/GenericInputControlCompressionPendingHeapComparator.hpp>

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{

			struct FastqToBamControl :
				public FastqInputPackageReturnInterface,
				public FastqInputPackageAddPendingInterface,
				public FastqParsePackageReturnInterface,
				public FastqParsePackageFinishedInterface,
				public GenericInputControlGetCompressorInterface,
				public GenericInputControlPutCompressorInterface,
				public GenericInputControlBlockCompressionWorkPackageReturnInterface,
				public GenericInputControlBlockCompressionFinishedInterface,
				public GenericInputControlBlockWritePackageBlockWrittenInterface,
				public GenericInputControlBlocksWritePackageReturnInterface
			{
				typedef FastqToBamControl this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				std::istream & in;
				std::ostream & out;

				libmaus2::parallel::SimpleThreadPool & STP;
				FastQInputDesc desc;

				libmaus2::parallel::SimpleThreadPoolWorkPackageFreeList<libmaus2::bambam::parallel::FastqInputPackage> readWorkPackages;
				libmaus2::parallel::SimpleThreadPoolWorkPackageFreeList<libmaus2::bambam::parallel::FastqParsePackage> parseWorkPackages;
				libmaus2::parallel::SimpleThreadPoolWorkPackageFreeList<libmaus2::bambam::parallel::GenericInputControlBlockCompressionWorkPackage> compressionWorkPackages;
				libmaus2::parallel::SimpleThreadPoolWorkPackageFreeList<libmaus2::bambam::parallel::GenericInputControlBlocksWritePackage> writevWorkPackages;

				FastqInputPackageDispatcher FIPD;
				uint64_t const FIPDid;
				FastqParsePackageDispatcher FPPD;
				uint64_t const FPPDid;
				GenericInputControlBlockCompressionWorkPackageDispatcher GICBCWPD;
				uint64_t const GICBCWPDid;
				GenericInputControlBlocksWritePackageDispatcher GICBsWPD;
				uint64_t const GICBsWPDid;

				bool volatile readingFinished;
				libmaus2::parallel::PosixSpinLock readingFinishedLock;

				bool volatile parsingFinished;
				libmaus2::parallel::PosixSpinLock parsingFinishedLock;

				uint64_t volatile nextcompblockid;
				libmaus2::parallel::PosixSpinLock nextcompblockidlock;

				std::deque<GenericInputControlCompressionPending> compresspendingqueue;
				libmaus2::parallel::PosixSpinLock compresspendingqueuelock;

				std::map < int64_t, uint64_t > compressionunfinished;
				libmaus2::parallel::PosixSpinLock compressionunfinishedlock;

				std::map < int64_t, libmaus2::bambam::parallel::DecompressedBlock::shared_ptr_type > compressionactive;
				libmaus2::parallel::PosixSpinLock compressionactivelock;

				libmaus2::parallel::LockedFreeList<
					libmaus2::lz::BgzfDeflateOutputBufferBase,
					libmaus2::lz::BgzfDeflateOutputBufferBaseAllocator,
					libmaus2::lz::BgzfDeflateOutputBufferBaseTypeInfo
				> zbufferfreelist;

				libmaus2::parallel::LockedGrowingFreeList<
					libmaus2::lz::BgzfDeflateZStreamBase,
					libmaus2::lz::BgzfDeflateZStreamBaseAllocator,
					libmaus2::lz::BgzfDeflateZStreamBaseTypeInfo
				> compfreelist;

				std::priority_queue<
					GenericInputControlCompressionPending,
					std::vector<GenericInputControlCompressionPending>,
					GenericInputControlCompressionPendingHeapComparator
				> writependingqueue;
				libmaus2::parallel::PosixSpinLock writependingqueuelock;
				uint64_t volatile writependingqueuenext;

				bool volatile compressionfinished;
				libmaus2::parallel::PosixSpinLock compressionfinishedlock;

				libmaus2::lz::BgzfDeflateZStreamBase::shared_ptr_type genericInputControlGetCompressor()
				{
					return compfreelist.get();
				}

				void genericInputControlPutCompressor(libmaus2::lz::BgzfDeflateZStreamBase::shared_ptr_type comp)
				{
					compfreelist.put(comp);
				}

				void fastqParsePackageReturn(FastqParsePackage * package)
				{
					parseWorkPackages.returnPackage(package);
				}

				void genericInputControlBlocksWritePackageReturn(libmaus2::bambam::parallel::GenericInputControlBlocksWritePackage * package)
				{
					writevWorkPackages.returnPackage(package);
				}

				void genericInputControlBlockWritePackageBlockWritten(libmaus2::bambam::parallel::GenericInputControlCompressionPending GICCP)
				{
					zbufferfreelist.put(GICCP.outblock);
					checkCompressionPendingQueue();

					{
					libmaus2::parallel::ScopePosixSpinLock slock(writependingqueuelock);
					writependingqueuenext += 1;
					}

					checkWritePendingQueue();
				}

				void checkWritePendingQueue()
				{
					std::vector < GenericInputControlCompressionPending > Q;

					{
						libmaus2::parallel::ScopePosixSpinLock slock(writependingqueuelock);

						while (
							writependingqueue.size() &&
							writependingqueue.top().absid == (writependingqueuenext + Q.size())
						)
						{
							GenericInputControlCompressionPending GICCP = writependingqueue.top();
							writependingqueue.pop();

							if ( GICCP.final )
							{
								compressionfinishedlock.lock();
								compressionfinished = true;
								compressionfinishedlock.unlock();
							}

							Q.push_back(GICCP);

						}
					}

					libmaus2::bambam::parallel::GenericInputControlBlocksWritePackage * package = writevWorkPackages.getPackage();
					*package = libmaus2::bambam::parallel::GenericInputControlBlocksWritePackage(0/*prio*/,GICBsWPDid,Q,&out);
					STP.enque(package);
				}

				void genericInputControlBlockCompressionFinished(GenericInputControlCompressionPending GICCP)
				{
					uint64_t const blockid = GICCP.blockid;

					{
						libmaus2::parallel::ScopePosixSpinLock slock(compressionunfinishedlock);
						assert ( compressionunfinished.find(blockid) != compressionunfinished.end() );
						if ( ! --compressionunfinished[blockid] )
						{
							compressionunfinished.erase(compressionunfinished.find(blockid));

							libmaus2::bambam::parallel::DecompressedBlock::shared_ptr_type deblock;

							{
							libmaus2::parallel::ScopePosixSpinLock llock(compressionactivelock);
							assert ( compressionactive.find(blockid) != compressionactive.end() );
							deblock = compressionactive.find(blockid)->second;
							}

							desc.decompfreelist.put(deblock);
							checkSubReadPendingQueue();
						}
					}

					{
						libmaus2::parallel::ScopePosixSpinLock slock(writependingqueuelock);
						writependingqueue.push(GICCP);
					}

					checkWritePendingQueue();
				}

				void checkCompressionPendingQueue()
				{
					std::vector<GenericInputControlCompressionPending> readylist;

					{
						libmaus2::parallel::ScopePosixSpinLock slock(compresspendingqueuelock);
						libmaus2::lz::BgzfDeflateOutputBufferBase::shared_ptr_type obuf;

						while (
							compresspendingqueue.size()
							&&
							(obuf = zbufferfreelist.getIf())
						)
						{
							GenericInputControlCompressionPending GICCP = compresspendingqueue.front();
							compresspendingqueue.pop_front();
							GICCP.outblock = obuf;

							readylist.push_back(GICCP);
						}
					}

					for ( uint64_t i = 0; i < readylist.size(); ++i )
					{
						GenericInputControlCompressionPending GICCP = readylist[i];

						libmaus2::bambam::parallel::GenericInputControlBlockCompressionWorkPackage * package =
							compressionWorkPackages.getPackage();
						*package = libmaus2::bambam::parallel::GenericInputControlBlockCompressionWorkPackage(0, GICBCWPDid, GICCP);

						STP.enque(package);
					}
				}

				void checkParseReorderQueue()
				{
					bool finished = false;

					{
						libmaus2::parallel::ScopePosixSpinLock slock(desc.parsereorderqueuelock);

						while ( desc.parsereorderqueue.size() && desc.parsereorderqueue.top()->blockid == desc.parsereorderqueuenext )
						{
							libmaus2::bambam::parallel::DecompressedBlock::shared_ptr_type deblock = desc.parsereorderqueue.top();
							desc.parsereorderqueue.pop();

							uint64_t const maxblocksize = libmaus2::lz::BgzfConstants::getBgzfMaxBlockSize();
							uint64_t const tnumblocks = (deblock->uncompdatasize + maxblocksize - 1)/maxblocksize;
							uint64_t const bytesperblock = tnumblocks ? ((deblock->uncompdatasize+tnumblocks-1)/tnumblocks) : 0;
							uint64_t const numblocks = bytesperblock ? ((deblock->uncompdatasize+bytesperblock-1)/bytesperblock) : 0;

							uint64_t const blockid = deblock->blockid;

							{
								libmaus2::parallel::ScopePosixSpinLock llock(compressionunfinishedlock);
								compressionunfinished[blockid] += numblocks;
							}

							{
								libmaus2::parallel::ScopePosixSpinLock llock(compressionactivelock);
								compressionactive[blockid] = deblock;
							}

							for ( uint64_t i = 0; i < numblocks; ++i )
							{
								uint64_t const low = i*bytesperblock;
								uint64_t const high = std::min(low + bytesperblock, deblock->uncompdatasize);

								nextcompblockidlock.lock();
								uint64_t const absid = nextcompblockid++;
								nextcompblockidlock.unlock();

								GenericInputControlCompressionPending GICCP(
									blockid,
									i,
									absid,
									deblock->final && (i+1 == numblocks),
									std::pair<uint8_t *,uint8_t *>(
										reinterpret_cast<uint8_t *>(deblock->D.begin()) + low,
										reinterpret_cast<uint8_t *>(deblock->D.begin()) + high
									)
								);

								{
									libmaus2::parallel::ScopePosixSpinLock llock(compresspendingqueuelock);
									compresspendingqueue.push_back(GICCP);
								}
							}

							if ( deblock->final )
								finished = true;

							desc.parsereorderqueuenext += 1;
						}
					}

					checkCompressionPendingQueue();

					if ( finished )
					{
						parsingFinishedLock.lock();
						parsingFinished = true;
						parsingFinishedLock.unlock();
					}
				}

				void fastqParsePackageFinished(FastqToBamControlSubReadPending data)
				{
					if ( data.block->meta.returnBlock() )
					{
						desc.blockFreeList.put(data.block);
						if ( ! desc.getEOF() )
							enqueReadPackage();
					}

					{
						libmaus2::parallel::ScopePosixSpinLock slock(desc.parsereorderqueuelock);
						desc.parsereorderqueue.push(data.deblock);
					}

					checkParseReorderQueue();
				}

				FastqToBamControl(
					std::istream & rin,
					std::ostream & rout,
					libmaus2::parallel::SimpleThreadPool & rSTP,
					uint64_t const numblocks, uint64_t const blocksize,
					uint64_t const numoutblocks,
					int const level,
					std::string const rgid
				)
				: in(rin), out(rout), STP(rSTP), desc(in,numblocks,blocksize,0/*streamid*/),
				  FIPD(*this,*this), FIPDid(STP.getNextDispatcherId()),
				  FPPD(*this,*this,rgid), FPPDid(STP.getNextDispatcherId()),
				  GICBCWPD(*this,*this,*this,*this), GICBCWPDid(STP.getNextDispatcherId()),
				  GICBsWPD(*this,*this), GICBsWPDid(STP.getNextDispatcherId()),
				  readingFinished(false), readingFinishedLock(),
				  nextcompblockid(0), nextcompblockidlock(),
				  zbufferfreelist(numoutblocks,libmaus2::lz::BgzfDeflateOutputBufferBaseAllocator(level)),
				  writependingqueuenext(0),
				  compressionfinished(false)
				{
					STP.registerDispatcher(FIPDid,&FIPD);
					STP.registerDispatcher(FPPDid,&FPPD);
					STP.registerDispatcher(GICBCWPDid,&GICBCWPD);
					STP.registerDispatcher(GICBsWPDid,&GICBsWPD);
				}

				void enqueReadPackage()
				{
					libmaus2::bambam::parallel::FastqInputPackage * package = readWorkPackages.getPackage();
					*package = libmaus2::bambam::parallel::FastqInputPackage(
						10/*priority*/,
						FIPDid/*dispid*/,
						&desc
					);
					STP.enque(package);
				}

				void fastqInputPackageReturn(FastqInputPackage * package)
				{
					readWorkPackages.returnPackage(package);
				}

				void genericInputControlBlockCompressionWorkPackageReturn(libmaus2::bambam::parallel::GenericInputControlBlockCompressionWorkPackage * package)
				{
					compressionWorkPackages.returnPackage(package);
				}

				void checkSubReadPendingQueue()
				{
					std::vector<FastqToBamControlSubReadPending> readyList;

					{
						libmaus2::parallel::ScopePosixSpinLock llock(desc.readpendingsubqueuelock);
						libmaus2::bambam::parallel::DecompressedBlock::shared_ptr_type deblock;

						while (
							desc.readpendingsubqueue.size()
							&&
							(deblock = desc.decompfreelist.getIf())
						)
						{
							FastqToBamControlSubReadPending t = desc.readpendingsubqueue.top();
							desc.readpendingsubqueue.pop();
							t.absid = desc.readpendingsubqueuenextid++;
							t.deblock = deblock;
							readyList.push_back(t);
						}
					}

					for ( uint64_t i = 0; i < readyList.size(); ++i )
					{
						FastqToBamControlSubReadPending t = readyList[i];
						FastqParsePackage * package = parseWorkPackages.getPackage();
						*package = FastqParsePackage(0/*prio*/,FPPDid,t);
						STP.enque(package);
					}
				}

				void checkReadPendingQueue()
				{
					libmaus2::parallel::ScopePosixSpinLock slock(desc.readpendingqueuelock);

					while (
						desc.readpendingqueue.size()
						&&
						desc.readpendingqueuenext == desc.readpendingqueue.top()->meta.blockid
					)
					{
						FastQInputDescBase::input_block_type::shared_ptr_type block = desc.readpendingqueue.top();
						desc.readpendingqueue.pop();

						{
							libmaus2::parallel::ScopePosixSpinLock llock(desc.readpendingsubqueuelock);
							for ( uint64_t i = 0; i < block->meta.blocks.size(); ++i )
								desc.readpendingsubqueue.push(
									FastqToBamControlSubReadPending(block,i)
								);
						}


						desc.readpendingqueuenext += 1;
					}

					checkSubReadPendingQueue();
				}

				void fastqInputPackageAddPending(FastQInputDescBase::input_block_type::shared_ptr_type block)
				{
					assert ( block->meta.blocks.size() <= 1 );

					if ( block->meta.blocks.size() )
					{
						std::pair<uint8_t *,uint8_t *> PP(block->meta.blocks[0]);

						if ( PP.second != PP.first )
						{
							assert ( PP.second[-1] == '\n' );

							std::vector<uint8_t *> PV;
							ptrdiff_t const d = (PP.second-PP.first) / STP.getNumThreads();

							// try to produce work packages
							for ( uint64_t i = 0; i < STP.getNumThreads(); ++i )
							{
								// set start pointer for scanning
								uint8_t * p = PP.first + i*d;

								// search next newline if this is not the start of the buffer
								if ( p != PP.first )
								{
									while ( *p != '\n' )
										++p;
									assert ( *p == '\n' );
									++p;
								}

								// if start pointer is not the end of the buffer
								uint8_t * ls[4] = {0,0,0,0};
								if ( p != PP.second )
								{
									while ( p != PP.second )
									{
										ls[0] = ls[1];
										ls[1] = ls[2];
										ls[2] = ls[3];
										ls[3] = p;

										if ( ls[0] && ls[0][0] == '@' && ls[2][0] == '+' )
											break;

										while ( *p != '\n' )
											++p;
										assert ( *p == '\n' );
										p += 1;
									};

								}

								if ( ls[0] && ls[0][0] == '@' && ls[2][0] == '+' )
									PV.push_back(ls[0]);
							}

							PV.push_back(PP.second);
							std::vector < std::pair<uint8_t *,uint8_t *> > PPV;
							for ( uint64_t i = 1; i < PV.size(); ++i )
							{
								std::pair<uint8_t *,uint8_t *> P(PV[i-1],PV[i]);
								if ( P.second != P.first )
									PPV.push_back(P);
							}

							if ( PPV.size() )
							{
								block->meta.blocks.resize(0);
								for ( uint64_t i = 0; i < PPV.size(); ++i )
									block->meta.blocks.push_back(PPV[i]);
							}
						}

					}

					{
						libmaus2::parallel::ScopePosixSpinLock slock(desc.readpendingqueuelock);
						desc.readpendingqueue.push(block);
					}

					desc.incrementBlocksPassed();

					if ( desc.getEOF() && (desc.getBlocksProduced() == desc.getBlocksPassed()) )
					{
						readingFinishedLock.lock();
						readingFinished = true;
						readingFinishedLock.unlock();
					}

					checkReadPendingQueue();
				}
				bool getReadingFinished()
				{
					bool finished;

					readingFinishedLock.lock();
					finished = readingFinished;
					readingFinishedLock.unlock();

					return finished;
				}

				void waitReadingFinished()
				{
					while (
						!getReadingFinished()
						&&
						!STP.isInPanicMode() )
						sleep(1);

					if ( STP.isInPanicMode() )
						STP.join();
				}

				bool getParsingFinished()
				{
					bool finished = false;
					parsingFinishedLock.lock();
					finished = parsingFinished;
					parsingFinishedLock.unlock();
					return finished;
				}

				void waitParsingFinished()
				{
					while (
						!getParsingFinished()
						&&
						!STP.isInPanicMode()
					)
						sleep(1);

					if ( STP.isInPanicMode() )
						STP.join();
				}

				bool getCompressionFinished()
				{
					bool finished;
					compressionfinishedlock.lock();
					finished = compressionfinished;
					compressionfinishedlock.unlock();
					return finished;
				}

				void waitCompressionFinished()
				{
					while (
						!getCompressionFinished()
						&&
						!STP.isInPanicMode()
					)
						sleep(1);

					if ( STP.isInPanicMode() )
						STP.join();
				}
			};
		}
	}
}
#endif
