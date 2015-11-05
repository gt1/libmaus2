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
#if ! defined(LIBMAUS2_BAMBAM_PARALLEL_BLOCKCOMPRESSPACKAGEDISPATCHER_HPP)
#define LIBMAUS2_BAMBAM_PARALLEL_BLOCKCOMPRESSPACKAGEDISPATCHER_HPP

#include <libmaus2/bambam/parallel/AddPendingCompressBufferWriteInterface.hpp>
#include <libmaus2/bambam/parallel/ReturnCompressionPendingElementInterface.hpp>
#include <libmaus2/bambam/parallel/AlignmentBlockCompressPackageReturnInterface.hpp>
#include <libmaus2/parallel/SimpleThreadWorkPackageDispatcher.hpp>
#include <libmaus2/parallel/LockedGrowingFreeList.hpp>
#include <libmaus2/lz/CompressorObject.hpp>
#include <libmaus2/lz/CompressorObjectFreeListAllocator.hpp>

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{
			// dispatcher for block compression
			struct BlockCompressPackageDispatcher : public libmaus2::parallel::SimpleThreadWorkPackageDispatcher
			{
				libmaus2::parallel::LockedGrowingFreeList<libmaus2::lz::CompressorObject, libmaus2::lz::CompressorObjectFreeListAllocator> & compfreelist;
				AddPendingCompressBufferWriteInterface & addPendingWriteInterface;
				ReturnCompressionPendingElementInterface & returnCompressionPendingElementInterface;
				AlignmentBlockCompressPackageReturnInterface & packageReturnInterface;

				BlockCompressPackageDispatcher(
					libmaus2::parallel::LockedGrowingFreeList<libmaus2::lz::CompressorObject, libmaus2::lz::CompressorObjectFreeListAllocator> & rcompfreelist,
					AddPendingCompressBufferWriteInterface & raddPendingWriteInterface,
					ReturnCompressionPendingElementInterface & rreturnCompressionPendingElementInterface,
					AlignmentBlockCompressPackageReturnInterface & rpackageReturnInterface
				) : compfreelist(rcompfreelist),
				    addPendingWriteInterface(raddPendingWriteInterface),
				    returnCompressionPendingElementInterface(rreturnCompressionPendingElementInterface),
				    packageReturnInterface(rpackageReturnInterface)
				{
				}

				virtual void dispatch(
					libmaus2::parallel::SimpleThreadWorkPackage * P,
					libmaus2::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & tpi
				)
				{
					AlignmentBlockCompressPackage * BP = dynamic_cast<AlignmentBlockCompressPackage *>(P);
					assert ( BP );

					CompressionPendingElement * pend = BP->pend;
					CompressBuffer * compbuf = BP->compbuf;

					compbuf->blockid = pend->blockid;
					compbuf->subid = pend->subid;
					compbuf->totalsubids = pend->totalsubids;

					AlignmentRewriteBuffer * rewritebuffer = pend->buffer;
					uint64_t const low = rewritebuffer->blocksizes[pend->subid];
					uint64_t const high = rewritebuffer->blocksizes[pend->subid+1];

					uint8_t * compin = compbuf->inputBuffer.begin();

					for ( uint64_t i = low; i < high; ++i )
					{
						std::pair<uint8_t const *,uint64_t> P = rewritebuffer->at(i);
						assert ( ( (compin + P.second + sizeof(uint32_t)) - compbuf->inputBuffer.begin()) <= compbuf->inputBuffer.size() );

						// length as little endian
						*(compin++) = (P.second >> 0) & 0xFF;
						*(compin++) = (P.second >> 8) & 0xFF;
						*(compin++) = (P.second >> 16) & 0xFF;
						*(compin++) = (P.second >> 24) & 0xFF;

						// copy alignment data block
						memcpy(compin,P.first,P.second);
						compin += P.second;
					}

					uint64_t const insize = compin - compbuf->inputBuffer.begin();

					libmaus2::lz::CompressorObject * compressor = compfreelist.get();

					try
					{
						compbuf->compsize = compressor->compress(
							reinterpret_cast<char const *>(compbuf->inputBuffer.begin()),
							insize,
							compbuf->outputBuffer
						);
					}
					catch(...)
					{
						compfreelist.put(compressor);
						throw;
					}

					compfreelist.put(compressor);

					// return pending object
					returnCompressionPendingElementInterface.putReturnCompressionPendingElement(pend);

					// mark block as pending for writing
					addPendingWriteInterface.putAddPendingCompressBufferWrite(compbuf);

					// return the work package
					packageReturnInterface.putAlignmentBlockCompressPackagePackage(BP);
				}
			};
		}
	}
}
#endif
