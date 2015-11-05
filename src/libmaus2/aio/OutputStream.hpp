/*
    libmaus2
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2015 Genome Research Limited

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
#if ! defined(LIBMAUS2_AIO_OUTPUTSTREAM_HPP)
#define LIBMAUS2_AIO_OUTPUTSTREAM_HPP

#include <libmaus2/aio/OutputStreamWrapper.hpp>

namespace libmaus2
{
	namespace aio
	{
		struct OutputStream : public OutputStreamWrapper, std::ostream
		{
			typedef OutputStream this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			OutputStream(OutputStreamWrapper::shared_stream_ptr_type & Tstream) : OutputStreamWrapper(Tstream), std::ostream(OutputStreamWrapper::getStream().rdbuf()) {}
			OutputStream(OutputStreamWrapper::stream_type & rstream)            : OutputStreamWrapper(rstream), std::ostream(OutputStreamWrapper::getStream().rdbuf()) {}
			virtual ~OutputStream() {}
		};
	}
}
#endif
