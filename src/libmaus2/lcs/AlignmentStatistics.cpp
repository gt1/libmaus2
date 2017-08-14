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
#include <sstream>

std::ostream & libmaus2::lcs::operator<<(std::ostream & out, libmaus2::lcs::AlignmentStatistics const & A)
{
	std::ostringstream sostr;
	sostr << std::fixed << std::setprecision(10) << A.getErrorRate();

	return out << "AlignmentStatistics("
		<< "matches=" << A.matches << ","
		<< "mismatches=" << A.mismatches << ","
		<< "insertions=" << A.insertions << ","
		<< "deletions=" << A.deletions  << ","
		<< "editdistance=" << A.mismatches+A.insertions+A.deletions << ","
		<< "erate=" << sostr.str()
		<< ")";
}

static void expect(std::istream & in, std::string const & s)
{
	uint64_t i = 0;

	while ( in.peek() != std::istream::traits_type::eof() && i < s.size() )
	{
		int const c = in.get();

		if ( c == s[i] )
			++i;
		else
		{
			libmaus2::exception::LibMausException lme;
			lme.getStream() << "std::istream & libmaus2::lcs::operator>>(std::istream & in, libmaus2::lcs::AlignmentStatistics & A): failed to find " << s << std::endl;
			lme.finish();
			throw lme;
		}
	}
}

std::istream & libmaus2::lcs::operator>>(std::istream & in, libmaus2::lcs::AlignmentStatistics & A)
{
	expect(in,"AlignmentStatistics(");
	expect(in,"matches=");
	in >> A.matches;
	expect(in,",mismatches=");
	in >> A.mismatches;
	expect(in,",insertions=");
	in >> A.insertions;
	expect(in,",deletions=");
	in >> A.deletions;
	expect(in,",editdistance=");
	uint64_t ed;
	in >> ed;
	expect(in,",erate=");
	double erate;
	in >> erate;
	expect(in,")");
	return in;
}
