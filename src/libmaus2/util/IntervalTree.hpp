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

#if ! defined(INTERVALTREE_HPP)
#define INTERVALTREE_HPP

#include <libmaus2/types/types.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/bitio/BitVector.hpp>
#include <vector>
#include <cassert>

namespace libmaus2
{
	namespace util
	{
		struct IntervalTree
		{
			typedef IntervalTree this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			unique_ptr_type leftchild;
			unique_ptr_type rightchild;

			uint64_t split;

			bool isLeaf() const;

			IntervalTree(
				::libmaus2::autoarray::AutoArray < std::pair<uint64_t,uint64_t> > const & H,
				uint64_t const ileft,
				uint64_t const iright,
				bool const check = true
			);
			~IntervalTree();

			uint64_t findTrace(std::vector < IntervalTree const * > & trace, uint64_t const v) const;
			IntervalTree const * lca(uint64_t const v, uint64_t const w) const;
			uint64_t find(uint64_t const v) const;
			uint64_t getNumLeafs() const;
			std::ostream & flatten(std::ostream & ostr, uint64_t depth = 0) const;

		};

		std::ostream & operator<<(std::ostream & out, IntervalTree const & I);
	}
}
#endif
