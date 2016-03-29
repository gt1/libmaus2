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
#if ! defined(LIBMAUS2_AIO_ARRAYINPUTSTREAM_HPP)
#define LIBMAUS2_AIO_ARRAYINPUTSTREAM_HPP

#include <libmaus2/aio/ArrayInputStreamBuffer.hpp>

namespace libmaus2
{
	namespace aio
	{
		template<typename _iterator>
		struct ArrayInputStream : public ArrayInputStreamBuffer<_iterator>, public std::istream
		{
			typedef _iterator iterator;
			typedef ArrayInputStream<iterator> this_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			ArrayInputStream(iterator ita, iterator ite, int64_t const bufsize = -1, uint64_t const pushbacksize = 0)
			: ArrayInputStreamBuffer<iterator>(ita,ite,bufsize,pushbacksize),
			  std::istream(this)
			{
				exceptions(std::ios::badbit);
			}
		};
	}
}
#endif
