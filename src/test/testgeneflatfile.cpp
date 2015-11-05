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

#include <libmaus2/aio/PosixFdInputStream.hpp>
#include <libmaus2/bambam/GeneFlatFile.hpp>
#include <libmaus2/util/ArgInfo.hpp>

int main(int argc, char const * argv[])
{
	try
	{
		libmaus2::util::ArgInfo const arginfo(argc,argv);
		std::string const fn = arginfo.getUnparsedRestArg(0);

		libmaus2::bambam::GeneFlatFile::unique_ptr_type GFF(libmaus2::bambam::GeneFlatFile::construct(fn));

		std::cout << *GFF;
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
