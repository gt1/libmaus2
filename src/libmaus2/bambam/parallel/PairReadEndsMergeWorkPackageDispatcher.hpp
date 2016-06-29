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
				int verbose;

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

				void setVerbose(int const rverbose)
				{
					verbose = rverbose;
				}

				virtual void dispatch(libmaus2::parallel::SimpleThreadWorkPackage * P, libmaus2::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & /* tpi */)
				{
					try
					{
						PairReadEndsMergeWorkPackage * BP = dynamic_cast<PairReadEndsMergeWorkPackage *>(P);
						assert ( BP );

						if ( verbose )
						{
							libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
							std::cerr << "PairReadEndsMergeWorkPackageDispatcher package " << P << " instantiating ReadEndsBlockIndexSet" << std::endl;
						}
						ReadEndsBlockIndexSet pairindexset(*(BP->REQ.MI));
						if ( verbose )
						{
							libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
							std::cerr << "PairReadEndsMergeWorkPackageDispatcher package " << P << " instantiated ReadEndsBlockIndexSet" << std::endl;
						}

						if ( verbose )
						{
							libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
							std::cerr << "PairReadEndsMergeWorkPackageDispatcher package " << P << " instantiating DupSetCallbackSharedVector" << std::endl;
						}
						libmaus2::bambam::DupSetCallbackSharedVector dvec(*(BP->REQ.dupbitvec));
						if ( verbose )
						{
							libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
							std::cerr << "PairReadEndsMergeWorkPackageDispatcher package " << P << " instantiated DupSetCallbackSharedVector" << std::endl;
						}

						if ( verbose )
						{
							libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
							std::cerr << "PairReadEndsMergeWorkPackageDispatcher package " << P << " calling pairindexset.merge" << std::endl;
						}
						pairindexset.merge(
							BP->REQ.SMI,
							libmaus2::bambam::DupMarkBase::isDupPair,
							libmaus2::bambam::DupMarkBase::markDuplicatePairs,
							dvec);
						if ( verbose )
						{
							libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
							std::cerr << "PairReadEndsMergeWorkPackageDispatcher package " << P << " returned from pairindexset.merge" << std::endl;
						}

						if ( verbose )
						{
							libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
							std::cerr << "PairReadEndsMergeWorkPackageDispatcher package " << P << " calling addDuplicationMetrics" << std::endl;
						}
						addDuplicationMetricsInterface.addDuplicationMetrics(dvec.metrics);
						if ( verbose )
						{
							libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
							std::cerr << "PairReadEndsMergeWorkPackageDispatcher package " << P << " returned from addDuplicationMetrics" << std::endl;
						}

						mergeFinishedInterface.pairReadEndsMergeWorkPackageFinished(BP);
						packageReturnInterface.pairReadEndsMergeWorkPackageReturn(BP);

						if ( verbose )
						{
							libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
							std::cerr << "PairReadEndsMergeWorkPackageDispatcher package " << P << " end" << std::endl;
						}
					}
					catch(std::exception const & ex)
					{
						libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
						std::cerr << "PairReadEndsMergeWorkPackageDispatcher exception: " << ex.what() << std::endl;
						throw;
					}

				}
			};
		}
	}
}
#endif
