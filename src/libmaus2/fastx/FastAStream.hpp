/*
    libmaus2
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
#if ! defined(LIBMAUS2_FASTX_FASTASTREAM_HPP)
#define LIBMAUS2_FASTX_FASTASTREAM_HPP

#include <libmaus2/fastx/FastAStreamWrapper.hpp>

namespace libmaus2
{
	namespace fastx
	{
		struct FastAStream : public FastAStreamWrapper, public ::std::istream
		{
			typedef FastAStream this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
		
			FastAStream(::libmaus2::fastx::FastALineParser & parser, uint64_t const buffersize, uint64_t const pushbackspace)
			: FastAStreamWrapper(parser,buffersize,pushbackspace), ::std::istream(this)
			{
				
			}
			uint64_t tellg() const
			{
				return FastAStreamWrapper::tellg();
			}
		};
	}
}
#endif
