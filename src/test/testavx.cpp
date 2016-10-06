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
#include <libmaus2/lcs/AlignmentOneAgainstManyAVX2.hpp>

static std::string numString(uint64_t const from, uint64_t const to)
{
	std::string s(to-from,'\0');

	for ( uint64_t i = 0; i < s.size(); ++i )
		s[i] = from+i;

	return s;
}

#include <vector>

int main()
{
	std::vector<std::string> V;
	for ( uint64_t i = 0; i < 5; ++i )
		V.push_back(numString(i*11,(i+1)*11));

	V[0][5] = 7;

	std::string q = numString(0,11);
	uint8_t const * qa = reinterpret_cast<uint8_t const *>(q.c_str());
	uint8_t const * qe = qa + q.size();

	typedef std::pair<uint8_t const *, uint64_t> up;
	libmaus2::autoarray::AutoArray < up > U(V.size());
	for ( uint64_t i = 0; i < V.size(); ++i )
	{
		char const * c = V[i].c_str();
		uint8_t const * u = reinterpret_cast<uint8_t const *>(c);
		U[i] = up(u,V[i].size());
	}

	libmaus2::autoarray::AutoArray<uint64_t> E;
	libmaus2::lcs::AlignmentOneAgainstManyAVX2 aligner;
	aligner.process(qa,qe,U.begin(),U.size(),E);
	for ( uint64_t i = 0; i < U.size(); ++i )
		std::cerr << "U[" << i << "]=" << E[i] << std::endl;

	#if 0
	uint8_t A[] __attribute__((aligned(sizeof(LIBMAUS2_SIMD_WORD_TYPE)))) = {
		0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
		16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
	};
	__m256i i = LIBMAUS2_SIMD_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_SIMD_WORD_TYPE const *>(&A[0]));
	std::cerr << formatRegister(i) << std::endl;
	#endif
}
