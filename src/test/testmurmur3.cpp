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
#include <libmaus2/digest/MurmurHash3_x64_128.hpp>
#include <libmaus2/hashing/MurmurHash3.h>

void test(std::string const & s)
{
	uint8_t const * data = reinterpret_cast<uint8_t const *>(s.c_str());
	uint64_t const n = s.size();

	uint8_t digestA[16];
	uint8_t digestB[16];
	uint8_t digestC[16];

	libmaus2::digest::MurmurHash3_x64_128 context;
	context.init();
	context.update(data,n);
	context.digest(&digestA[0]);
	std::string const refdigest = context.digestToString(&digestA[0]);
	std::cout << refdigest << std::endl;

	uint64_t out[2];
	libmaus2::hashing::MurmurHash3_x64_128(data,n,context.getDefaultSeed(),&out[0]);

	context.wordsToDigest(out[0],out[1],&digestB[0]);
	assert ( context.digestToString(&digestB[0]) == refdigest );

	context.init();
	for ( uint64_t i = 0; i < n; ++i )
		context.update(data+i,1);
	context.digest(&digestC[0]);

	assert ( context.digestToString(&digestC[0]) == refdigest );

	for ( uint64_t step = 1; step <= 128; ++step )
	{
		uint8_t digestD[16];
		std::fill(&digestD[0],&digestD[16],0);

		context.init();
		uint64_t i = 0;
		while ( i < n )
		{
			uint64_t const tocopy = std::min(step,n-i);
			context.update(data + i, tocopy);
			i += tocopy;
		}
		context.digest(&digestD[0]);

		// std::cout << context.digestToString(&digestD[0]) << std::endl;
		assert ( context.digestToString(&digestD[0]) == refdigest );
	}
}

int main()
{
	test("Hello world");

	libmaus2::autoarray::AutoArray<char> const A = libmaus2::autoarray::AutoArray<char>::readFile("configure");
	std::string const S(A.begin(),A.end());
	test(S);
}
