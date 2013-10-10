/*
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
*/
#if ! defined(GZIPSTREAM_HPP)
#define GZIPSTREAM_HPP

#include <libmaus/lz/GzipSingleStream.hpp>

namespace libmaus
{
	namespace lz
	{
		struct GzipStream
		{
			typedef GzipStream this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
		
			::libmaus::lz::StreamWrapper< ::std::istream > in;
			GzipSingleStream singlestream;
			uint64_t gcnt;
			uint64_t gpos;
			
			bool openNextStream()
			{
				// std::cerr << "opening new stream." << std::endl;
			
				if ( in.peek() < 0 )
				{
					// std::cerr << "peek() returned -1" << std::endl;
					return false;
				}
				
				singlestream.startNewBlock();
					
				return true;
			}
			
			GzipStream(std::istream & rin)
			: 
			  in(rin,::libmaus::lz::Inflate::input_buffer_size,::libmaus::lz::Inflate::input_buffer_size),
			  singlestream(in),
			  gcnt(0),
			  gpos(0)
			{
				
			}
			
			uint64_t tellg()
			{
				return gpos;
			}
			
			uint64_t read(char * buffer, uint64_t n)
			{
				uint64_t red = 0;

				while ( n )
				{
					uint64_t subred = singlestream.read(buffer,n);
					
					if ( subred )
					{
						buffer += subred;
						n -= subred;
						red += subred;
					}
					else
					{
						if ( ! openNextStream() )
						{
							// std::cerr << "no next stream." << std::endl;
							n = 0;
						}
					}
				}
				
				gcnt = red;
				gpos += red;
								
				return red;
			}
			
			int get()
			{
				uint8_t c;
				uint64_t const red = read(reinterpret_cast<char *>(&c),1);
				if ( red )
					return c;
				else
					return -1;
			}
			
			uint64_t gcount() const
			{
				// std::cerr << "gcnt " << gcnt << std::endl;
			
				return gcnt;
			}			
		};
	}
}
#endif
