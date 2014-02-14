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
#if ! defined(LIBMAUS_FASTX_FASTALINEPARSERLINEINFO_HPP)
#define LIBMAUS_FASTX_FASTALINEPARSERLINEINFO_HPP

#include <libmaus/types/types.hpp>
#include <ostream>

namespace libmaus
{
	namespace fastx
	{
		struct FastALineParserLineInfo
		{
			enum line_type_type { libmaus_fastx_fasta_id_line, libmaus_fastx_fasta_base_line, libmaus_fastx_fasta_id_line_eof };
				
			uint8_t const * line;
			line_type_type linetype;
			uint64_t linelen;
			
			FastALineParserLineInfo()
			: line(0), linetype(libmaus_fastx_fasta_id_line_eof), linelen(0) {}
		};

		std::ostream & operator<<(std::ostream & out, ::libmaus::fastx::FastALineParserLineInfo const & line);
	}
}
#endif
