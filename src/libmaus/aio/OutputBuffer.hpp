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


#if ! defined(OUTPUTBUFFER_HPP)
#define OUTPUTBUFFER_HPP

#include <libmaus/aio/AsynchronousWriter.hpp>

namespace libmaus
{
	namespace aio
	{
		struct OutputBuffer
		{
			::libmaus::autoarray::AutoArray<uint8_t> B;
			uint8_t * const pa;
			uint8_t * pc;
			uint8_t * const pe;
			::libmaus::aio::AsynchronousWriter W;
		
			OutputBuffer(std::string const & filename, uint64_t const bufsize)
			: B(bufsize), pa(B.get()), pc(pa), pe(pa+B.getN()), W(filename,16)
			{
		
			}
		
			void flush()
			{
				writeBuffer();
				W.flush();
			}
		
			void writeBuffer()
			{
				W.write ( 
					reinterpret_cast<char const *>(pa),
					reinterpret_cast<char const *>(pc));
				pc = pa;
			}
		
			void put(uint8_t const c)
			{
				*(pc++) = c;
				if ( pc == pe )
					writeBuffer();
			}
		};
	}
}
#endif
