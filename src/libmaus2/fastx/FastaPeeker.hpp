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
#if ! defined(LIBMAUS2_FASTX_FASTAPEEKER_HPP)
#define LIBMAUS2_FASTX_FASTAPEEKER_HPP

#include <libmaus2/fastx/FastAReader.hpp>

namespace libmaus2
{
	namespace fastx
	{
		struct FastaPeeker
		{
			libmaus2::fastx::FastAReader dec;
			libmaus2::fastx::FastAReader::pattern_type slot;
			bool slotfilled;

			FastaPeeker(std::string const & fn)
			: dec(fn), slot(), slotfilled(false) {}

			bool getNext(libmaus2::fastx::FastAReader::pattern_type & ralgn)
			{
				if ( slotfilled )
				{
					ralgn = slot;
					slotfilled = false;
					return true;
				}
				else
				{
					libmaus2::fastx::FastAReader::pattern_type pat;
					bool const ok = dec.getNextPatternUnlocked(pat);

					if ( ok )
					{
						ralgn = pat;
						return true;
					}
					else
					{
						return false;
					}
				}
			}

			bool peekNext(libmaus2::fastx::FastAReader::pattern_type & ralgn)
			{
				if ( ! slotfilled )
					slotfilled = getNext(slot);
				if ( slotfilled )
				{
					ralgn = slot;
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
