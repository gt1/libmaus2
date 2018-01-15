/*
    libmaus2
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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_FILTERUNIQUE_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_FILTERUNIQUE_HPP

#include <libmaus2/dazzler/align/OverlapIndexer.hpp>
#include <libmaus2/dazzler/align/SortingOverlapOutputBuffer.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct FilterUnique
			{
				static void filterUnique(std::string const & Coutmergefn, std::string const & Coutmergetmpfn)
				{
					int64_t const tspace = libmaus2::dazzler::align::AlignmentFile::getTSpace(Coutmergefn);
					libmaus2::dazzler::align::AlignmentFileRegion::unique_ptr_type pF(libmaus2::dazzler::align::OverlapIndexer::openAlignmentFileWithoutIndex(Coutmergefn));
					libmaus2::dazzler::align::AlignmentWriter::unique_ptr_type AW(
						new libmaus2::dazzler::align::AlignmentWriter(Coutmergetmpfn,tspace)
					);
					libmaus2::dazzler::align::Overlap OVLprev, OVL;
					bool prevvalid = false;
					libmaus2::dazzler::align::OverlapFullComparator comp;
					while ( pF->getNextOverlap(OVL) )
					{
						if ( (!prevvalid) || comp(OVLprev,OVL) )
							AW->put(OVL);

						prevvalid = true;
						OVLprev = OVL;
					}

					AW.reset();
					pF.reset();

					libmaus2::dazzler::align::SortingOverlapOutputBuffer<>::rename(Coutmergetmpfn,Coutmergefn);
				}

				template<typename comparator_type>
				static void sortFilterUnique(std::string const & Coutmergefn, std::string const & Coutmergetmpfn, comparator_type const & comp)
				{
					// sort
					libmaus2::dazzler::align::SortingOverlapOutputBuffer<comparator_type>::sort(
						Coutmergefn,std::string(),
						libmaus2::dazzler::align::SortingOverlapOutputBuffer<comparator_type>::getDefaultMergeFanIn(),
						1,1,comp
					);
					// get tspace
					int64_t const tspace = libmaus2::dazzler::align::AlignmentFile::getTSpace(Coutmergefn);
					// open input
					libmaus2::dazzler::align::AlignmentFileRegion::unique_ptr_type pF(libmaus2::dazzler::align::OverlapIndexer::openAlignmentFileWithoutIndex(Coutmergefn));
					// open output
					libmaus2::dazzler::align::AlignmentWriter::unique_ptr_type AW(
						new libmaus2::dazzler::align::AlignmentWriter(Coutmergetmpfn,tspace,false /* index */)
					);
					libmaus2::dazzler::align::Overlap OVLprev, OVL;
					bool prevvalid = false;
					while ( pF->getNextOverlap(OVL) )
					{
						if ( (!prevvalid) || comp(OVLprev,OVL) )
							AW->put(OVL);

						prevvalid = true;
						OVLprev = OVL;
					}

					AW.reset();
					pF.reset();

					libmaus2::dazzler::align::SortingOverlapOutputBuffer<>::rename(Coutmergetmpfn,Coutmergefn);
				}
			};
		}
	}
}
#endif
