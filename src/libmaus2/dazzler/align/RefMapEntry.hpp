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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_REFMAPENTRY_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_REFMAPENTRY_HPP

#include <libmaus2/util/NumberSerialisation.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct RefMapEntry
			{
				uint64_t refid;
				uint64_t offset;

				RefMapEntry()
				{

				}

				RefMapEntry(uint64_t const rrefid, uint64_t const roffset)
				: refid(rrefid), offset(roffset)
				{

				}

				RefMapEntry(std::istream & in)
				{
					deserialise(in);
				}

				std::ostream & serialise(std::ostream & out) const
				{
					libmaus2::util::NumberSerialisation::serialiseNumber(out,refid);
					libmaus2::util::NumberSerialisation::serialiseNumber(out,offset);
					return out;
				}

				std::istream & deserialise(std::istream & in)
				{
					refid = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
					offset = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
					return in;
				}
			};
		}
	}
}
#endif
