/*
    libmaus2
    Copyright (C) 2015 German Tischler

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
#if ! defined(LIBMAUS2_UTIL_LONGESTINCREASINGSUBSEQUENCEEXTENDEDRESULT_HPP)
#define LIBMAUS2_UTIL_LONGESTINCREASINGSUBSEQUENCEEXTENDEDRESULT_HPP

#include <libmaus2/autoarray/AutoArray.hpp>

namespace libmaus2
{
	namespace util
	{
		template<typename iterator_type, typename comparator_type>
		struct LongestIncreasingSubsequence;

		struct LongestIncreasingSubsequenceExtendedResult
		{
			typedef LongestIncreasingSubsequenceExtendedResult this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			private:
			template<typename,typename>
			friend struct LongestIncreasingSubsequence;

			// next index in longest increasing subsequence
			::libmaus2::autoarray::AutoArray<uint64_t> const P;
			// length of LISS starting at index i
			::libmaus2::autoarray::AutoArray<uint64_t> const L;

			LongestIncreasingSubsequenceExtendedResult(
				::libmaus2::autoarray::AutoArray<uint64_t> & rP,
				::libmaus2::autoarray::AutoArray<uint64_t> & rL
			) : P(rP), L(rL)
			{
				assert ( P.size() == L.size() );
			}

			public:
			size_t size() const
			{
				return P.size();
			}

			uint64_t getLength(uint64_t const i) const
			{
				assert ( i < size() );
				return L[i];
			}

			uint64_t getMaximumLength() const
			{
				if ( L.size() )
					return *std::max_element<uint64_t const *>(L.begin(),L.end());
				else
					return 0;
			}

			std::pair<uint64_t,uint64_t> getMaximumLengthAndInstance() const
			{
				if ( L.size() )
				{
					uint64_t const * it = std::max_element<uint64_t const *>(L.begin(),L.end());
					return std::pair<uint64_t,uint64_t>(*it,it-L.begin());
				}
				else
				{
					return std::pair<uint64_t,uint64_t>(0,0);
				}
			}

			bool hasNext(uint64_t const i) const
			{
				return P[i] != i;
			}

			uint64_t getNext(uint64_t const i) const
			{
				return P[i];
			}
		};

		std::ostream & operator<<(std::ostream & out, LongestIncreasingSubsequenceExtendedResult const & R);
	}
}
#endif
