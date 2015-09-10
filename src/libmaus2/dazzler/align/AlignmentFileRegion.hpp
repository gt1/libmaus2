/*
    libmaus2
    Copyright (C) 2015 German Tischler

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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_ALIGNMENTFILEREGION_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_ALIGNMENTFILEREGION_HPP

#include <libmaus2/dazzler/align/AlignmentFile.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct AlignmentFileRegion
			{
				typedef AlignmentFileRegion this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
				
				libmaus2::aio::InputStreamInstance::unique_ptr_type Pfile;
				libmaus2::dazzler::align::AlignmentFile::unique_ptr_type Palgn;
				
				AlignmentFileRegion(
					libmaus2::aio::InputStreamInstance::unique_ptr_type & rPfile,
					libmaus2::dazzler::align::AlignmentFile::unique_ptr_type & rPalgn
				) : Pfile(UNIQUE_PTR_MOVE(rPfile)), Palgn(UNIQUE_PTR_MOVE(rPalgn))
				{
				
				}					

				bool peekNextOverlap(Overlap & OVL)
				{
					return Palgn->peekNextOverlap(*Pfile,OVL);
				}
				
				bool getNextOverlap(Overlap & OVL)
				{
					return Palgn->getNextOverlap(*Pfile,OVL);
				}

				bool getNextOverlap(Overlap & OVL, uint64_t & s)
				{
					return Palgn->getNextOverlap(*Pfile,OVL,s);				
				}
			};
		}
	}
}
#endif
