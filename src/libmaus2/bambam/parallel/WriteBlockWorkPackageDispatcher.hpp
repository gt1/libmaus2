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
#if ! defined(LIBMAUS2_BAMBAM_PARALLEL_WRITEBLOCKWORKPACKAGEDISPATCHER_HPP)
#define LIBMAUS2_BAMBAM_PARALLEL_WRITEBLOCKWORKPACKAGEDISPATCHER_HPP

#include <libmaus2/parallel/SimpleThreadWorkPackageDispatcher.hpp>
#include <libmaus2/parallel/SimpleThreadPoolInterfaceEnqueTermInterface.hpp>
#include <libmaus2/bambam/parallel/WriteBlockWorkPackageReturnInterface.hpp>
#include <libmaus2/bambam/parallel/ReturnBgzfOutputBufferInterface.hpp>
#include <libmaus2/bambam/parallel/BgzfOutputBlockWrittenInterface.hpp>

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{
			// dispatcher for block writing
			struct WriteBlockWorkPackageDispatcher : public libmaus2::parallel::SimpleThreadWorkPackageDispatcher
			{
				WriteBlockWorkPackageReturnInterface & packageReturnInterface;
				ReturnBgzfOutputBufferInterface & bufferReturnInterface;
				BgzfOutputBlockWrittenInterface & bgzfOutputBlockWrittenInterface;

				WriteBlockWorkPackageDispatcher(
					WriteBlockWorkPackageReturnInterface & rpackageReturnInterface,
					ReturnBgzfOutputBufferInterface & rbufferReturnInterface,
					BgzfOutputBlockWrittenInterface & rbgzfOutputBlockWrittenInterface
				) : packageReturnInterface(rpackageReturnInterface), bufferReturnInterface(rbufferReturnInterface),
				    bgzfOutputBlockWrittenInterface(rbgzfOutputBlockWrittenInterface)
				{

				}

				virtual void dispatch(
					libmaus2::parallel::SimpleThreadWorkPackage * P,
					libmaus2::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & /* tpi */
				)
				{
					WriteBlockWorkPackage * BP = dynamic_cast<WriteBlockWorkPackage *>(P);
					assert ( BP );

					std::ostream & out = *(BP->obj.out);
					libmaus2::lz::BgzfDeflateOutputBufferBase::shared_ptr_type obuf = BP->obj.obuf;
					libmaus2::lz::BgzfDeflateZStreamBaseFlushInfo const & flushinfo = BP->obj.flushinfo;
					char const * outp = reinterpret_cast<char const *>(obuf->outbuf.begin());
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

					out.write(outp, n);

					if ( ! out )
					{
						// check for output errors
					}

					// return output buffer
					bufferReturnInterface.returnBgzfOutputBufferInterface(obuf);
					//
					bgzfOutputBlockWrittenInterface.bgzfOutputBlockWritten(BP->obj.streamid,BP->obj.blockid,BP->obj.subid,n);
					// return work package
					packageReturnInterface.returnWriteBlockWorkPackage(BP);
				}
			};
		}
	}
}
#endif
