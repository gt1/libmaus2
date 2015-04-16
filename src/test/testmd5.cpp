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
#include <libmaus2/digest/md5.hpp>
#include <libmaus2/util/GetFileSize.hpp>
#include <libmaus2/util/ArgInfo.hpp>

int main(int argc, char * argv[])
{
	try
	{
		libmaus2::util::ArgInfo const arginfo(argc,argv);
	
		for ( uint64_t i = 0; i < arginfo.restargs.size(); ++i )
		{
			::libmaus2::autoarray::AutoArray<uint8_t> const A = libmaus2::util::GetFileSize::readFile<uint8_t>(arginfo.restargs.at(i));
			std::string const input(A.begin(),A.end());
			std::string output;
			libmaus2::util::MD5::md5(input,output);
			std::cout << arginfo.restargs[i] << "\t" << output << "\t" << std::hex << libmaus2::util::MD5::md5(A.begin(), A.size()) << std::dec << std::endl;
		}
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
