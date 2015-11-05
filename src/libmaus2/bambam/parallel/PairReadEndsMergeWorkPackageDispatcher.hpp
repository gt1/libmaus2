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
#if ! defined(LIBMAUS2_BAMBAM_PARALLEL_PAIRREADENDSMERGEWORKPACKAGEDISPATCHER_HPP)
#define LIBMAUS2_BAMBAM_PARALLEL_PAIRREADENDSMERGEWORKPACKAGEDISPATCHER_HPP

#include <libmaus2/bambam/parallel/PairReadEndsMergeWorkPackageReturnInterface.hpp>
#include <libmaus2/bambam/parallel/PairReadEndsMergeWorkPackageFinishedInterface.hpp>
#include <libmaus2/bambam/parallel/AddDuplicationMetricsInterface.hpp>
#include <libmaus2/bambam/ReadEndsBlockIndexSet.hpp>
#include <libmaus2/parallel/SimpleThreadWorkPackageDispatcher.hpp>

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{
			struct PairReadEndsMergeWorkPackageDispatcher : public libmaus2::parallel::SimpleThreadWorkPackageDispatcher
			{
				typedef PairReadEndsMergeWorkPackageDispatcher this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				PairReadEndsMergeWorkPackageReturnInterface & packageReturnInterface;
				PairReadEndsMergeWorkPackageFinishedInterface & mergeFinishedInterface;
				AddDuplicationMetricsInterface & addDuplicationMetricsInterface;

				PairReadEndsMergeWorkPackageDispatcher(
					PairReadEndsMergeWorkPackageReturnInterface & rpackageReturnInterface,
					PairReadEndsMergeWorkPackageFinishedInterface & rmergeFinishedInterface,
					AddDuplicationMetricsInterface & raddDuplicationMetricsInterface
				) : libmaus2::parallel::SimpleThreadWorkPackageDispatcher(),
				    packageReturnInterface(rpackageReturnInterface), mergeFinishedInterface(rmergeFinishedInterface),
				    addDuplicationMetricsInterface(raddDuplicationMetricsInterface)
				{

				}
				virtual ~PairReadEndsMergeWorkPackageDispatcher() {}
				virtual void dispatch(libmaus2::parallel::SimpleThreadWorkPackage * P, libmaus2::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & /* tpi */)
				{
					PairReadEndsMergeWorkPackage * BP = dynamic_cast<PairReadEndsMergeWorkPackage *>(P);
					assert ( BP );

					ReadEndsBlockIndexSet pairindexset(*(BP->REQ.MI));
					libmaus2::bambam::DupSetCallbackSharedVector dvec(*(BP->REQ.dupbitvec));

					pairindexset.merge(
						BP->REQ.SMI,
						libmaus2::bambam::DupMarkBase::isDupPair,
						libmaus2::bambam::DupMarkBase::markDuplicatePairs,
						dvec);

					addDuplicationMetricsInterface.addDuplicationMetrics(dvec.metrics);

					mergeFinishedInterface.pairReadEndsMergeWorkPackageFinished(BP);
					packageReturnInterface.pairReadEndsMergeWorkPackageReturn(BP);
				}
			};
		}
	}
}
#endif
