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
#if ! defined(LIBMAUS_UTIL_LOOKUPINTERVALTREE_HPP)
#define LIBMAUS_UTIL_LOOKUPINTERVALTREE_HPP

#include <libmaus/util/IntervalTree.hpp>
#include <libmaus/random/Random.hpp>

namespace libmaus
{
	namespace util
	{
		struct LookupIntervalTree
		{
			typedef LookupIntervalTree this_type;

			::libmaus::autoarray::AutoArray < std::pair<uint64_t,uint64_t> > const H;
			::libmaus::util::IntervalTree const I;
			unsigned int const rangebits;
			unsigned int const sublookupbits;
			::libmaus::autoarray::AutoArray < ::libmaus::util::IntervalTree const * > const L;
			unsigned int const lookupshift;

			::libmaus::autoarray::AutoArray < ::libmaus::util::IntervalTree const * > createLookup()
			{
				::libmaus::autoarray::AutoArray < ::libmaus::util::IntervalTree const * > L(1ull << sublookupbits,false);

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
			
			LookupIntervalTree(
				::libmaus::autoarray::AutoArray < std::pair<uint64_t,uint64_t> > const & rH,
				unsigned int const rrangebits,
				unsigned int const rsublookupbits
			) : H(rH.clone()), I(H,0,H.size()), rangebits(rrangebits), sublookupbits(rsublookupbits),
			    L(createLookup()), lookupshift ( rangebits - sublookupbits )
			{

			}
			
			uint64_t find(uint64_t const v) const
			{
				// start search from lowest common ancestor of all values
				// which have the same length sublookupbits prefix
				return L[ v >> lookupshift ] -> find(v);	
			}
			
			void test(bool setupRandom = true) const
			{
				for ( uint64_t i = 0; i < H.size(); ++i )
				{
					assert (  find ( H[i].first ) == I.find(H[i].first) );
					assert (  find ( H[i].second-1 ) == I.find(H[i].second-1) );
				}
				
				if ( setupRandom )
					::libmaus::random::Random::setup();
				
				for ( uint64_t i = 0; i < 64*1024; ++i )
				{
					uint64_t const v = ::libmaus::random::Random::rand64() & ((1ull << (rangebits))-1);
					assert ( find(v) == I.find(v) );
				}
			}
		};
	}
}
#endif
