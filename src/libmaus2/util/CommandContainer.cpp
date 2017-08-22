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
#include <libmaus2/util/CommandContainer.hpp>

std::ostream & libmaus2::util::operator<<(std::ostream & out, libmaus2::util::CommandContainer const & CC)
{
	out << "CommandContainer(id=" << CC.id << ",attempt=" << CC.attempt << ",depid={";
	for ( uint64_t i = 0; i < CC.depid.size(); ++i )
		out << CC.depid[i] << ((i+1 < CC.depid.size())?",":"");
        out << "},rdepid={";
	for ( uint64_t i = 0; i < CC.rdepid.size(); ++i )
		out << CC.rdepid[i] << ((i+1 < CC.rdepid.size())?",":"");
        out << "},";
	out << "\n";
	for ( uint64_t i = 0; i < CC.V.size(); ++i )
		out << "\t" << CC.V[i] << "\n";
        return out;
}
