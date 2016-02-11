/*
    libmaus2
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2015 Genome Research Limited

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
#if ! defined(LIBMAUS2_BAMBAM_SEQCHKSUMPRIMENUMBERS_HPP)
#define LIBMAUS2_BAMBAM_SEQCHKSUMPRIMENUMBERS_HPP

#include <libmaus2/math/UnsignedInteger.hpp>

namespace libmaus2
{
	namespace bambam
	{
		struct SeqChksumPrimeNumbers
		{
			template<size_t k>
			static libmaus2::math::UnsignedInteger<k> getMersenne31()
			{
				return libmaus2::math::UnsignedInteger<k>(0x7FFFFFFFull);
			}

			template<size_t k>
			static libmaus2::math::UnsignedInteger<k> getMersenne521()
			{
				libmaus2::math::UnsignedInteger< (521 + 31)/32 > M;

				M[0] = 1;
				M <<= 521;
				M -= 1;

				return libmaus2::math::UnsignedInteger<k>(M);
			}

			template<size_t k>
			static libmaus2::math::UnsignedInteger<k> getPrime64()
			{
				libmaus2::math::UnsignedInteger<k> M;
				if ( 0 < k )
					M[0] = 0xFFFFFFFFUL - 59UL;
				if ( 1 < k )
					M[1] = 0xFFFFFFFFUL;
				return M; // 2^64-59 prime according to https://primes.utm.edu/lists/2small/0bit.html
			}

			template<size_t k>
			static libmaus2::math::UnsignedInteger<k> getPrime96()
			{
				libmaus2::math::UnsignedInteger<k> M;
				if ( 0 < k )
					M[0] = 0xFFFFFFFFUL - 17;
				if ( 1 < k )
					M[1] = 0xFFFFFFFFUL;
				if ( 2 < k )
					M[2] = 0xFFFFFFFFUL;
				return M; // 2^96-17 prime according to https://primes.utm.edu/lists/2small/0bit.html
			}

			template<size_t k>
			static libmaus2::math::UnsignedInteger<k> getPrime128()
			{
				libmaus2::math::UnsignedInteger<k> M;
				if ( 0 < k )
					M[0] = 0xFFFFFFFFUL - 159;
				if ( 1 < k )
					M[1] = 0xFFFFFFFFUL;
				if ( 2 < k )
					M[2] = 0xFFFFFFFFUL;
				if ( 3 < k )
					M[3] = 0xFFFFFFFFUL;
				return M; // 2^128-159 prime according to https://primes.utm.edu/lists/2small/100bit.html
			}

			template<size_t k>
			static libmaus2::math::UnsignedInteger<k> getPrime160()
			{
				libmaus2::math::UnsignedInteger<k> M;
				if ( 0 < k )
					M[0] = 0xFFFFFFFFUL - 47;
				if ( 1 < k )
					M[1] = 0xFFFFFFFFUL;
				if ( 2 < k )
					M[2] = 0xFFFFFFFFUL;
				if ( 3 < k )
					M[3] = 0xFFFFFFFFUL;
				if ( 4 < k )
					M[4] = 0xFFFFFFFFUL;
				return M; // 2^160-47 prime according to https://primes.utm.edu/lists/2small/100bit.html
			}

			template<size_t k>
			static libmaus2::math::UnsignedInteger<k> getPrime192()
			{
				libmaus2::math::UnsignedInteger<k> M;
				if ( 0 < k )
					M[0] = 0xFFFFFFFFUL - 237;
				if ( 1 < k )
					M[1] = 0xFFFFFFFFUL;
				if ( 2 < k )
					M[2] = 0xFFFFFFFFUL;
				if ( 3 < k )
					M[3] = 0xFFFFFFFFUL;
				if ( 4 < k )
					M[4] = 0xFFFFFFFFUL;
				if ( 5 < k )
					M[5] = 0xFFFFFFFFUL;
				return M; // 2^192-237 prime according to https://primes.utm.edu/lists/2small/100bit.html
			}

			template<size_t k>
			static libmaus2::math::UnsignedInteger<k> getPrime224()
			{
				libmaus2::math::UnsignedInteger<k> M;
				if ( 0 < k )
					M[0] = 0xFFFFFFFFUL - 63;
				if ( 1 < k )
					M[1] = 0xFFFFFFFFUL;
				if ( 2 < k )
					M[2] = 0xFFFFFFFFUL;
				if ( 3 < k )
					M[3] = 0xFFFFFFFFUL;
				if ( 4 < k )
					M[4] = 0xFFFFFFFFUL;
				if ( 5 < k )
					M[5] = 0xFFFFFFFFUL;
				if ( 6 < k )
					M[6] = 0xFFFFFFFFUL;
				return M; // 2^224-63 prime according to https://primes.utm.edu/lists/2small/200bit.html
			}

			template<size_t k>
			static libmaus2::math::UnsignedInteger<k> getPrime256()
			{
				libmaus2::math::UnsignedInteger<k> M;
				if ( 0 < k )
					M[0] = 0xFFFFFFFFUL - 189;
				if ( 1 < k )
					M[1] = 0xFFFFFFFFUL;
				if ( 2 < k )
					M[2] = 0xFFFFFFFFUL;
				if ( 3 < k )
					M[3] = 0xFFFFFFFFUL;
				if ( 4 < k )
					M[4] = 0xFFFFFFFFUL;
				if ( 5 < k )
					M[5] = 0xFFFFFFFFUL;
				if ( 6 < k )
					M[6] = 0xFFFFFFFFUL;
				if ( 7 < k )
					M[7] = 0xFFFFFFFFUL;
				return M; // 2^256-189 prime according to https://primes.utm.edu/lists/2small/200bit.html
			}

			template<size_t k>
			static libmaus2::math::UnsignedInteger<k> getNextPrime512()
			{
				libmaus2::math::UnsignedInteger<k> U(1);
				U <<= 512;
				U += libmaus2::math::UnsignedInteger<k>(75);
				return U;
			}

			template<size_t k>
			static libmaus2::math::UnsignedInteger<k> getNextPrime128()
			{
				libmaus2::math::UnsignedInteger<k> U(1);
				U <<= 128;
				U += libmaus2::math::UnsignedInteger<k>(51);
				return U;
			}
		};
	}
}
#endif
