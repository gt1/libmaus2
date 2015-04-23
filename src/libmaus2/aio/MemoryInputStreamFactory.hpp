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
#if ! defined(LIBMAUS2_AIO_MEMORYINPUTSTREAMFACTORY_HPP)
#define LIBMAUS2_AIO_MEMORYINPUTSTREAMFACTORY_HPP

#include <libmaus2/aio/InputStreamFactory.hpp>
#include <libmaus2/aio/MemoryInputStream.hpp>

namespace libmaus2
{
	namespace aio
	{
		struct MemoryInputStreamFactory : public libmaus2::aio::InputStreamFactory
		{
			typedef MemoryInputStreamFactory this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
			
			virtual ~MemoryInputStreamFactory() {}
			virtual libmaus2::aio::InputStream::unique_ptr_type constructUnique(std::string const & filename)
			{
				libmaus2::util::shared_ptr<std::istream>::type iptr(new MemoryInputStream(filename));
				libmaus2::aio::InputStream::unique_ptr_type istr(new libmaus2::aio::InputStream(iptr));
				return UNIQUE_PTR_MOVE(istr);
			}
			virtual libmaus2::aio::InputStream::shared_ptr_type constructShared(std::string const & filename)
			{
				libmaus2::util::shared_ptr<std::istream>::type iptr(new MemoryInputStream(filename));
				libmaus2::aio::InputStream::shared_ptr_type istr(new libmaus2::aio::InputStream(iptr));
				return istr;
			}
		};
	}
}
#endif
