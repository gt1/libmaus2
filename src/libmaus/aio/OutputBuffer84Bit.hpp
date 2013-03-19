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

#if ! defined(OUTPUTBUFFER84BIT_HPP)
#define OUTPUTBUFFER84BIT_HPP

#include <libmaus/aio/OutputBuffer8.hpp>

namespace libmaus
{
	namespace aio
	{
		struct OutputBuffer84Bit : public OutputBuffer8
		{
			unsigned int cc;
			uint64_t lv;
		
			OutputBuffer84Bit(std::string const & filename, uint64_t const bufsize)
			: OutputBuffer8(filename,bufsize), cc(0), lv(0)
			{
		
			}
		
			void flush()
			{
				while ( cc )
					put(0);
				OutputBuffer8::flush();
			}
		
			void put(uint8_t const c)
			{
				assert ( c < 16 );
		
				lv <<= 4;
				lv |= c;
				cc += 1;
		
				if ( cc == 16 )
				{
					OutputBuffer8::put(lv);
					lv = 0;
					cc = 0;
				}
			}
			
			void writePad()
			{
				flush();
				OutputBuffer8::put(0);
				OutputBuffer8::flush();
			}
		};
	}
}
#endif
