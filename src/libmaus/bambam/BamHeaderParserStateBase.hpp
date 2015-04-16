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
#if ! defined(LIBMAUS_BAMBAM_BAMHEADERPARSERSTATEBASE_HPP)
#define LIBMAUS_BAMBAM_BAMHEADERPARSERSTATEBASE_HPP

#include <ostream>

namespace libmaus
{
	namespace bambam
	{
		struct BamHeaderParserStateBase
		{
			enum bam_header_parse_state
			{
				bam_header_read_magic,
				bam_header_read_l_text,
				bam_header_read_text,
				bam_header_read_n_ref,
				bam_header_reaf_ref_l_name,
				bam_header_read_ref_name,
				bam_header_read_ref_l_ref,
				bam_header_read_done,
				bam_header_read_failed
			};
		};
			
		std::ostream & operator<<(std::ostream & out, BamHeaderParserStateBase::bam_header_parse_state const & state);
	}
}
#endif
