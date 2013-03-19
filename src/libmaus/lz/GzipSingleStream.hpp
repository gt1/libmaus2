/**
    libmaus
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
**/
#if ! defined(GZIPSINGELSTREAM_HPP)
#define GZIPSINGELSTREAM_HPP

#include <libmaus/lz/GzipHeader.hpp>
#include <libmaus/lz/Inflate.hpp>

namespace libmaus
{
	namespace lz
	{
		struct GzipSingleStream
		{
			typedef GzipSingleStream this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			std::istream & in;
			::libmaus::lz::GzipHeader header;
			::libmaus::lz::Inflate infl;
			bool finished;
			
			GzipSingleStream(std::istream & rin) : in(rin), header(in), infl(in,-15), finished(false) {}
			
			uint64_t read(char * buffer, uint64_t n)
			{
				if ( finished )
					return 0;
					
				uint64_t red = 0;
				
				while ( n )
				{
					uint64_t subred = infl.read(buffer,n);
					if ( ! subred )
					{
						infl.ungetRest();

						std::ostringstream crcostr;	
						/* uint32_t const crc32 = */ ::libmaus::lz::GzipHeader::getLEInteger(in,4,&crcostr);
						/* uint32_t const isize = */ ::libmaus::lz::GzipHeader::getLEInteger(in,4,&crcostr);
						
						finished = true;

						return red;
					}
					red += subred;
					n -= subred;
					buffer += subred;
				}
				
				return red;
			}
		};
	}
}
#endif		
