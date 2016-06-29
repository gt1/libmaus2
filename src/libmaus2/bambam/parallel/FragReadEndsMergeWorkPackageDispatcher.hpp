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
#if ! defined(LIBMAUS2_BAMBAM_PARALLEL_FRAGREADENDSMERGEWORKPACKAGEDISPATCHER_HPP)
#define LIBMAUS2_BAMBAM_PARALLEL_FRAGREADENDSMERGEWORKPACKAGEDISPATCHER_HPP

#include <libmaus2/bambam/parallel/FragReadEndsMergeWorkPackageReturnInterface.hpp>
#include <libmaus2/bambam/parallel/FragReadEndsMergeWorkPackageFinishedInterface.hpp>
#include <libmaus2/bambam/parallel/AddDuplicationMetricsInterface.hpp>
#include <libmaus2/parallel/SimpleThreadWorkPackageDispatcher.hpp>
#include <libmaus2/bambam/ReadEndsBlockIndexSet.hpp>

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{
			struct FragReadEndsMergeWorkPackageDispatcher : public libmaus2::parallel::SimpleThreadWorkPackageDispatcher
			{
				typedef FragReadEndsMergeWorkPackageDispatcher this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				FragReadEndsMergeWorkPackageReturnInterface & packageReturnInterface;
				FragReadEndsMergeWorkPackageFinishedInterface & mergeFinishedInterface;
				AddDuplicationMetricsInterface & addDuplicationMetricsInterface;
				int verbose;

				FragReadEndsMergeWorkPackageDispatcher(
					FragReadEndsMergeWorkPackageReturnInterface & rpackageReturnInterface,
					FragReadEndsMergeWorkPackageFinishedInterface & rmergeFinishedInterface,
					AddDuplicationMetricsInterface & raddDuplicationMetricsInterface
				) : libmaus2::parallel::SimpleThreadWorkPackageDispatcher(),
				    packageReturnInterface(rpackageReturnInterface), mergeFinishedInterface(rmergeFinishedInterface),
				    addDuplicationMetricsInterface(raddDuplicationMetricsInterface)
				{

				}
				virtual ~FragReadEndsMergeWorkPackageDispatcher() {}

				void setVerbose(int const rverbose)
				{
					verbose = rverbose;
				}

				virtual void dispatch(libmaus2::parallel::SimpleThreadWorkPackage * P, libmaus2::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & /* tpi */)
				{
					try
					{
						FragReadEndsMergeWorkPackage * BP = dynamic_cast<FragReadEndsMergeWorkPackage *>(P);
						assert ( BP );

						if ( verbose )
						{
							libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
							std::cerr << "FragReadEndsMergeWorkPackageDispatcher package " << P << " instantiating ReadEndsBlockIndexSet" << std::endl;
						}
						ReadEndsBlockIndexSet fragindexset(*(BP->REQ.MI));
						if ( verbose )
						{
							libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
							std::cerr << "FragReadEndsMergeWorkPackageDispatcher package " << P << " instantiated ReadEndsBlockIndexSet" << std::endl;
						}

						if ( verbose )
						{
							libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
							std::cerr << "FragReadEndsMergeWorkPackageDispatcher package " << P << " instantiating DupSetCallbackSharedVector" << std::endl;
						}
						libmaus2::bambam::DupSetCallbackSharedVector dvec(*(BP->REQ.dupbitvec));
						if ( verbose )
						{
							libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
							std::cerr << "FragReadEndsMergeWorkPackageDispatcher package " << P << " instantiated DupSetCallbackSharedVector" << std::endl;
						}

						if ( verbose )
						{
							libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
							std::cerr << "FragReadEndsMergeWorkPackageDispatcher package " << P << " calling fragindexset.merge" << std::endl;
						}
						fragindexset.merge(
							BP->REQ.SMI,
							libmaus2::bambam::DupMarkBase::isDupFrag,
							libmaus2::bambam::DupMarkBase::markDuplicateFrags,dvec
						);
						if ( verbose )
						{
							libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
							std::cerr << "FragReadEndsMergeWorkPackageDispatcher package " << P << " returned from fragindexset.merge" << std::endl;
						}

						if ( verbose )
						{
							libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
							std::cerr << "FragReadEndsMergeWorkPackageDispatcher package " << P << " calling addDuplicationMetrics" << std::endl;
						}
						addDuplicationMetricsInterface.addDuplicationMetrics(dvec.metrics);
						if ( verbose )
						{
							libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
							std::cerr << "FragReadEndsMergeWorkPackageDispatcher package " << P << " returned from addDuplicationMetrics" << std::endl;
						}

						mergeFinishedInterface.fragReadEndsMergeWorkPackageFinished(BP);
						packageReturnInterface.fragReadEndsMergeWorkPackageReturn(BP);

						if ( verbose )
						{
							libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
							std::cerr << "FragReadEndsMergeWorkPackageDispatcher package " << P << " end" << std::endl;
						}
					}
					catch(std::exception const & ex)
					{
						libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
						std::cerr << "FragReadEndsMergeWorkPackageDispatcher package " << P << " exception: " << ex.what() << std::endl;
						throw;
					}
				}
			};
		}
	}
}
#endif
