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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_ALIGNMENTFILECAT_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_ALIGNMENTFILECAT_HPP

#include <libmaus2/dazzler/align/OverlapIndexer.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct AlignmentFileCat
			{
				typedef AlignmentFileCat this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				std::vector<std::string> const & Vin;
				std::vector<uint64_t> const R;
				uint64_t Ri;
				uint64_t const from;
				uint64_t const to;

				libmaus2::dazzler::align::AlignmentFileRegion::unique_ptr_type ptr;

				bool openNext()
				{
					if ( Ri < R.size() )
					{
						uint64_t const fid = R[Ri++];
						libmaus2::dazzler::align::AlignmentFileRegion::unique_ptr_type aptr(libmaus2::dazzler::align::OverlapIndexer::openAlignmentFileRegion(Vin[fid],from,to));
						ptr = UNIQUE_PTR_MOVE(aptr);
						return true;
					}
					else
					{
						ptr.reset();
						return false;
					}
				}

				AlignmentFileCat(
					// vector of input file names
					std::vector<std::string> const & rVin,
					// indices in Vin relevant for [from,to)
					std::vector<uint64_t> const & rR,
					// interval
					uint64_t const rfrom,
					uint64_t const rto
				) : Vin(rVin), R(rR), Ri(0), from(rfrom), to(rto)
				{
					openNext();
				}

				bool getNextOverlap(libmaus2::dazzler::align::Overlap & OVL)
				{
					while ( ptr )
					{
						bool const ok = ptr->getNextOverlap(OVL);

						if ( ok )
							return true;
						else
							openNext();
					}

					return false;
				}

				bool peekNextOverlap(libmaus2::dazzler::align::Overlap & OVL)
				{
					while ( ptr )
					{
						bool const ok = ptr->peekNextOverlap(OVL);

						if ( ok )
							return true;
						else
							openNext();
					}

					return false;
				}
			};
		}
	}
}
#endif
