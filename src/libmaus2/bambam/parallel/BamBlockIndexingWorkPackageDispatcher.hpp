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
#if ! defined(LIBMAUS2_BAMBAM_PARALLEL_BAMBLOCKINDEXINGWORKPACKAGEDISPATCHER_HPP)
#define LIBMAUS2_BAMBAM_PARALLEL_BAMBLOCKINDEXINGWORKPACKAGEDISPATCHER_HPP

#include <libmaus2/bambam/parallel/BamBlockIndexingBlockFinishedInterface.hpp>
#include <libmaus2/bambam/parallel/BamBlockIndexingWorkPackageReturnInterface.hpp>
#include <libmaus2/parallel/SimpleThreadWorkPackageDispatcher.hpp>

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{						
			struct BamBlockIndexingWorkPackageDispatcher : public libmaus2::parallel::SimpleThreadWorkPackageDispatcher
			{
				typedef BamBlockIndexingWorkPackageDispatcher this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				BamBlockIndexingWorkPackageReturnInterface & returnInterface;
				BamBlockIndexingBlockFinishedInterface & finishedInterface;
			
				BamBlockIndexingWorkPackageDispatcher(
					BamBlockIndexingWorkPackageReturnInterface & rreturnInterface,
					BamBlockIndexingBlockFinishedInterface & rfinishedInterface
				) : returnInterface(rreturnInterface), finishedInterface(rfinishedInterface)
				{}
				
				void dispatch(libmaus2::parallel::SimpleThreadWorkPackage * P, libmaus2::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & /* tpi */)
				{
					BamBlockIndexingWorkPackage * BP = dynamic_cast<BamBlockIndexingWorkPackage *>(P);

					if ( BP->bamindexgenerator )
					{					
						libmaus2::bambam::parallel::GenericInputControlCompressionPending & GICCP = BP->GICCP;
						libmaus2::bambam::BamIndexGenerator & indexer = *(BP->bamindexgenerator);
						libmaus2::lz::BgzfDeflateZStreamBaseFlushInfo const & flushinfo = GICCP.flushinfo;
						std::pair<uint8_t *,uint8_t *> const & input = GICCP.P; 
						
						if ( flushinfo.blocks == 1 )
						{
							indexer.addBlock(input.first,flushinfo.block_a_c,flushinfo.block_a_u);
						}
						else if ( flushinfo.blocks == 2 )
						{
							indexer.addBlock(input.first,                      flushinfo.block_a_c,flushinfo.block_a_u);
							indexer.addBlock(input.first + flushinfo.block_a_u,flushinfo.block_b_c,flushinfo.block_b_u);
						}
					}
					
					finishedInterface.bamBlockIndexingBlockFinished(BP->GICCP);
					returnInterface.bamBlockIndexingWorkPackageReturn(BP);
				}
			};		
		}
	}
}
#endif
