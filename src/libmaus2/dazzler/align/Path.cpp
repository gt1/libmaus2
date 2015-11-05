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
#include <libmaus2/dazzler/align/Path.hpp>

std::ostream & libmaus2::dazzler::align::operator<<(std::ostream & out, Path const & P)
{
	out << "Path(";

	out << "tlen=" << P.tlen << ";";
	out << "diffs=" << P.diffs << ";";
	out << "abpos=" << P.abpos << ";";
	out << "bbpos=" << P.bbpos << ";";
	out << "aepos=" << P.aepos << ";";
	out << "bepos=" << P.bepos << ";";

	for ( size_t i = 0; i < P.path.size(); ++i )
		out << "[" << P.path[i].first << "," << P.path[i].second << "]" << (i+1 < P.path.size() ? ";" : "");

	out << ")";

	return out;
}
