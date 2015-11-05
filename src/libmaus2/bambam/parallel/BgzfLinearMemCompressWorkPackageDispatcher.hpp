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
#if ! defined(LIBMAUS2_BAMBAM_PARALLEL_BGZFLINEARMEMCOMPRESSWORKPACKAGEDISPATCHER_HPP)
#define LIBMAUS2_BAMBAM_PARALLEL_BGZFLINEARMEMCOMPRESSWORKPACKAGEDISPATCHER_HPP

#include <libmaus2/parallel/SimpleThreadWorkPackageDispatcher.hpp>
#include <libmaus2/bambam/parallel/BgzfLinearMemCompressWorkPackageReturnInterface.hpp>
#include <libmaus2/bambam/parallel/GetBgzfDeflateZStreamBaseInterface.hpp>
#include <libmaus2/bambam/parallel/PutBgzfDeflateZStreamBaseInterface.hpp>
#include <libmaus2/bambam/parallel/SmallLinearBlockCompressionPendingObjectFinishedInterface.hpp>
#include <libmaus2/bambam/parallel/AddWritePendingBgzfBlockInterface.hpp>

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{
			// dispatcher for block compression
			struct BgzfLinearMemCompressWorkPackageDispatcher : public libmaus2::parallel::SimpleThreadWorkPackageDispatcher
			{
				BgzfLinearMemCompressWorkPackageReturnInterface & returnPackageObject;
				GetBgzfDeflateZStreamBaseInterface & getBgzfDeflateZStreamBaseObject;
				PutBgzfDeflateZStreamBaseInterface & putBgzfDeflateZStreamBaseObject;
				SmallLinearBlockCompressionPendingObjectFinishedInterface & smallLinearBlockCompressionPendingObjectFinishedObject;
				AddWritePendingBgzfBlockInterface & addWritePendingBgzfBlockObject;

				BgzfLinearMemCompressWorkPackageDispatcher(
					BgzfLinearMemCompressWorkPackageReturnInterface & rreturnPackageObject,
					GetBgzfDeflateZStreamBaseInterface & rgetBgzfDeflateZStreamBaseObject,
					PutBgzfDeflateZStreamBaseInterface & rputBgzfDeflateZStreamBaseObject,
					SmallLinearBlockCompressionPendingObjectFinishedInterface & rsmallLinearBlockCompressionPendingObjectFinishedObject,
					AddWritePendingBgzfBlockInterface & raddWritePendingBgzfBlockObject
				) :
				    returnPackageObject(rreturnPackageObject),
				    getBgzfDeflateZStreamBaseObject(rgetBgzfDeflateZStreamBaseObject),
				    putBgzfDeflateZStreamBaseObject(rputBgzfDeflateZStreamBaseObject),
				    smallLinearBlockCompressionPendingObjectFinishedObject(rsmallLinearBlockCompressionPendingObjectFinishedObject),
				    addWritePendingBgzfBlockObject(raddWritePendingBgzfBlockObject)
				{

				}


				virtual void dispatch(
					libmaus2::parallel::SimpleThreadWorkPackage * P,
					libmaus2::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & /* tpi */
				)
				{
					BgzfLinearMemCompressWorkPackage * BP = dynamic_cast<BgzfLinearMemCompressWorkPackage *>(P);
					assert ( BP );

					libmaus2::lz::BgzfDeflateOutputBufferBase::shared_ptr_type outbuf = BP->outbuf;
					libmaus2::lz::BgzfDeflateZStreamBase::shared_ptr_type compressor = getBgzfDeflateZStreamBaseObject.getBgzfDeflateZStreamBase();

					libmaus2::lz::BgzfDeflateZStreamBaseFlushInfo const flushinfo = compressor->flush(BP->obj.pa,BP->obj.pe,*outbuf);

					#if 0
					tpi.getGlobalLock().lock();
					std::cerr << "compressed " << BP->obj.blockid << "/" << BP->obj.subid << std::endl;
					tpi.getGlobalLock().unlock();
					#endif

					// return compressor
					putBgzfDeflateZStreamBaseObject.putBgzfDeflateZStreamBase(compressor);

					// return pending object
					smallLinearBlockCompressionPendingObjectFinishedObject.smallLinearBlockCompressionPendingObjectFinished(BP->obj);
					// mark block as pending
					addWritePendingBgzfBlockObject.addWritePendingBgzfBlock(BP->obj.blockid,BP->obj.subid,outbuf,flushinfo);

					// return work package
					returnPackageObject.returnBgzfLinearMemCompressWorkPackage(BP);
				}
			};
		}
	}
}
#endif
