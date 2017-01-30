/*
    libmaus2
    Copyright (C) 2017 German Tischler

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
#include <libmaus2/aio/PosixFdOutputStreamFactory.hpp>
#include <libmaus2/aio/PosixFdInputStream.hpp>
#include <libmaus2/aio/PosixFdOutputStream.hpp>

void libmaus2::aio::PosixFdOutputStreamFactory::copy(std::string const & from, std::string const & to)
{
	libmaus2::aio::PosixFdInputStream in(from);
	libmaus2::aio::PosixFdOutputStream out(to);
	libmaus2::autoarray::AutoArray<char> B(64*1024);

	while ( in )
	{
		in.read(B.begin(),B.size());
		ssize_t got = in.gcount();
		out.write(B.begin(),got);

		if ( ! out )
		{
			libmaus2::exception::LibMausException lme;
			lme.getStream() << "libmaus2::aio::PosixFdOutputStreamFactory::copy(" << from << "," << to << "): output failed" << std::endl;
			lme.finish();
			throw lme;
		}
	}

	if ( in.bad() )
	{
		libmaus2::exception::LibMausException lme;
		lme.getStream() << "libmaus2::aio::PosixFdOutputStreamFactory::copy(" << from << "," << to << "): input failed" << std::endl;
		lme.finish();
		throw lme;

	}

}
