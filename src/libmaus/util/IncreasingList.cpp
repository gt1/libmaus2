/**
    libmaus
    Copyright (C) 2009-2013 German Tischler
    Copyright (C) 2011-2013 Genome Research Limited

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
**/

#include <libmaus/util/IncreasingList.hpp>

uint64_t libmaus::util::IncreasingList::byteSize() const
{
	return 6*sizeof(uint64_t) + Bup.byteSize() + C.byteSize();
}
			
void libmaus::util::IncreasingList::setup()
{
	R = UNIQUE_PTR_MOVE(::libmaus::rank::ERank222B::unique_ptr_type(new ::libmaus::rank::ERank222B(Bup.get(),Bup.size()*64)));
}

libmaus::util::IncreasingList::IncreasingList(uint64_t const rn, uint64_t const rb)
: n(rn), b(rb), m(::libmaus::math::lowbits(b)), C(n,b), 
  Bup( (((n*m) >> b) + n + 63) / 64 )
{
}

void libmaus::util::IncreasingList::test(std::vector<uint64_t> const & W)
{
	if ( W.size() )
	{
		std::vector<uint64_t> V = W;
		std::sort(V.begin(),V.end());
		uint64_t const U = V.back() + 1;
		IncreasingList IL(V.size(),U);
		
		for ( uint64_t i = 0; i < V.size(); ++i )
			IL.put(i,V[i]);
		
		IL.setup();
		
		for ( uint64_t i = 0; i < V.size(); ++i )
		{
			assert ( V[i] == IL[i] );
			// std::cerr << V[i] << "\t" << IL[i] << std::endl;
		}
	}
}

void libmaus::util::IncreasingList::testRandom(uint64_t const n, uint64_t const k)
{
	std::vector < uint64_t > V(n);
	for ( uint64_t i = 0; i < n; ++i )
		V[i] = ::libmaus::random::Random::rand64() % k;
	test(V);
}
