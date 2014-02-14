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
#include <libmaus/fastx/FastALineParserLineInfo.hpp>
#include <string>

std::ostream & libmaus::fastx::operator<<(std::ostream & out, ::libmaus::fastx::FastALineParserLineInfo const & line)
{
	switch ( line.linetype )
	{
		case ::libmaus::fastx::FastALineParserLineInfo::libmaus_fastx_fasta_id_line_eof:
			out << "FastALineParserLineInfo(eof)";
			break;
		case ::libmaus::fastx::FastALineParserLineInfo::libmaus_fastx_fasta_id_line:
			out << "FastALineParserLineInfo(id," << std::string(
				reinterpret_cast<char const *>(line.line),
				reinterpret_cast<char const *>(line.line)+line.linelen
			) << ")";
			break;
		case ::libmaus::fastx::FastALineParserLineInfo::libmaus_fastx_fasta_base_line:
			out << "FastALineParserLineInfo(base," << std::string(
				reinterpret_cast<char const *>(line.line),
				reinterpret_cast<char const *>(line.line)+line.linelen
			) << ")";
			break;
	}
	
	return out;
}
