/*
    libmaus2
    Copyright (C) 2018 German Tischler-Höhle

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

#include <libmaus2/bambam/BamDecoder.hpp>

int main()
{
	try
	{
		libmaus2::lz::BgzfInflateThreadPoolReader R(std::cin,32);
		libmaus2::bambam::BamDecoderTemplate<libmaus2::lz::BgzfInflateThreadPoolReader> BD(R);

		std::cerr << BD.getHeader().text;

		uint64_t r = 0;
		while ( BD.readAlignment() )
		{
			++r;

			if ( r % (1024*1024) == 0 )
				std::cerr << r << std::endl;
		}

		std::cerr << r << std::endl;

		#if 0
		libmaus2::autoarray::AutoArray<char> C(256*1024*1024,false);

		uint64_t r;
		uint64_t c = 0;
		while ( (r=R.read(C.begin(),C.size())) != 0 )
		{
			c += r;
			std::cerr << "c=" << c << " r=" << r << std::endl;
		}
		#endif
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
