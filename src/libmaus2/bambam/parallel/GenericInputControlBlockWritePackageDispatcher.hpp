/*
    libmaus
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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_GENERICINPUTCONTROLBLOCKWRITEPACKAGEDISPATCHER_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_GENERICINPUTCONTROLBLOCKWRITEPACKAGEDISPATCHER_HPP

#include <libmaus/bambam/parallel/GenericInputControlBlockWritePackageBlockWrittenInterface.hpp>
#include <libmaus/bambam/parallel/GenericInputControlBlockWritePackageReturnInterface.hpp>
#include <libmaus/bambam/parallel/GenericInputControlBlockWritePackage.hpp>
#include <libmaus/parallel/SimpleThreadWorkPackageDispatcher.hpp>

namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{
			struct GenericInputControlBlockWritePackageDispatcher : public libmaus::parallel::SimpleThreadWorkPackageDispatcher
			{
				typedef GenericInputControlBlockWritePackageDispatcher this_type;
				typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

				GenericInputControlBlockWritePackageReturnInterface & packageReturnInterface;
				GenericInputControlBlockWritePackageBlockWrittenInterface & blockWrittenInterface;
				
				GenericInputControlBlockWritePackageDispatcher(				
					GenericInputControlBlockWritePackageReturnInterface & rpackageReturnInterface,
					GenericInputControlBlockWritePackageBlockWrittenInterface & rblockWrittenInterface
				) : packageReturnInterface(rpackageReturnInterface), blockWrittenInterface(rblockWrittenInterface) {}
				
				void dispatch(libmaus::parallel::SimpleThreadWorkPackage * P, libmaus::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & /* tpi */)
				{
					assert ( dynamic_cast<GenericInputControlBlockWritePackage *>(P) != 0 );
					GenericInputControlBlockWritePackage * BP = dynamic_cast<GenericInputControlBlockWritePackage *>(P);
					
					libmaus::bambam::parallel::GenericInputControlCompressionPending & G = BP->GICCP;

					libmaus::lz::BgzfDeflateOutputBufferBase::shared_ptr_type outblock = G.outblock;
					libmaus::lz::BgzfDeflateZStreamBaseFlushInfo flushinfo = G.flushinfo;

					char const * outp = reinterpret_cast<char const *>(outblock->outbuf.begin());
					uint64_t n = 0;

					assert ( flushinfo.blocks == 1 || flushinfo.blocks == 2 );
						
					if ( flushinfo.blocks == 1 )
					{
						/* write data to stream, one block */
						n = flushinfo.block_a_c;
					}
					else
					{
						assert ( flushinfo.blocks == 2 );
						/* write data to stream, two blocks */
						n = flushinfo.block_a_c + flushinfo.block_b_c;
					}

					BP->out->write(outp, n);                                
				
					blockWrittenInterface.genericInputControlBlockWritePackageBlockWritten(BP->GICCP);
					packageReturnInterface.genericInputControlBlockWritePackageReturn(BP);
				}
			};
		}
	}
}
#endif
