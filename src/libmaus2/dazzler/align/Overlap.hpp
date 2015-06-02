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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_OVERLAP_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_OVERLAP_HPP

#include <libmaus2/dazzler/align/Path.hpp>
		
namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct Overlap : public libmaus2::dazzler::db::InputBase
			{
				Path path;
				uint32_t flags;
				int32_t aread;
				int32_t bread;
				
				// 40
				
				void deserialise(std::istream & in)
				{
					path.deserialise(in);
					
					uint64_t offset = 0;
					flags = getUnsignedLittleEndianInteger4(in,offset);
					aread = getLittleEndianInteger4(in,offset);
					bread = getLittleEndianInteger4(in,offset);

					getLittleEndianInteger4(in,offset); // padding
					
					// std::cerr << "flags=" << flags << " aread=" << aread << " bread=" << bread << std::endl;
				}
				
				Overlap()
				{
				
				}
				
				Overlap(std::istream & in)
				{
					deserialise(in);
				}
				
				bool isInverse() const
				{
					return (flags & 1) != 0;
				}
				
				uint64_t getNumErrors() const
				{
					return path.getNumErrors();
				}
			};

			std::ostream & operator<<(std::ostream & out, Overlap const & P);
		}
	}
}
#endif
