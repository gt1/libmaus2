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

#if ! defined(LONGESTINCREASINGSUBSEQUENCE_HPP)
#define LONGESTINCREASINGSUBSEQUENCE_HPP

#include <libmaus2/types/types.hpp>

namespace libmaus2
{
	namespace util
	{
		template<
			typename iterator_type, 
			typename comparator_type = std::less< typename std::iterator_traits<iterator_type>::value_type >
		>
		struct LongestIncreasingSubsequence
		{
			static std::vector<uint64_t> longestIncreasingSubsequence(
				iterator_type X, uint64_t const n,
				comparator_type const & comparator = comparator_type()
			)
			{
				std::vector<uint64_t> M;
				// previous index in longest increasing subsequence
				::libmaus2::autoarray::AutoArray<uint64_t> P(n,false);

				// length of longest increasing subsequence ending at i
				std::vector<uint64_t> LA;
				// histogram over LA (shifted by 1, as value 0 is not possible)
				::libmaus2::autoarray::AutoArray<uint64_t> L(n,false);

				for ( uint64_t i = 0; i < n; ++i )
				{
					// find largest j such that X[M[j]] < X[i]		
					int64_t l = 0, r = M.size();
					
					// if M is empty or element at index i is new minimum
					if ( (! M.size()) || (! ( comparator(X[M[0]],X[i]) )) )
						l = -1;
					else
						// binary search
						while ( r-l > 1 )
						{
							int64_t const m = (r+l) >> 1;
							if ( comparator(X[M[m]],X[i]) )
								l = m;
							else
								r = m;
						}
					
					// #define LONGESTINCREASINGDEBUG
					#if ! defined(LONGESTINCREASINGDEBUG)
					int64_t const j = l;
					#else
					int64_t j = -1;
					for ( uint64_t k = 0; k < M.size(); ++k )
						if ( comparator(X[M[k]],X[i]) )
							j = k;

					assert ( l == j );
					#endif
					
					// set length of liss ending at index i
					L [ i ] = static_cast<uint64_t>(j+2);
					
					if ( static_cast<uint64_t>(j+1) < M.size() )
					{
						M[j+1] = i;
						LA[j+1]++;
					}
					else
					{
						M.push_back(i);
						LA.push_back(1);
					}

					if ( j >= 0 )
						P[i] = M[j];	
					else
						P[i] = i;
				}
				
				if ( M.size() )
				{
					// fill in one liss (by indexes (not value) of elements in X)
					uint64_t m = M.back();

					for ( int64_t i = static_cast<int64_t>(M.size())-1; i >= 0; --i )
					{
						M [ i ] = m;
						m = P[m];
					}				
				}
				
				#if 0
				for ( uint64_t i = 0; i < LA.size(); ++i )
				{
					std::cerr << "LA[" << i << "]=" << LA[i] << std::endl;
				}
				#endif

				return M;
			}
		};
	}
}
#endif
