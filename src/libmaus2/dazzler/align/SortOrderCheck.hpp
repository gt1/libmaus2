/*
    daccord
    Copyright (C) 2017 German Tischler

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
#include <libmaus2/dazzler/align/OverlapIndexer.hpp>
#include <libmaus2/dazzler/align/SortingOverlapOutputBuffer.hpp>

#if ! defined(LIBMAUS2_DAZZLER_ALIGN_SORTORDERCHECK_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_SORTORDERCHECK_HPP

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct SortOrderCheck
			{
				template<typename comparator_type>
				static bool checkSortOrder(std::string const & infn, std::ostream & errOSI, comparator_type const & comp)
				{
					libmaus2::dazzler::align::AlignmentFileRegion::unique_ptr_type AFR(libmaus2::dazzler::align::OverlapIndexer::openAlignmentFileWithoutIndex(infn));
					libmaus2::dazzler::align::Overlap OVLprev;
					bool ovlprevvalid = false;
					libmaus2::dazzler::align::Overlap OVL;
					bool ok = true;

					while ( ok && AFR->getNextOverlap(OVL) )
					{
						ok = (!ovlprevvalid) || comp(OVLprev,OVL);

						if ( ! ok )
						{
							libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
							errOSI << "[V] broken order in " << infn << "\n";
							errOSI << OVLprev << "\n";
							errOSI << OVL << "\n";
							errOSI.flush();
						}

						OVLprev = OVL;
						ovlprevvalid = true;
					}

					return ok;
				}

				template<typename comparator_type>
				static void checkSortState(
					std::vector<std::string> const & Vin, uint64_t const numthreads, std::string const & tmpbase, std::ostream & errOSI,
					comparator_type const & comp
				)
				{
					errOSI << "[V] checking file sort state" << std::endl;


					#if defined(_OPENMP)
					#pragma omp parallel for schedule(dynamic,1) num_threads(numthreads)
					#endif
					for ( uint64_t i = 0; i < Vin.size(); ++i )
					{
						std::string const infn = Vin[i];
						bool ok = libmaus2::dazzler::align::SortOrderCheck::checkSortOrder(infn,errOSI,comp);

						if ( ! ok )
						{
							{
							libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
							errOSI << "[V] sorting " << infn << "\n";
							}

							std::ostringstream tmpprefix;
							tmpprefix << tmpbase << "_" << std::setw(6) << std::setfill('0') << i;

							libmaus2::dazzler::align::SortingOverlapOutputBuffer<libmaus2::dazzler::align::OverlapFullComparator>::sort(
								infn,tmpprefix.str(),
								libmaus2::dazzler::align::SortingOverlapOutputBuffer<libmaus2::dazzler::align::OverlapFullComparator>::getDefaultMergeFanIn(),
								1 /* sort threads */,
								1 /* merge threads */,
								comp
							);

							{
							libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
							errOSI << "[V] sorted " << infn << "\n";
							}
						}
					}

					errOSI << "[V] checked file sort state" << std::endl;
				}

				static void checkSortState(
					std::vector<std::string> const & Vin, uint64_t const numthreads, std::string const & tmpbase, std::ostream & errOSI
				)
				{
					libmaus2::dazzler::align::OverlapFullComparator const comp;
					checkSortState(Vin,numthreads,tmpbase,errOSI,comp);
				}
			};
		}
	}
}
#endif
