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
#include <libmaus/parallel/LockedFreeList.hpp>
#include <libmaus/parallel/LockedGrowingFreeList.hpp>
#include <libmaus/parallel/LockedQueue.hpp>
#include <libmaus/parallel/PosixMutex.hpp>
#include <libmaus/parallel/PosixSpinLock.hpp>
#include <libmaus/parallel/SimpleThreadPool.hpp>
#include <libmaus/parallel/SimpleThreadPoolWorkPackageFreeList.hpp>
#include <libmaus/parallel/SimpleThreadWorkPackage.hpp>
#include <libmaus/parallel/SimpleThreadWorkPackageDispatcher.hpp>
#include <libmaus/timing/RealTimeClock.hpp>
#include <libmaus/util/FreeList.hpp>
#include <libmaus/util/GetObject.hpp>
#include <libmaus/util/GrowingFreeList.hpp>

#include <stack>

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
			uint64_t payloadsize;
			//! compressed data
			libmaus::autoarray::AutoArray<char> C;
			//! size of decompressed data
			uint64_t uncompdatasize;
			//! true if this is the last block in the stream
			bool final;
			//! stream id
			uint64_t streamid;
			//! block id
			uint64_t blockid;
			
			BamParallelDecodingInputBlock() 
			: 
				inflateheaderbase(),
				payloadsize(0),
				C(libmaus::lz::BgzfConstants::getBgzfMaxBlockSize(),false), 
				uncompdatasize(0),
				final(false) ,
				streamid(0),
				blockid(0)
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
			bool eof;
			std::istream & istr;
			uint64_t streamid;
			uint64_t blockid;
			libmaus::parallel::LockedFreeList<input_block_type> inputBlockFreeList;

			BamParallelDecodingControlInputInfo(
				std::istream & ristr, 
				uint64_t rstreamid,
				uint64_t rfreelistsize
			) : eof(false), istr(ristr), streamid(rstreamid), blockid(0), inputBlockFreeList(rfreelistsize) {}
			
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
				BamParallelDecodingInputBlockWorkPackage * BP = dynamic_cast<BamParallelDecodingInputBlockWorkPackage *>(P);
				assert ( BP );
				
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

				returnInterface.putBamParallelDecodingInputBlockWorkPackage(BP);

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

				BP->outputblock->decompressBlock(
					BP->decoder,
					BP->inputblock
				);
				
				BP->outputblock->streamid = BP->inputblock->streamid;
				BP->outputblock->blockid = BP->inputblock->blockid;

				decoderReturnInterface.putBamParallelDecodingBgzfInflateZStreamBaseReturn(BP->decoder);
				inputBlockReturnInterface.putBamParallelDecodingInputBlockReturn(BP->inputblock);
				decompressedBlockPendingInterface.putBamParallelDecodingDecompressedBlockAddPending(BP->outputblock);
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
		
			libmaus::autoarray::AutoArray<uint8_t> A;
			
			uint8_t * pA;
			pointer_type * pP;
			
			uint64_t pointerdif;
			
			BamParallelDecodingAlignmentBuffer(uint64_t const buffersize, uint64_t const rpointerdif = 1)
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
			
			uint32_t decodeLength(uint64_t const off) const
			{
				uint8_t const * B = A.begin() + off;
				
				return
					(static_cast<uint32_t>(B[0]) << 0)  |
					(static_cast<uint32_t>(B[1]) << 8)  |
					(static_cast<uint32_t>(B[2]) << 16) |
					(static_cast<uint32_t>(B[3]) << 24) ;
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
		
		struct BamParallelDecodingParseInfo
		{
			typedef BamParallelDecodingParseInfo this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			BamParallelDecodingPushBackSpace BPDPBS;
			
			libmaus::bambam::BamHeaderParserState BHPS;
			bool headerComplete;
			libmaus::bambam::BamHeaderLowMem::unique_ptr_type Pheader;

			libmaus::autoarray::AutoArray<char> concatBuffer;
			unsigned int concatBufferFill;
			
			enum parser_state_type {
				parser_state_read_blocklength,
				parser_state_read_block
			};
			
			parser_state_type parser_state;
			unsigned int blocklengthread;
			uint32_t blocklen;
			
			BamParallelDecodingParseInfo()
			: BPDPBS(), BHPS(), headerComplete(false),
			  concatBuffer(), concatBufferFill(0), 
			  parser_state(parser_state_read_blocklength),
			  blocklengthread(0), blocklen(0)
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

		struct BamParallelDecodingControl :
			public BamParallelDecodingInputBlockWorkPackageReturnInterface,
			public BamParallelDecodingInputBlockAddPendingInterface,
			public BamParallelDecodingDecompressBlockWorkPackageReturnInterface,
			public BamParallelDecodingInputBlockReturnInterface,
			public BamParallelDecodingDecompressedBlockAddPendingInterface,
			public BamParallelDecodingBgzfInflateZStreamBaseReturnInterface
		{
			typedef BamParallelDecodingControl this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

			typedef libmaus::bambam::BamParallelDecodingDecompressedBlock decompressed_block_type;
			
			libmaus::parallel::SimpleThreadPool & STP;
			BamParallelDecodingInputBlockWorkPackageDispatcher readDispatcher;
			uint64_t readDispatcherId;
			
			BamParallelDecodingDecompressBlockWorkPackageDispatcher decompressDispatcher;
			uint64_t decompressDispatcherId;

			uint64_t numthreads;

			BamParallelDecodingControlInputInfo inputinfo;

			libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<BamParallelDecodingInputBlockWorkPackage> readWorkPackages;
			libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<BamParallelDecodingDecompressBlockWorkPackage> decompressWorkPackages;

			libmaus::parallel::LockedFreeList<libmaus::lz::BgzfInflateZStreamBase>::unique_ptr_type Pdecoders;
			libmaus::parallel::LockedFreeList<libmaus::lz::BgzfInflateZStreamBase> & decoders;
			
			libmaus::parallel::LockedFreeList<decompressed_block_type> decompressedBlockFreeList;
			
			libmaus::parallel::LockedQueue<BamParallelDecodingControlInputInfo::input_block_type *> decompressPending;

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

			// add decompressed block to pending list
			virtual void putBamParallelDecodingDecompressedBlockAddPending(BamParallelDecodingDecompressedBlock * block)
			{
				// add block to the free list
				decompressedBlockFreeList.put(block);
				// check whether we have sufficient resources to decompress next package
				checkDecompressPendingList();
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

			BamParallelDecodingControl(libmaus::parallel::SimpleThreadPool & rSTP, std::istream & ristr, uint64_t const rstreamid)
			: 
				STP(rSTP),
				readDispatcher(*this,*this),
				readDispatcherId(STP.getNextDispatcherId()),
				decompressDispatcher(*this,*this,*this,*this),
				decompressDispatcherId(STP.getNextDispatcherId()),
				numthreads(STP.getNumThreads()),
				inputinfo(ristr,rstreamid,numthreads),
				readWorkPackages(),
				Pdecoders(new libmaus::parallel::LockedFreeList<libmaus::lz::BgzfInflateZStreamBase>(numthreads)),
				decoders(*Pdecoders),
				decompressedBlockFreeList(numthreads)
			{
				STP.registerDispatcher(readDispatcherId,&readDispatcher);
				STP.registerDispatcher(decompressDispatcherId,&decompressDispatcher);
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
				BamParallelDecodingControl BPDC(STP,in,0/*streamid*/);
				BPDC.serialTestDecode(out);
			}
			
			void enqueReadPackage()
			{
				BamParallelDecodingInputBlockWorkPackage * package = readWorkPackages.getPackage();
				*package = BamParallelDecodingInputBlockWorkPackage(0 /* priority */, &inputinfo,readDispatcherId);
				STP.enque(package);
			}

			static void serialParallelDecode1(std::istream & in)
			{
				libmaus::parallel::SimpleThreadPool STP(8);
				BamParallelDecodingControl BPDC(STP,in,0/*streamid*/);
				BPDC.enqueReadPackage();
				
				while ( ! BPDC.inputinfo.getEOF() )
				{
					sleep(1);
				}
				
				STP.terminate();
			}
		};	
	}
}
#endif
