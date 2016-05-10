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
#if ! defined(LIBMAUS2_HUFFMAN_LFRANKPOS_HPP)
#define LIBMAUS2_HUFFMAN_LFRANKPOS_HPP

#include <libmaus2/types/types.hpp>
#include <ostream>

namespace libmaus2
{
	namespace huffman
	{
		struct LFRankPos
		{
			uint64_t r;
			uint64_t p;
			uint64_t n;
			uint64_t * v;
			bool active;

			LFRankPos(
				int64_t rr = 0,
				uint64_t rp = 0,
				uint64_t rn = 0,
				uint64_t * rv = 0,
				bool ractive = false
			) : r(rr), p(rp), n(rn), v(rv), active(ractive)
			{

			}

			bool operator==(LFRankPos const & o) const
			{
				if ( r != o.r )
					return false;
				if ( p != o.p )
					return false;
				if ( n != o.n )
					return false;
				if ( active != o.active )
					return false;
				for ( uint64_t i = 0; i < n; ++i )
					if ( v[i] != o.v[i] )
						return false;
				return true;
			}
		};

		inline std::ostream & operator<<(std::ostream & out, LFRankPos const & L)
		{
			out << "LFRankPos(r=" << L.r << ",p=" << L.p << ",n=" << L.n << ",v={";
			for ( uint64_t i = 0; i < L.n; ++i )
				out << L.v[i] << ((i+1<L.n)?",":"");
			out << "},active=" << L.active << ")";
			return out;
		}
	}
}
#endif
