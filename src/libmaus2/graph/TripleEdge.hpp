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

#if ! defined(TRIPLEEDGE_HPP)
#define TRIPLEEDGE_HPP

#include <libmaus2/types/types.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/random/Random.hpp>

namespace libmaus2
{
	namespace graph
	{
		struct TripleEdge
		{
			typedef TripleEdge this_type;
			typedef uint64_t node_id_type;
			typedef uint32_t link_weight_type;

			node_id_type a;
			node_id_type b;
			link_weight_type c;

			TripleEdge() : a(0), b(0), c(0) {}
			TripleEdge(node_id_type const ra, node_id_type const rb)
			: a(ra), b(rb), c(1) {}
			TripleEdge(node_id_type const ra, node_id_type const rb, link_weight_type const rc)
			: a(ra), b(rb), c(rc) {}

			bool operator==(TripleEdge const & o) const
			{
				return 
					(o.a == a)
					&&
					(o.b == b)
					&&
					(o.c == c);
			}

			bool operator!=(TripleEdge const & o) const
			{
				return !(*this == o);
			}

			bool operator<(TripleEdge const & o) const
			{
				if ( a != o.a )
					return a < o.a;
				if ( b != o.b )
					return b < o.b;
				if ( c != o.c )
					return c < o.c;
				return false;
			}
			
			static TripleEdge random()
			{
				return TripleEdge ( 
					::libmaus2::random::Random::rand64() % 8,
					::libmaus2::random::Random::rand64() % 8,
					::libmaus2::random::Random::rand16() % 4
				);
			}
			
			static ::libmaus2::autoarray::AutoArray<this_type> randomArray(uint64_t const n)
			{
				::libmaus2::autoarray::AutoArray<this_type> A(n,false);
				for ( uint64_t i = 0; i < n; ++i )
					A[i] = random();
				return A;
			}
			static ::libmaus2::autoarray::AutoArray<this_type> randomValidArray(uint64_t const n)
			{
				::libmaus2::autoarray::AutoArray<this_type> A(n,false);
				for ( uint64_t i = 0; i < n; ++i )
				{
					A[i] = random();
					
					if ( A[i].a > A[i].b )
						std::swap( A[i].a, A[i].b );
					if ( A[i].a == A[i].b )
						A[i].b ++;
				}
				return A;
			}
		};
		
		inline ::std::ostream & operator<<(::std::ostream & out, TripleEdge const & TE)
		{
			out << "TripleEdge(" << TE.a << "," << TE.b << "," << TE.c << ")";
			return out;
		}
	}
}
#endif
