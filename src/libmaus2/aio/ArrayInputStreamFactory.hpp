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
#if ! defined(LIBMAUS2_AIO_ARRAYINPUTSTREAMFACTORY_HPP)
#define LIBMAUS2_AIO_ARRAYINPUTSTREAMFACTORY_HPP

#include <libmaus2/aio/InputStreamFactory.hpp>
#include <libmaus2/aio/ArrayInputStream.hpp>
#include <libmaus2/aio/ArrayFileContainer.hpp>

namespace libmaus2
{
	namespace aio
	{
		template<typename _iterator>
		struct ArrayInputStreamFactory : public libmaus2::aio::InputStreamFactory
		{
			typedef _iterator iterator;
			typedef ArrayInputStreamFactory<iterator> this_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			libmaus2::aio::ArrayFileContainer<iterator> & container;

			ArrayInputStreamFactory(libmaus2::aio::ArrayFileContainer<iterator> & rcontainer)
			: container(rcontainer)
			{

			}
			virtual ~ArrayInputStreamFactory() {}
			virtual libmaus2::aio::InputStream::unique_ptr_type constructUnique(std::string const & filename)
			{
				libmaus2::util::shared_ptr<std::istream>::type iptr(container.getEntry(filename));
				libmaus2::aio::InputStream::unique_ptr_type istr(new libmaus2::aio::InputStream(iptr));
				return UNIQUE_PTR_MOVE(istr);
			}
			virtual libmaus2::aio::InputStream::shared_ptr_type constructShared(std::string const & filename)
			{
				libmaus2::util::shared_ptr<std::istream>::type iptr(container.getEntry(filename));
				libmaus2::aio::InputStream::shared_ptr_type istr(new libmaus2::aio::InputStream(iptr));
				return istr;
			}
			virtual bool tryOpen(std::string const & fn)
			{
				return container.hasEntry(fn);			}
		};
	}
}
#endif
