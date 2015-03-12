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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_FILECHECKSUMBLOCKWORKPACKAGEDISPATCHER_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_FILECHECKSUMBLOCKWORKPACKAGEDISPATCHER_HPP

#include <libmaus/bambam/parallel/FileChecksumBlockWorkPackageReturnInterface.hpp>
#include <libmaus/bambam/parallel/FileChecksumBlockFinishedInterface.hpp>			
#include <libmaus/parallel/SimpleThreadWorkPackageDispatcher.hpp>			

namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{
			template<typename _file_checksum_type>
			struct FileChecksumBlockWorkPackageDispatcher : libmaus::parallel::SimpleThreadWorkPackageDispatcher
			{
				typedef _file_checksum_type file_checksum_type;
				typedef FileChecksumBlockWorkPackageDispatcher<file_checksum_type> this_type;
				typedef typename libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef typename libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

				FileChecksumBlockWorkPackageReturnInterface<file_checksum_type> & packageReturnInterface;
				FileChecksumBlockFinishedInterface & packageFinishedInterface;
	
				FileChecksumBlockWorkPackageDispatcher(
					FileChecksumBlockWorkPackageReturnInterface<file_checksum_type> & rpackageReturnInterface,
					FileChecksumBlockFinishedInterface & rpackageFinishedInterface
				) : packageReturnInterface(rpackageReturnInterface), packageFinishedInterface(rpackageFinishedInterface)
				{
				
				}
				void dispatch(libmaus::parallel::SimpleThreadWorkPackage * P, libmaus::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & /* tpi */)
				{
					FileChecksumBlockWorkPackage<file_checksum_type> * BP = dynamic_cast< FileChecksumBlockWorkPackage<file_checksum_type> * >(P);
					
					libmaus::bambam::parallel::GenericInputControlCompressionPending & GICCP = BP->GICCP;
					libmaus::lz::BgzfDeflateOutputBufferBase::shared_ptr_type & outblock = GICCP.outblock;
					libmaus::lz::BgzfDeflateZStreamBaseFlushInfo const & flushinfo = GICCP.flushinfo;
					
					if ( flushinfo.blocks == 1 )
						BP->checksum->update(outblock->outbuf.begin(),flushinfo.block_a_c);
					else if ( flushinfo.blocks == 2 )
						BP->checksum->update(outblock->outbuf.begin(),flushinfo.block_a_c + flushinfo.block_b_c);
					
					packageFinishedInterface.fileChecksumBlockFinished(BP->GICCP);
					packageReturnInterface.fileChecksumBlockWorkPackageReturn(BP);
				}
			};
		}
	}
}
#endif
