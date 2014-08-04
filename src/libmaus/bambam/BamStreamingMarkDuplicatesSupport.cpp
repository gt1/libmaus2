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
#include <libmaus/bambam/BamStreamingMarkDuplicatesSupport.hpp>

std::ostream & libmaus::bambam::operator<<(
	std::ostream & out, 
	libmaus::bambam::BamStreamingMarkDuplicatesSupport::PairHashKeyType::pair_orientation_type const & ori
)
{
	switch ( ori )
	{
		case libmaus::bambam::BamStreamingMarkDuplicatesSupport::PairHashKeyType::pair_orientaton_FF: return out << "FF";
		case libmaus::bambam::BamStreamingMarkDuplicatesSupport::PairHashKeyType::pair_orientaton_FR: return out << "FR";
		case libmaus::bambam::BamStreamingMarkDuplicatesSupport::PairHashKeyType::pair_orientaton_RF: return out << "RF";
		case libmaus::bambam::BamStreamingMarkDuplicatesSupport::PairHashKeyType::pair_orientaton_RR: return out << "RR";
		default: return out << "unknown_orientation";
	}
}

std::ostream & libmaus::bambam::operator<<(std::ostream & ostr, libmaus::bambam::BamStreamingMarkDuplicatesSupport::PairHashKeyType const & H)
{
	ostr << "PairHashKeyType(";
	ostr << "refid=" << H.getRefId();
	ostr << ",";
	ostr << "coord=" << H.getCoord();
	ostr << ",";
	ostr << "materefid=" << H.getMateRefId();
	ostr << ",";
	ostr << "matecoord=" << H.getMateCoord();
	ostr << ",";
	ostr << "lib=" << H.getLibrary();
	ostr << ",";
	ostr << "left=" << H.getLeft();
	ostr << ",";
	ostr << "orientation=" << H.getOrientation();
	ostr << ")";
	
	return ostr;
}

std::ostream & libmaus::bambam::operator<<(
	std::ostream & out, 
	libmaus::bambam::BamStreamingMarkDuplicatesSupport::FragmentHashKeyType::fragment_orientation_type const & ori
)
{
	switch ( ori )
	{
		case libmaus::bambam::BamStreamingMarkDuplicatesSupport::FragmentHashKeyType::fragment_orientation_F: return out << "F";
		case libmaus::bambam::BamStreamingMarkDuplicatesSupport::FragmentHashKeyType::fragment_orientation_R: return out << "R";
		default: return out << "unknown_orientation";
	}
}

std::ostream & libmaus::bambam::operator<<(std::ostream & ostr, libmaus::bambam::BamStreamingMarkDuplicatesSupport::FragmentHashKeyType const & H)
{
	ostr << "FragmentHashKeyType(";
	ostr << "refid=" << H.getRefId();
	ostr << ",";
	ostr << "coord=" << H.getCoord();
	ostr << ",";
	ostr << "lib=" << H.getLibrary();
	ostr << ",";
	ostr << "orientation=" << H.getOrientation();
	ostr << " v=" << H.key.A[0] << "," << H.key.A[1] << "," << H.key.A[2];
	ostr << ")";
	
	return ostr;
}

std::ostream & libmaus::bambam::operator<<(std::ostream & out, libmaus::bambam::BamStreamingMarkDuplicatesSupport::OpticalInfoListNode const & O)
{
	out << "OpticalInfoListNode(";
	
	out << O.readgroup << "," << O.tile << "," << O.x << "," << O.y;
	
	out << ")";
	return out;
}

std::ostream & libmaus::bambam::operator<<(std::ostream & out, libmaus::bambam::BamStreamingMarkDuplicatesSupport::OpticalExternalInfoElement const & O)
{
	libmaus::bambam::BamStreamingMarkDuplicatesSupport::PairHashKeyType HK(O.key);
	out << "OpticalExternalInfoElement(";
	out << HK << ",";
	out << O.readgroup << ",";
	out << O.tile << ",";
	out << O.x << ",";
	out << O.y;
	out << ")";
	
	return out;
}
