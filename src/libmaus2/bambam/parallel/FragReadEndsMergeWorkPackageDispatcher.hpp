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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_FRAGREADENDSMERGEWORKPACKAGEDISPATCHER_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_FRAGREADENDSMERGEWORKPACKAGEDISPATCHER_HPP

#include <libmaus/bambam/parallel/FragReadEndsMergeWorkPackageReturnInterface.hpp>
#include <libmaus/bambam/parallel/FragReadEndsMergeWorkPackageFinishedInterface.hpp>
#include <libmaus/bambam/parallel/AddDuplicationMetricsInterface.hpp>
#include <libmaus/parallel/SimpleThreadWorkPackageDispatcher.hpp>
#include <libmaus/bambam/ReadEndsBlockIndexSet.hpp>

namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{
			struct FragReadEndsMergeWorkPackageDispatcher : public libmaus::parallel::SimpleThreadWorkPackageDispatcher
			{
				typedef FragReadEndsMergeWorkPackageDispatcher this_type;
				typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
				
				FragReadEndsMergeWorkPackageReturnInterface & packageReturnInterface;
				FragReadEndsMergeWorkPackageFinishedInterface & mergeFinishedInterface;
				AddDuplicationMetricsInterface & addDuplicationMetricsInterface;
						
				FragReadEndsMergeWorkPackageDispatcher(
					FragReadEndsMergeWorkPackageReturnInterface & rpackageReturnInterface,
					FragReadEndsMergeWorkPackageFinishedInterface & rmergeFinishedInterface,
					AddDuplicationMetricsInterface & raddDuplicationMetricsInterface
				) : libmaus::parallel::SimpleThreadWorkPackageDispatcher(), 
				    packageReturnInterface(rpackageReturnInterface), mergeFinishedInterface(rmergeFinishedInterface),
				    addDuplicationMetricsInterface(raddDuplicationMetricsInterface)
				{
				
				}
				virtual ~FragReadEndsMergeWorkPackageDispatcher() {}
				virtual void dispatch(libmaus::parallel::SimpleThreadWorkPackage * P, libmaus::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & /* tpi */)
				{
					FragReadEndsMergeWorkPackage * BP = dynamic_cast<FragReadEndsMergeWorkPackage *>(P);
					assert ( BP );

					ReadEndsBlockIndexSet fragindexset(*(BP->REQ.MI));
					libmaus::bambam::DupSetCallbackSharedVector dvec(*(BP->REQ.dupbitvec));
							
					fragindexset.merge(
						BP->REQ.SMI,
						libmaus::bambam::DupMarkBase::isDupFrag,
						libmaus::bambam::DupMarkBase::markDuplicateFrags,dvec
					);
									
					addDuplicationMetricsInterface.addDuplicationMetrics(dvec.metrics);
					
					mergeFinishedInterface.fragReadEndsMergeWorkPackageFinished(BP);
					packageReturnInterface.fragReadEndsMergeWorkPackageReturn(BP);
				}
			};
		}
	}
}
#endif
