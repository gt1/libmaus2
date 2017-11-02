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
#include <libmaus2/dazzler/align/OverlapDataInterface.hpp>

std::ostream & libmaus2::dazzler::align::operator<<(std::ostream & out, libmaus2::dazzler::align::OverlapDataInterface const & O)
{
	out << "OverlapDataInterface("
		<< "a=" << O.aread() << ","
		<< "b=" << O.bread() << ","
		<< "b=" << O.flags() << ","
		<< "ab=" << O.abpos() << ","
		<< "ae=" << O.aepos() << ","
		<< "bb=" << O.bbpos() << ","
		<< "be=" << O.bepos()
		<< ")";
	return out;
}
