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
#if ! defined(OUTPUTBUFFER_HPP)
#define OUTPUTBUFFER_HPP

#include <libmaus2/aio/AsynchronousWriter.hpp>

namespace libmaus2
{
	namespace aio
	{
		/**
		 * byte oriented asynchronous output buffer
		 **/
		struct OutputBuffer
		{
			private:
			//! buffer
			::libmaus2::autoarray::AutoArray<uint8_t> B;
			//! buffer start pointer
			uint8_t * const pa;
			//! buffer current pointer
			uint8_t * pc;
			//! buffer end pointer
			uint8_t * const pe;
			//! async writer
			::libmaus2::aio::AsynchronousWriter W;

			/**
			 * write buffer contents to writer
			 **/
			void writeBuffer()
			{
				W.write ( 
					reinterpret_cast<char const *>(pa),
					reinterpret_cast<char const *>(pc));
				pc = pa;
			}
		
			public:
			/**
			 * constructor
			 *
			 * @param filename output file name
			 * @param bufsize size of output buffer in elements
			 **/
			OutputBuffer(std::string const & filename, uint64_t const bufsize)
			: B(bufsize), pa(B.get()), pc(pa), pe(pa+B.getN()), W(filename,16)
			{
		
			}
		
			/**
			 * flush the output buffer
			 **/
			void flush()
			{
				writeBuffer();
				W.flush();
			}
		
			/**
			 * put one element and flush buffer it is full afterwards
			 *
			 * @param c element to be put in buffer
			 **/
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
