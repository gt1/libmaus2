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
#if ! defined(LIBMAUS_BAMBAM_ENCODERBASE_HPP)
#define LIBMAUS_BAMBAM_ENCODERBASE_HPP

#include <libmaus/fastx/CharBuffer.hpp>
#include <libmaus/bambam/CigarOperation.hpp>

namespace libmaus
{
	namespace bambam
	{
		/**
		 * encoder base class
		 **/
		struct EncoderBase
		{
			/**
			 * write number N as little endian on stream; sizeof(N) is used for determining the length of the code
			 *
			 * @param stream output stream
			 * @param v number to be encoded
			 **/
			template<typename stream_type, typename N>
			static void putLE(stream_type & stream, N const v)
			{
				for ( unsigned int i = 0; i < sizeof(v); ++i )
					stream.put( (v >> (i*8)) & 0xFF );
			}
		};
	}
}
#endif
