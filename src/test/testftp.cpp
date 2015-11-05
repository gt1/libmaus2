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
#include <libmaus2/util/ArgInfo.hpp>
#include <libmaus2/network/FtpSocket.hpp>
#include <libmaus2/lz/BufferedGzipStream.hpp>

int main(int argc, char * argv[])
{
	try
	{
		libmaus2::util::ArgInfo const arginfo(argc,argv);
		std::string const url = arginfo.getRestArg<std::string>(0);
		int const gz = arginfo.getValue<int>("gz",0);
		libmaus2::network::FtpSocket ftpsock(url,0,true);
		std::istream & ftpstream = ftpsock.getStream();
		std::istream * in = &ftpstream;

		libmaus2::lz::BufferedGzipStream::unique_ptr_type gzptr;
		if ( gz )
		{
			libmaus2::lz::BufferedGzipStream::unique_ptr_type tptr(new libmaus2::lz::BufferedGzipStream(ftpstream));
			gzptr = UNIQUE_PTR_MOVE(tptr);
			in = gzptr.get();
		}

		libmaus2::autoarray::AutoArray<char> A(1024*1024,false);
		uint64_t r = 0;
		while ( *in )
		{
			in->read(A.begin(),A.size());
			uint64_t const gcnt = in->gcount();
			std::cout.write(A.begin(),gcnt);
			r += gcnt;

			std::cerr << "\r" << std::string(80,' ') << "\r" << "[V] " << r/(1024*1024);
		}

		std::cout.flush();
		std::cerr << "\r" << std::string(80,' ') << "\r" << "[V] " << r/(1024*1024) << "\t" << r << "\t" << ftpsock.size << "\n";
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
