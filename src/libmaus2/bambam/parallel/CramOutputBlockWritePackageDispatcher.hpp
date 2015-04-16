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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_CRAMOUTPUTBLOCKWRITEPACKAGEDISPATCHER_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_CRAMOUTPUTBLOCKWRITEPACKAGEDISPATCHER_HPP

#include <libmaus2/bambam/parallel/CramOutputBlockWritePackageFinishedInterface.hpp>
#include <libmaus2/bambam/parallel/CramOutputBlockWritePackageReturnInterface.hpp>
#include <libmaus2/parallel/SimpleThreadWorkPackageDispatcher.hpp>

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{

			struct CramOutputBlockWritePackageDispatcher : public libmaus2::parallel::SimpleThreadWorkPackageDispatcher
			{
				typedef CramOutputBlockWritePackageDispatcher this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
				
				CramOutputBlockWritePackageReturnInterface & packageReturnInterface;
				CramOutputBlockWritePackageFinishedInterface & finishedInterface;
						
				CramOutputBlockWritePackageDispatcher(
					CramOutputBlockWritePackageReturnInterface & rpackageReturnInterface,
					CramOutputBlockWritePackageFinishedInterface & rfinishedInterface
				) : packageReturnInterface(rpackageReturnInterface), finishedInterface(rfinishedInterface) {}
				~CramOutputBlockWritePackageDispatcher() {}
				void dispatch(libmaus2::parallel::SimpleThreadWorkPackage * P, libmaus2::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & /* tpi */)
				{
					CramOutputBlockWritePackage * BP = dynamic_cast<CramOutputBlockWritePackage *>(P);

					CramOutputBlock::shared_ptr_type block = BP->block;
					size_t const n = block->fill;
					char const * data = n ? block->A->begin() : NULL;
					std::ostream * out = BP->out;
					
					out->write(data,n);
					if ( ! out )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "CramOutputBlockWritePackageDispatcher::dispatch: failed to write " << n << " bytes of data." << std::endl;
						lme.finish();
						throw lme;
					}

					finishedInterface.cramOutputBlockWritePackageFinished(block);
					packageReturnInterface.cramOutputBlockWritePackageReturn(BP);
				}
			};
		}
	}
}
#endif
