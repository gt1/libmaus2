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
#include <libmaus2/dazzler/db/DatabaseFile.hpp>

std::ostream & libmaus2::dazzler::db::operator<<(std::ostream & out, libmaus2::dazzler::db::DatabaseFile const & D)
{
	out << "DatabaseFile("
		<< "isdam=" << D.isdam << ","
		<< "root=" << D.root << ","
		<< "path=" << D.path << ","
		<< "part=" << D.part << ","
		<< "dbpath=" << D.dbpath << ","
		<< "nfiles=" << D.nfiles << ",";

	for ( uint64_t i = 0; i < D.fileinfo.size(); ++i )
		out << D.fileinfo[i];
	out << ",";

	out
		<< "numblocks=" << D.numblocks << ","
		<< "blocksize=" << D.blocksize << ","
		<< "cutoff=" << D.cutoff << ","
		<< "all=" << D.all << ",";

	out << "blocks=";
	for ( uint64_t i = 0; i < D.blocks.size(); ++i )
		out << "(" << D.blocks[i].first << "," << D.blocks[i].second << ")";
	out << ",";

	out << "idxpath=" << D.idxpath << ",";
	out << D.indexbase << ",";
	out << "indexoffset=" << D.indexoffset;

	out << ")";

	return out;
}
