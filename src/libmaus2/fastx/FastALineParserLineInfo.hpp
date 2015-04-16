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
#if ! defined(LIBMAUS2_FASTX_FASTALINEPARSERLINEINFO_HPP)
#define LIBMAUS2_FASTX_FASTALINEPARSERLINEINFO_HPP

#include <libmaus2/types/types.hpp>
#include <ostream>

namespace libmaus2
{
	namespace fastx
	{
		struct FastALineParserLineInfo
		{
			enum line_type_type { libmaus2_fastx_fasta_id_line, libmaus2_fastx_fasta_base_line, libmaus2_fastx_fasta_id_line_eof };
				
			uint8_t const * line;
			line_type_type linetype;
			uint64_t linelen;
			
			FastALineParserLineInfo()
			: line(0), linetype(libmaus2_fastx_fasta_id_line_eof), linelen(0) {}
		};

		std::ostream & operator<<(std::ostream & out, ::libmaus2::fastx::FastALineParserLineInfo const & line);
	}
}
#endif
