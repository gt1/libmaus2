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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_GENERICINPUTCONTROLBLOCKCOMPRESSIONWORKPACKAGEDISPATCHER_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_GENERICINPUTCONTROLBLOCKCOMPRESSIONWORKPACKAGEDISPATCHER_HPP

#include <libmaus2/bambam/parallel/GenericInputControlPutCompressorInterface.hpp>
#include <libmaus2/bambam/parallel/GenericInputControlGetCompressorInterface.hpp>
#include <libmaus2/bambam/parallel/GenericInputControlBlockCompressionWorkPackageReturnInterface.hpp>
#include <libmaus2/bambam/parallel/GenericInputControlBlockCompressionWorkPackage.hpp>
#include <libmaus2/bambam/parallel/GenericInputControlBlockCompressionFinishedInterface.hpp>
#include <libmaus2/parallel/SimpleThreadWorkPackageDispatcher.hpp>

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{
			struct GenericInputControlBlockCompressionWorkPackageDispatcher : public libmaus2::parallel::SimpleThreadWorkPackageDispatcher
			{
				typedef GenericInputControlBlockCompressionWorkPackageDispatcher this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				GenericInputControlBlockCompressionWorkPackageReturnInterface & packageReturnInterface;
				GenericInputControlBlockCompressionFinishedInterface & blockCompressedInterface;
				GenericInputControlGetCompressorInterface & getCompressorInterface;
				GenericInputControlPutCompressorInterface & putCompressorInterface;
			
				GenericInputControlBlockCompressionWorkPackageDispatcher(
					GenericInputControlBlockCompressionWorkPackageReturnInterface & rpackageReturnInterface,
					GenericInputControlBlockCompressionFinishedInterface & rblockCompressedInterface,
					libmaus2::bambam::parallel::GenericInputControlGetCompressorInterface & rgetCompressorInterface,
					libmaus2::bambam::parallel::GenericInputControlPutCompressorInterface & rputCompressorInterface
				)
				: packageReturnInterface(rpackageReturnInterface), blockCompressedInterface(rblockCompressedInterface),
				  getCompressorInterface(rgetCompressorInterface), putCompressorInterface(rputCompressorInterface)
				{
				}
			
				void dispatch(libmaus2::parallel::SimpleThreadWorkPackage * P, libmaus2::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & /* tpi */)
				{
					assert ( dynamic_cast<GenericInputControlBlockCompressionWorkPackage *>(P) != 0 );
					GenericInputControlBlockCompressionWorkPackage * BP = dynamic_cast<GenericInputControlBlockCompressionWorkPackage *>(P);
			
					GenericInputControlCompressionPending & GICCP = BP->GICCP;
					libmaus2::lz::BgzfDeflateZStreamBase::shared_ptr_type compressor = getCompressorInterface.genericInputControlGetCompressor();		
					libmaus2::lz::BgzfDeflateOutputBufferBase & outblock = *(GICCP.outblock);
					std::pair<uint8_t *,uint8_t *> R = GICCP.P;

					if ( R.first == R.second )
					{
						static uint8_t const emptybgzfblock[] = {
							0x1f, 0x8b, 0x08, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x06, 0x00, 0x42, 0x43, 
							0x02, 0x00, 0x1b, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
						};
						
						std::copy(
							&emptybgzfblock[0],
							&emptybgzfblock[0] + sizeof(emptybgzfblock)/sizeof(emptybgzfblock[0]),
							outblock.outbuf.begin()
						);
						
						GICCP.flushinfo = libmaus2::lz::BgzfDeflateZStreamBaseFlushInfo(0,sizeof(emptybgzfblock)/sizeof(emptybgzfblock[0]));
					}
					else
					{
						libmaus2::lz::BgzfDeflateZStreamBaseFlushInfo const info = compressor->flush(R.first,R.second,outblock);
						GICCP.flushinfo = info;
					}
										
					putCompressorInterface.genericInputControlPutCompressor(compressor);
					blockCompressedInterface.genericInputControlBlockCompressionFinished(BP->GICCP);
					packageReturnInterface.genericInputControlBlockCompressionWorkPackageReturn(BP);
				}
			};
		}
	}
}
#endif
