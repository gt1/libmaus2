/*
    libmaus2
    Copyright (C) 2016 German Tischler

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
#include <libmaus2/util/HugePageAllocator.hpp>

int main()
{
	try
	{
		libmaus2::util::HugePages & HP = libmaus2::util::HugePages::getHugePageObject();

		// nothing allocated yet, produce empty line
		HP.print(std::cerr);
		std::cerr << std::endl;

		void * block0 = HP.malloc(1024);

		HP.print(std::cerr);
		std::cerr << std::endl;

		void * block1 = HP.malloc(1024);

		HP.print(std::cerr);
		std::cerr << std::endl;

		HP.free(block0);

		HP.print(std::cerr);
		std::cerr << std::endl;

		void * block2 = HP.malloc(1024);

		HP.print(std::cerr);
		std::cerr << std::endl;

		HP.free(block1);

		HP.print(std::cerr);
		std::cerr << std::endl;

		HP.free(block2);

		HP.print(std::cerr);
		std::cerr << std::endl;

		typedef uint64_t key_type;
		typedef uint64_t value_type;
		typedef libmaus2::util::HugePageAllocator< std::pair<key_type const, value_type> > allocator_type;
		std::map < uint64_t, uint64_t, std::less<uint64_t>, allocator_type > M;

		M [ 5 ] = 7;
		M [ 7 ] = 5;

		M.erase(5);

		HP.print(std::cerr);
		std::cerr << std::endl;

		M.erase(7);

		HP.print(std::cerr);
		std::cerr << std::endl;
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
