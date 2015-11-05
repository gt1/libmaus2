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
#include <libmaus2/lcs/AlignmentStatistics.hpp>

std::ostream & libmaus2::lcs::operator<<(std::ostream & out, libmaus2::lcs::AlignmentStatistics const & A)
{
	return out << "AlignmentStatistics("
		<< "matches=" << A.matches << ","
		<< "mismatches=" << A.mismatches << ","
		<< "insertions=" << A.insertions << ","
		<< "deletions=" << A.deletions  << ","
		<< "editdistance=" << A.mismatches+A.insertions+A.deletions << ","
		<< "erate=" << A.getErrorRate()
		<< ")";
}
