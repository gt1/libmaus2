/*
    libmaus2
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2015 Genome Research Limited

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
#include <libmaus2/fastx/NameInfoBase.hpp>

std::ostream & libmaus2::fastx::operator<<(std::ostream & out, NameInfoBase::fastq_name_scheme_type const namescheme)
{
	switch ( namescheme )
	{
		case NameInfoBase::fastq_name_scheme_generic:
			out << "generic";
			break;
		case NameInfoBase::fastq_name_scheme_casava18_single:
			out << "c18s";
			break;
		case NameInfoBase::fastq_name_scheme_casava18_paired_end:
			out << "c18pe";
			break;
		case NameInfoBase::fastq_name_scheme_pairedfiles:
			out << "pairedfiles";
			break;
		default:
			out << "unknown";
			break;
	}
	return out;
}
