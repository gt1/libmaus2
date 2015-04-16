/*
    libmaus
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#include <libmaus/fastx/FastAMatchTable.hpp>

std::ostream & libmaus::fastx::operator<<(std::ostream & out, libmaus::fastx::FastAMatchTable const & F)
{
	for ( std::map< uint8_t, std::vector<uint8_t> >::const_iterator ita = F.M.begin();
		ita != F.M.end(); ++ita )
	{
		out << ita->first << '\t';
		std::vector<uint8_t> const & V = ita->second;
		for ( uint64_t i = 0; i < V.size(); ++i )
			out << V[i] << ((i+1<V.size())?",":"");
		out << '\n';
	}
	
	return out;
}
