/*
    libmaus2
    Copyright (C) 2009-2013 German Tischler
    Copyright (C) 2011-2013 Genome Research Limited

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

#if ! defined(DISTNODETYPE_HPP)
#define DISTNODETYPE_HPP

#include <ostream>

namespace libmaus2
{
	namespace lcs
	{
		enum dist_node_type { dist_primary, dist_secondary, dist_tertiary };

		inline std::ostream & operator<< ( std::ostream& out, dist_node_type const dnt)
		{
			switch ( dnt )
			{
				case dist_primary:
					out << "primary";
					break;
				case dist_secondary:
					out << "secondary";
					break;
				case dist_tertiary:
					out << "tertiary";
					break;
			}
			return out;
		}

	}
}
#endif
