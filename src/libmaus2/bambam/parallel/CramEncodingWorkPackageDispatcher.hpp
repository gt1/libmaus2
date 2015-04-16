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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_CRAMENCODINGWORKPACKAGEDISPATCHER_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_CRAMENCODINGWORKPACKAGEDISPATCHER_HPP

#include <libmaus2/parallel/SimpleThreadWorkPackageDispatcher.hpp>
#include <libmaus2/bambam/parallel/CramEncodingWorkPackageReturnInterface.hpp>

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{
			struct CramEncodingWorkPackageDispatcher : public libmaus2::parallel::SimpleThreadWorkPackageDispatcher
			{
				typedef CramEncodingWorkPackageDispatcher this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
				
				CramEncodingWorkPackageReturnInterface & packageReturnInterface;
						
				CramEncodingWorkPackageDispatcher(
					CramEncodingWorkPackageReturnInterface & rpackageReturnInterface
				) : packageReturnInterface(rpackageReturnInterface) {}
				~CramEncodingWorkPackageDispatcher() {}
				void dispatch(libmaus2::parallel::SimpleThreadWorkPackage * P, libmaus2::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & /* tpi */)
				{
					CramEncodingWorkPackage * BP = dynamic_cast<CramEncodingWorkPackage *>(P);
					BP->dispatch();
					packageReturnInterface.cramEncodingWorkPackageReturn(BP);
				}
			};
		}
	}
}
#endif
