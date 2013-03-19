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
#if ! defined(LIBMAUS_AIO_CIRCULARWRAPPER_HPP)
#define LIBMAUS_AIO_CIRCULARWRAPPER_HPP

#include <libmaus/aio/CircularBuffer.hpp>

namespace libmaus
{
	namespace aio
	{
		struct CircularWrapper : public CircularBuffer, public ::std::istream
		{
			CircularWrapper(
				std::string const & filename, 
				uint64_t const offset = 0,
				uint64_t const buffersize = 64*1024, 
				uint64_t const pushbackspace = 64
			)
			: CircularBuffer(filename,offset,buffersize,pushbackspace), ::std::istream(this)
			{
				
			}
			uint64_t tellg() const
			{
				return CircularBuffer::tellg();
			}
		};
		struct CircularReverseWrapper : public CircularReverseBuffer, public ::std::istream
		{
			CircularReverseWrapper(
				std::string const & filename, 
				uint64_t const offset = 0,
				uint64_t const buffersize = 64*1024, 
				uint64_t const pushbackspace = 64
			)
			: CircularReverseBuffer(filename,offset,buffersize,pushbackspace), ::std::istream(this)
			{
				
			}
			uint64_t tellg() const
			{
				return CircularReverseBuffer::tellg();
			}
		};
	}
}
#endif
