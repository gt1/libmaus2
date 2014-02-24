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
#if ! defined(LIBMAUS_FASTX_FASTASTREAMSET_HPP)
#define LIBMAUS_FASTX_FASTASTREAMSET_HPP

#include <libmaus/fastx/FastAStream.hpp>

namespace libmaus
{
	namespace fastx
	{
		struct FastAStreamSet
		{
			::libmaus::fastx::FastALineParser parser;
			
			FastAStreamSet(std::istream & in) : parser(in) {}
			
			bool getNextStream(std::pair<std::string,FastAStream::shared_ptr_type> & P)
			{
				::libmaus::fastx::FastALineParserLineInfo line;
				
				if ( ! parser.getNextLine(line) )
					return false;
				
				if ( line.linetype != ::libmaus::fastx::FastALineParserLineInfo::libmaus_fastx_fasta_id_line )
				{
					libmaus::exception::LibMausException se;
					se.getStream() << "FastAStreamSet::getNextStream(): unexpected line type" << std::endl;
					se.finish();
					throw se;
				}
				
				std::string const id(line.line,line.line+line.linelen);		

				libmaus::fastx::FastAStream::shared_ptr_type ptr(
					new libmaus::fastx::FastAStream(parser,64*1024,0));
					
				P.first = id;
				P.second = ptr;
				
				return true;
			}
		};
	}
}
#endif
