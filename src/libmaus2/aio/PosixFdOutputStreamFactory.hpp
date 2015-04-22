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
#if ! defined(LIBMAUS2_AIO_POSIXFDOUTPUTSTREAMFACTORY_HPP)
#define LIBMAUS2_AIO_POSIXFDOUTPUTSTREAMFACTORY_HPP

#include <libmaus2/aio/OutputStreamFactory.hpp>
#include <libmaus2/aio/PosixFdOutputStream.hpp>

namespace libmaus2
{
	namespace aio
	{
		struct PosixFdOutputStreamFactory : public libmaus2::aio::OutputStreamFactory
		{
			typedef PosixFdOutputStreamFactory this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
			
			virtual ~PosixFdOutputStreamFactory() {}
			virtual libmaus2::aio::OutputStream::unique_ptr_type constructUnique(std::string const & filename)
			{
				if ( filename == "-" )
				{				
					libmaus2::util::shared_ptr<std::ostream>::type iptr(new PosixFdOutputStream(STDOUT_FILENO));
					libmaus2::aio::OutputStream::unique_ptr_type istr(new libmaus2::aio::OutputStream(iptr));
					return UNIQUE_PTR_MOVE(istr);
				}
				else
				{
					libmaus2::util::shared_ptr<std::ostream>::type iptr(new PosixFdOutputStream(filename));
					libmaus2::aio::OutputStream::unique_ptr_type istr(new libmaus2::aio::OutputStream(iptr));
					return UNIQUE_PTR_MOVE(istr);
				}
			}
			virtual libmaus2::aio::OutputStream::shared_ptr_type constructShared(std::string const & filename)
			{
				if ( filename == "-" )
				{				
					libmaus2::util::shared_ptr<std::ostream>::type iptr(new PosixFdOutputStream(STDOUT_FILENO));
					libmaus2::aio::OutputStream::shared_ptr_type istr(new libmaus2::aio::OutputStream(iptr));
					return istr;
				}
				else
				{
					libmaus2::util::shared_ptr<std::ostream>::type iptr(new PosixFdOutputStream(filename));
					libmaus2::aio::OutputStream::shared_ptr_type istr(new libmaus2::aio::OutputStream(iptr));
					return istr;
				}
			}
		};
	}
}
#endif
