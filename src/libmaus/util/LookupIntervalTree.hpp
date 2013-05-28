/*
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
*/
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

			::libmaus::autoarray::AutoArray < ::libmaus::util::IntervalTree const * > createLookup();
			LookupIntervalTree(
				::libmaus::autoarray::AutoArray < std::pair<uint64_t,uint64_t> > const & rH,
				unsigned int const rrangebits,
				unsigned int const rsublookupbits
			);
			
			uint64_t find(uint64_t const v) const
			{
				// start search from lowest common ancestor of all values
				// which have the same length sublookupbits prefix
				return L[ v >> lookupshift ] -> find(v);	
			}
			
			void test(bool setupRandom = true) const;
		};
	}
}
#endif
