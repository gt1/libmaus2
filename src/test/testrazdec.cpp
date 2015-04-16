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

#include <libmaus2/lz/RAZFDecoder.hpp>
#include <libmaus2/util/ArgInfo.hpp>

int main(int argc, char * argv[])
{
	try
	{
		libmaus2::util::ArgInfo const arginfo(argc,argv);
		
		std::string const fn = arginfo.getRestArg<std::string>(0);
		std::string const fnu = arginfo.getRestArg<std::string>(1);
	
		#if 0
		libmaus2::lz::RAZFIndex index(fn);		
		std::cerr << index;
		#endif
		
		libmaus2::lz::RAZFDecoder dec(fn);
		
		libmaus2::autoarray::AutoArray<char> A = libmaus2::autoarray::AutoArray<char>::readFile(fnu);

		for ( uint64_t i = 0; i <= A.size(); i += 1024 )
		{
			if ( i % 1024 == 0 )
				std::cerr << "i=" << i << std::endl;

			dec.clear();				
			dec.seekg(i);
			int c;
			uint64_t j = i;
			while ( (c=dec.get()) != std::istream::traits_type::eof() )
				assert ( c == static_cast<uint8_t>(A[j++]) );
		}
	
		return EXIT_SUCCESS;
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
