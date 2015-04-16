/*
    libmaus2
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
*/

#if ! defined(OUTPUTBUFFER84BIT_HPP)
#define OUTPUTBUFFER84BIT_HPP

#include <libmaus2/aio/OutputBuffer8.hpp>

namespace libmaus2
{
	namespace aio
	{
		/**
		 * class for writing 4 bit number aggregated in 8 byte words
		 **/
		struct OutputBuffer84Bit : public OutputBuffer8
		{
			private:
			unsigned int cc;
			uint64_t lv;
		
			public:
			/**
			 * constructor
			 *
			 * @param filename output file name
			 * @param bufsize size of output bffer in words
			 **/
			OutputBuffer84Bit(std::string const & filename, uint64_t const bufsize)
			: OutputBuffer8(filename,bufsize), cc(0), lv(0)
			{
		
			}
		
			/**
			 * flush buffer
			 **/
			void flush()
			{
				while ( cc )
					put(0);
				OutputBuffer8::flush();
			}
		
			/**
			 * put one element, flush if buffer is full after putting the new element
			 **/
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
			
			/**
			 * write padding
			 **/
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
