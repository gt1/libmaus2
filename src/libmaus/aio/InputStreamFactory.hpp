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
#if ! defined(LIBMAUS_AIO_INPUTSTREAMFACTORY_HPP)
#define LIBMAUS_AIO_INPUTSTREAMFACTORY_HPP

#include <libmaus/aio/InputStream.hpp>

namespace libmaus
{
	namespace aio
	{
		struct InputStreamFactory
		{
			typedef InputStreamFactory this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			virtual ~InputStreamFactory() {}
			virtual InputStream::unique_ptr_type construct(std::string const & filename) = 0;
		};
	}
}
#endif
