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
#include <libmaus2/bambam/BamHeader.hpp>

std::ostream & libmaus2::bambam::operator<<(std::ostream & out, libmaus2::bambam::BamHeaderParserStateBase::bam_header_parse_state const & state)
{
	switch(state)
	{
		case libmaus2::bambam::BamHeaderParserStateBase::bam_header_read_magic:
			out << "bam_header_read_magic";
			break;
		case libmaus2::bambam::BamHeaderParserStateBase::bam_header_read_l_text:
			out << "bam_header_read_l_text";
			break;
		case libmaus2::bambam::BamHeaderParserStateBase::bam_header_read_text:
			out << "bam_header_read_text";
			break;
		case libmaus2::bambam::BamHeaderParserStateBase::bam_header_read_n_ref:
			out << "bam_header_read_n_ref";
			break;
		case libmaus2::bambam::BamHeaderParserStateBase::bam_header_reaf_ref_l_name:
			out << "bam_header_reaf_ref_l_name";
			break;
		case libmaus2::bambam::BamHeaderParserStateBase::bam_header_read_ref_name:
			out << "bam_header_read_ref_name";
			break;
		case libmaus2::bambam::BamHeaderParserStateBase::bam_header_read_ref_l_ref:
			out << "bam_header_read_ref_l_ref";
			break;
		case libmaus2::bambam::BamHeaderParserStateBase::bam_header_read_done:
			out << "bam_header_read_done";
			break;
		case libmaus2::bambam::BamHeaderParserStateBase::bam_header_read_failed:
			out << "bam_header_read_failed";
			break;
		default:
			out << "bam_header_read_unknown";
			break;
	}

	return out;
}
