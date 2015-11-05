/*
    libmaus2
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
*/

#include <libmaus2/util/LookupIntervalTree.hpp>

::libmaus2::autoarray::AutoArray < ::libmaus2::util::IntervalTree const * > libmaus2::util::LookupIntervalTree::createLookup()
{
	::libmaus2::autoarray::AutoArray < ::libmaus2::util::IntervalTree const * > L(1ull << sublookupbits,false);

	for ( uint64_t i = 0; i < (1ull << sublookupbits); ++i )
	{
		uint64_t const low = i << (rangebits-sublookupbits);
		uint64_t const high = low | ((1ull << (rangebits-sublookupbits))-1ull);
		L[i] = I.lca(low,high);

		#if 0
		std::cerr << std::hex << low << "\t" << high << std::dec
			<< "\t" << L[i]->isLeaf() << std::endl;
		#endif
	}

	return L;
}

libmaus2::util::LookupIntervalTree::LookupIntervalTree(
	::libmaus2::autoarray::AutoArray < std::pair<uint64_t,uint64_t> > const & rH,
	unsigned int const rrangebits,
	unsigned int const rsublookupbits
) : H(rH.clone()), I(H,0,H.size()), rangebits(rrangebits), sublookupbits(rsublookupbits),
    L(createLookup()), lookupshift ( rangebits - sublookupbits )
{

}

void libmaus2::util::LookupIntervalTree::test(bool setupRandom) const
{
	for ( uint64_t i = 0; i < H.size(); ++i )
	{
		assert (  find ( H[i].first ) == I.find(H[i].first) );
		assert (  find ( H[i].second-1 ) == I.find(H[i].second-1) );
	}

	if ( setupRandom )
		::libmaus2::random::Random::setup();

	for ( uint64_t i = 0; i < 64*1024; ++i )
	{
		uint64_t const v = ::libmaus2::random::Random::rand64() & ((1ull << (rangebits))-1);
		assert ( find(v) == I.find(v) );
	}
}
