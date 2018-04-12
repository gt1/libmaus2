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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_LASFILERANGE_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_LASFILERANGE_HPP

#include <libmaus2/types/types.hpp>
#include <libmaus2/dazzler/align/AlignmentFile.hpp>
#include <libmaus2/util/GetFileSize.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct LasFileRange
			{
				uint64_t id;
				uint64_t startoffset;
				uint64_t endoffset;

				LasFileRange() {}
				LasFileRange(
					uint64_t const rid,
					uint64_t const rstartoffset,
					uint64_t const rendoffset
				) : id(rid), startoffset(rstartoffset), endoffset(rendoffset) {}
				LasFileRange(std::string const & rfilename, uint64_t const rid)
				:
					id(rid),
					startoffset(libmaus2::dazzler::align::AlignmentFile::getSerialisedHeaderSize()),
					endoffset(libmaus2::util::GetFileSize::getFileSize(rfilename))
				{
				}

				static std::vector<LasFileRange> construct(std::vector<std::string> const & Vfn)
				{
					std::vector<LasFileRange> V;

					for ( uint64_t i = 0; i < Vfn.size(); ++i )
					{
						LasFileRange const LFR = LasFileRange(Vfn[i],i);
						if ( LFR.endoffset != LFR.startoffset )
							V.push_back(LFR);
					}

					return V;
				}
			};
		}
	}
}
#endif
