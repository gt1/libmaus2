/*
    libmaus2
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
#include <libmaus2/bambam/GeneFlatFileEntry.hpp>

std::ostream & libmaus2::bambam::operator<<(std::ostream & out, libmaus2::bambam::GeneFlatFileEntry const & O)
{
	out.write(O.geneName.first,O.geneName.second-O.geneName.first); out.put('\t');
	out.write(O.name.first,O.name.second-O.name.first); out.put('\t');
	out.write(O.chrom.first,O.chrom.second-O.chrom.first); out.put('\t');
	out.put(O.strand); out.put('\t');
	out << O.txStart; out.put('\t');
	out << O.txEnd; out.put('\t');
	out << O.cdsStart; out.put('\t');
	out << O.cdsEnd; out.put('\t');
	out << O.exons.size(); out.put('\t');

	for ( uint64_t i = 0; i < O.exons.size(); ++i )
		out << O.exons[i].first << ",";
	out.put('\t');

	for ( uint64_t i = 0; i < O.exons.size(); ++i )
		out << O.exons[i].second << ",";

	return out;
}
