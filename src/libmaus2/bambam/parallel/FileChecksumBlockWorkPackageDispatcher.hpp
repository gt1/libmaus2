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
#if ! defined(LIBMAUS2_BAMBAM_PARALLEL_FILECHECKSUMBLOCKWORKPACKAGEDISPATCHER_HPP)
#define LIBMAUS2_BAMBAM_PARALLEL_FILECHECKSUMBLOCKWORKPACKAGEDISPATCHER_HPP

#include <libmaus2/bambam/parallel/FileChecksumBlockWorkPackageReturnInterface.hpp>
#include <libmaus2/bambam/parallel/FileChecksumBlockFinishedInterface.hpp>			
#include <libmaus2/parallel/SimpleThreadWorkPackageDispatcher.hpp>			

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{
			struct FileChecksumBlockWorkPackageDispatcher : libmaus2::parallel::SimpleThreadWorkPackageDispatcher
			{
				typedef FileChecksumBlockWorkPackageDispatcher this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				FileChecksumBlockWorkPackageReturnInterface & packageReturnInterface;
				FileChecksumBlockFinishedInterface & packageFinishedInterface;
	
				FileChecksumBlockWorkPackageDispatcher(
					FileChecksumBlockWorkPackageReturnInterface & rpackageReturnInterface,
					FileChecksumBlockFinishedInterface & rpackageFinishedInterface
				) : packageReturnInterface(rpackageReturnInterface), packageFinishedInterface(rpackageFinishedInterface)
				{
				
				}
				void dispatch(libmaus2::parallel::SimpleThreadWorkPackage * P, libmaus2::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & /* tpi */)
				{
					FileChecksumBlockWorkPackage * BP = dynamic_cast< FileChecksumBlockWorkPackage * >(P);
					
					libmaus2::bambam::parallel::GenericInputControlCompressionPending & GICCP = BP->GICCP;
					libmaus2::lz::BgzfDeflateOutputBufferBase::shared_ptr_type & outblock = GICCP.outblock;
					libmaus2::lz::BgzfDeflateZStreamBaseFlushInfo const & flushinfo = GICCP.flushinfo;
					
					if ( flushinfo.blocks == 1 )
						BP->checksum->vupdate(outblock->outbuf.begin(),flushinfo.block_a_c);
					else if ( flushinfo.blocks == 2 )
						BP->checksum->vupdate(outblock->outbuf.begin(),flushinfo.block_a_c + flushinfo.block_b_c);
					
					packageFinishedInterface.fileChecksumBlockFinished(BP->GICCP);
					packageReturnInterface.fileChecksumBlockWorkPackageReturn(BP);
				}
			};
		}
	}
}
#endif
