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
#if ! defined(LIBMAUS2_DAZZLER_STRINGGRAPH_OVERLAPNODE_HPP)
#define LIBMAUS2_DAZZLER_STRINGGRAPH_OVERLAPNODE_HPP

#include <libmaus2/util/NumberSerialisation.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace stringgraph
		{
			struct OverlapNode;
			std::ostream & operator<<(std::ostream & out, OverlapNode const & O);

			struct OverlapNode
			{
				uint64_t id;
				bool end;

				OverlapNode() {}
				OverlapNode(uint64_t const rid, bool const rend) : id(rid), end(rend) {}
				OverlapNode(std::istream & in)
				:
					id(libmaus2::util::NumberSerialisation::deserialiseNumber(in,4)),
					end(libmaus2::util::NumberSerialisation::deserialiseNumber(in,1))
				{
					//std::cerr << "got OverlapNode " << *this << std::endl;
				}

				bool operator<(OverlapNode const & O) const
				{
					if ( id != O.id )
						return id < O.id;
					else if ( end != O.end )
						return end < O.end;
					else
						return false;
				}

				bool operator==(OverlapNode const & O) const
				{
					return id==O.id && end == O.end;
				}

				bool operator!=(OverlapNode const & O) const
				{
					return !operator==(O);
				}
			};

			std::ostream & operator<<(std::ostream & out, OverlapNode const & O);
		}
	}
}

inline std::ostream & libmaus2::dazzler::stringgraph::operator<<(std::ostream & out, libmaus2::dazzler::stringgraph::OverlapNode const & O)
{
	return out << "(" << O.id << "," << (O.end?'e':'b') << ")";
}
#endif
