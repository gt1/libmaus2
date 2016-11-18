/*
    libmaus2
    Copyright (C) 2016 German Tischler

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
#if ! defined(LIBMAUS2_BAMBAM_BAMPEEKER_HPP)
#define LIBMAUS2_BAMBAM_BAMPEEKER_HPP

#include <libmaus2/bambam/BamDecoder.hpp>

namespace libmaus2
{
	namespace bambam
	{
		struct BamPeeker
		{
			libmaus2::bambam::BamDecoder dec;
			libmaus2::bambam::BamAlignment slot;
			bool slotfilled;

			BamPeeker(std::string const & fn)
			: dec(fn), slot(), slotfilled(false) {}

			bool getNext(libmaus2::bambam::BamAlignment & ralgn)
			{
				if ( slotfilled )
				{
					ralgn.copyFrom(slot);
					slotfilled = false;
					return true;
				}
				else
				{
					bool const ok = dec.readAlignment();

					if ( ok )
					{
						libmaus2::bambam::BamAlignment & algn = dec.getAlignment();
						ralgn.copyFrom(algn);
						return true;
					}
					else
					{
						return false;
					}
				}
			}

			bool peekNext(libmaus2::bambam::BamAlignment & ralgn)
			{
				if ( ! slotfilled )
					slotfilled = getNext(slot);
				if ( slotfilled )
				{
					ralgn.copyFrom(slot);
					return true;
				}
				else
				{
					return false;
				}
			}
		};
	}
}
#endif
