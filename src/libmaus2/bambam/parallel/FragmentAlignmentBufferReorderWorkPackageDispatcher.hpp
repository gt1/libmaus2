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
#if ! defined(LIBMAUS2_BAMBAM_PARALLEL_FRAGMENTALIGNMENTBUFFERREORDERWORKPACKAGEDISPATCHER_HPP)
#define LIBMAUS2_BAMBAM_PARALLEL_FRAGMENTALIGNMENTBUFFERREORDERWORKPACKAGEDISPATCHER_HPP

#include <libmaus2/parallel/SimpleThreadWorkPackageDispatcher.hpp>
#include <libmaus2/bambam/parallel/FragmentAlignmentBufferReorderWorkPackage.hpp>
#include <libmaus2/bambam/parallel/FragmentAlignmentBufferReorderWorkPackageReturnInterface.hpp>
#include <libmaus2/bambam/parallel/FragmentAlignmentBufferReorderWorkPackageFinishedInterface.hpp>

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{
			struct FragmentAlignmentBufferReorderWorkPackageDispatcher : public libmaus2::parallel::SimpleThreadWorkPackageDispatcher
			{
				typedef FragmentAlignmentBufferReorderWorkPackageDispatcher this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				libmaus2::bambam::parallel::FragmentAlignmentBufferReorderWorkPackageReturnInterface & workPackageReturnInterface;
				libmaus2::bambam::parallel::FragmentAlignmentBufferReorderWorkPackageFinishedInterface & workPackageFinishedInterface;

				FragmentAlignmentBufferReorderWorkPackageDispatcher(
					libmaus2::bambam::parallel::FragmentAlignmentBufferReorderWorkPackageReturnInterface & rworkPackageReturnInterface,
					libmaus2::bambam::parallel::FragmentAlignmentBufferReorderWorkPackageFinishedInterface & rworkPackageFinishedInterface
				) : workPackageReturnInterface(rworkPackageReturnInterface), workPackageFinishedInterface(rworkPackageFinishedInterface)
				{

				}

				void dispatch(
					libmaus2::parallel::SimpleThreadWorkPackage * P,
					libmaus2::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & /* tpi */)
				{
					libmaus2::bambam::parallel::FragmentAlignmentBufferReorderWorkPackage * BP =
						dynamic_cast<libmaus2::bambam::parallel::FragmentAlignmentBufferReorderWorkPackage *>(P);

					BP->copyReq.dispatch();

					workPackageFinishedInterface.fragmentAlignmentBufferReorderWorkPackageFinished(BP);
					workPackageReturnInterface.returnFragmentAlignmentBufferReorderWorkPackage(BP);
				}
			};
		}
	}
}
#endif
