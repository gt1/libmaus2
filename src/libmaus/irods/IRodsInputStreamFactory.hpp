/*
    libmaus
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
#if ! defined(LIBMAUS_IRODS_IRODSINPUTSTREAMFACTORY_HPP)
#define LIBMAUS_IRODS_IRODSINPUTSTREAMFACTORY_HPP

#include <libmaus/aio/InputStreamFactory.hpp>
#include <libmaus/irods/IRodsInputStream.hpp>
#include <libmaus/aio/InputStreamFactoryContainer.hpp>

namespace libmaus
{
	namespace irods
	{
		struct IRodsInputStreamFactory : public libmaus::aio::InputStreamFactory
		{
			typedef IRodsInputStreamFactory this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			virtual ~IRodsInputStreamFactory() {}
			virtual libmaus::aio::InputStream::unique_ptr_type construct(std::string const & filename)
			{
				IRodsInputStream::unique_ptr_type tptr(new IRodsInputStream(filename));
				libmaus::util::unique_ptr<std::istream>::type iptr(tptr.release());
				libmaus::aio::InputStream::unique_ptr_type istr(new libmaus::aio::InputStream(iptr));
				return UNIQUE_PTR_MOVE(istr);
			}

			static void registerHandler()
			{
				libmaus::aio::InputStreamFactoryContainer::addHandler(
					"irods",
					libmaus::aio::InputStreamFactory::shared_ptr_type(new libmaus::irods::IRodsInputStreamFactory)
				);
			}
		};
	}
}
#endif
