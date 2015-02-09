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
#include <libmaus/irods/IRodsInputStreamFactory.hpp>
#include <libmaus/util/ArgInfo.hpp>

int main(int argc, char * argv[])
{
	try
	{
		libmaus::util::ArgInfo const arginfo(argc,argv);
	
		libmaus::irods::IRodsInputStreamFactory::registerHandler();
		
		libmaus::autoarray::AutoArray<char> B(1024*1024,false);
		for ( uint64_t i = 0; i < arginfo.restargs.size(); ++i )
		{
			libmaus::aio::InputStream::unique_ptr_type Pin(libmaus::aio::InputStreamFactoryContainer::constructUnique(arginfo.restargs[i]));
			std::istream & in = *Pin;
			
			#if 0
			in.seekg(0,std::ios::end);
			uint64_t const len = in.tellg();
			in.seekg(0,std::ios::beg);
			#else
			uint64_t const len = 0;
			#endif
			
			uint64_t t = 0;
			while ( in )
			{
				in.read(B.begin(),B.size());
				t += in.gcount();
				std::cout.write(B.begin(),in.gcount());
				
				if ( len != 0 )
					std::cerr << "\r" << std::string(80,' ') << "\r" << (static_cast<double>(t) / len) << std::flush;
				else
					std::cerr << "\r" << std::string(80,' ') << "\r" << t << std::flush;
			}
			
			if ( len != 0 )
				std::cerr << "\r" << std::string(80,' ') << "\r" << (static_cast<double>(t) / len) << std::endl;
			else
				std::cerr << "\r" << std::string(80,' ') << "\r" << t << std::endl;
		}		
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
