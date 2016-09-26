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
#if ! defined(LIBMAUS2_SV_SV_HPP)
#define LIBMAUS2_SV_SV_HPP

#include <libmaus2/autoarray/AutoArray.hpp>

namespace libmaus2
{
	namespace sv
	{
		struct NSV
		{
			/**
			 * algorithm nsv
			 *
			 * NEXT[r] : smallest rank s such that s>r and SUF[s]<SUF[r]
			 **/
			template<typename iterator>
			static ::libmaus2::autoarray::AutoArray< typename std::iterator_traits<iterator>::value_type > nsv(iterator SUF, uint64_t const n)
			{
				typedef typename std::iterator_traits<iterator>::value_type value_type;

				// allocate result array
				::libmaus2::autoarray::AutoArray<value_type> next(n,false);

				// do not crash for n==0
				if ( n )
				{
					// initialize next for rank n-1
					next[n-1] = n;
				}

				// compute next for the remaining ranks
				for ( int64_t r = static_cast<int64_t>(n)-2; r >= 0; --r )
				{
					uint64_t t = static_cast<uint64_t>(r) + 1;

					while ( (t < n) && (SUF[t] >= SUF[r]) )
						t = next[t];
					next[r] = t;
				}

				// return result
				return next;
			}

			/**
			 * algorithm nsv
			 *
			 * NEXT[r] : smallest rank s such that s>r and SUF[s]<SUF[r]
			 **/
			template<typename iterator>
			static void nsv(iterator SUF, uint64_t const n, ::libmaus2::autoarray::AutoArray< typename std::iterator_traits<iterator>::value_type > & next)
			{
				// allocate result array
				next.ensureSize(n);

				// do not crash for n==0
				if ( n )
				{
					// initialize next for rank n-1
					next[n-1] = n;
				}

				// compute next for the remaining ranks
				for ( int64_t r = static_cast<int64_t>(n)-2; r >= 0; --r )
				{
					uint64_t t = static_cast<uint64_t>(r) + 1;

					while ( (t < n) && (SUF[t] >= SUF[r]) )
						t = next[t];
					next[r] = t;
				}
			}
		};
	}
}

namespace libmaus2
{
	namespace sv
	{
		struct PSV
		{
			/**
			 * algorithm PREVonly
			 *
			 * PREV[r] : largest rank s such that s<r and SUF[s]<SUF[r]
			 **/
			template<typename iterator>
			static ::libmaus2::autoarray::AutoArray< typename std::iterator_traits<iterator>::value_type > psv(iterator SUF, uint64_t const n)
			{
				typedef typename std::iterator_traits<iterator>::value_type value_type;

				// allocate result array
				::libmaus2::autoarray::AutoArray<value_type> prev(n,false);

				// do not crash for n==0
				if ( n )
					prev[0] = 0;

				for ( uint64_t r = 1; r < n; ++r )
				{
					int64_t t = r - 1;

					while ( (t > 0) && (SUF[t] >= SUF[r]) )
						t = prev[t];

					prev[r] = t;
				}

				// return result
				return prev;
			}

			/**
			 * algorithm PREVonly
			 *
			 * PREV[r] : largest rank s such that s<r and SUF[s]<SUF[r]
			 **/
			template<typename iterator>
			static void psv(iterator SUF, uint64_t const n, ::libmaus2::autoarray::AutoArray< typename std::iterator_traits<iterator>::value_type > & prev)
			{
				// allocate result array
				prev.ensureSize(n);

				// do not crash for n==0
				if ( n )
					prev[0] = n;

				for ( uint64_t r = 1; r < n; ++r )
				{
					int64_t t = r - 1;

					while ( (t != n) && (SUF[t] >= SUF[r]) )
						t = prev[t];

					prev[r] = t;
				}
			}
		};
	}
}
#endif
