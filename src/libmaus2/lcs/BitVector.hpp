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
#if ! defined(LIBMAUS2_LCS_BITVECTOR_HPP)
#define LIBMAUS2_LCS_BITVECTOR_HPP

#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/lcs/BitVectorResultCallbackInterface.hpp>

namespace libmaus2
{
	namespace lcs
	{
		template<typename _word_type>
		struct BitVector
		{
			typedef _word_type word_type;

			static word_type lowbits(unsigned int m)
			{
				if ( ! m )
				{
					return 0;
				}
				else if ( m < 8*sizeof(word_type) )
				{
					return (static_cast<word_type>(1)<<m)-1;
				}
				else
				{
					return (lowbits((m+1)/2) << (m/2)) | lowbits(m/2);
				}
			}

			template<typename iterator_t, typename iterator_q>
			static void bitvector(
				iterator_t t, size_t n,
				iterator_q q, size_t m,
				BitVectorResultCallbackInterface & callback,
				unsigned int const maxk,
				size_t const alphabetsize =
					static_cast<size_t>(
						std::max(
							static_cast<int64_t>(std::numeric_limits< typename std::iterator_traits<iterator_t>::value_type >::max()),
							static_cast<int64_t>(std::numeric_limits< typename std::iterator_traits<iterator_q>::value_type >::max())
						)
					)+1
			)
			{
				// an empty pattern would match infinitely at each position, which we consider as an error
				if ( ! m )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "libmaus2::lcs::BitVector::bitvector: empty query" << std::endl;
					lme.finish();
					throw lme;
				}

				#if defined(_MSC_VER) || defined(__MINGW32__)
				word_type * const B = reinterpret_cast<uint64_t *>(_alloca( alphabetsize * sizeof(word_type) ));
				#else
				word_type * const B = reinterpret_cast<uint64_t *>(alloca( alphabetsize * sizeof(word_type) ));
				#endif

				std::fill(B,B+alphabetsize,0);

				static word_type const one = static_cast<word_type>(1);
				for ( size_t i = 0; i < m; ++i )
					B[ static_cast<int>(q[i]) ] |= one << i;

				word_type VP = lowbits(m);
				word_type VN = 0;

				int64_t score = m;

				word_type const topbit = one << (m-1);

				for ( size_t i = 0; i < n; ++i )
				{
					word_type const X = B[static_cast<int>(t[i])] | VN;
					word_type const D0 = ((VP + (X & VP)) ^ VP) | X;
					word_type const HN = VP & D0;
					word_type const HP = VN | ~(VP|D0);
					word_type const Y = HP << 1;
					VN = Y & D0;
					VP = (HN << 1) | ~(Y|D0);

					if ( HP & topbit )
						score += 1;
					else if ( HN & topbit )
						score -= 1;

					if ( score <= maxk )
						callback(i,score);
				}
			}
		};
	}
}
#endif
