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
#include <libmaus2/bambam/ReadEnds.hpp>

::libmaus2::util::DigitTable const ::libmaus2::bambam::ReadEndsBase::D;

std::ostream & libmaus2::bambam::operator<<(std::ostream & out, libmaus2::bambam::ReadEnds::read_end_orientation reo)
{
	switch ( reo )
	{
		case libmaus2::bambam::ReadEnds::F: out << "F"; break;
		case libmaus2::bambam::ReadEnds::R: out << "R"; break;
		case libmaus2::bambam::ReadEnds::FF: out << "FF"; break;
		case libmaus2::bambam::ReadEnds::FR: out << "FR"; break;
		case libmaus2::bambam::ReadEnds::RF: out << "RF"; break;
		case libmaus2::bambam::ReadEnds::RR: out << "RR"; break;
	}

	return out;
}

std::ostream & libmaus2::bambam::operator<<(std::ostream & out, libmaus2::bambam::ReadEndsBase const & RE)
{
	out << "ReadEndsBase("
		<< "libId=" << RE.libraryId << ","
		<< "r1Seq=" << RE.read1Sequence << ","
		<< "r1Cor=" << RE.read1Coordinate << "(" << RE.getRead1Coordinate() << ")" << ","
		<< "orie=" << RE.orientation << ","
		<< "r2Seq=" << RE.read2Sequence << ","
		<< "r2Cor=" << RE.read2Coordinate << "(" << RE.getRead2Coordinate() << ")" << ","
		<< "r1Rank=" << RE.read1IndexInFile << ","
		<< "r2Rank=" << RE.read2IndexInFile << ","
		<< "score=" << RE.score << ","
		<< "RG=" << RE.readGroup << ","
		<< "tile=" << static_cast<int>(RE.tile) << ","
		<< "x=" << RE.x << ","
		<< "y=" << RE.y
		<< ")";

	return out;
}

std::ostream & libmaus2::bambam::operator<<(std::ostream & out, libmaus2::bambam::ReadEnds const & RE)
{
	out << "ReadEnds(" << static_cast<libmaus2::bambam::ReadEndsBase const &>(RE) << ")";

	if ( RE.p )
		out << "[" << RE.p->getName() << "]";

	return out;
}
