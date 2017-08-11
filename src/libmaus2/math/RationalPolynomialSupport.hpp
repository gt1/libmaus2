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
#if ! defined(LIBMAUS2_MATH_RATIONALPOLYNOMIALSUPPORT_HPP)
#define LIBMAUS2_MATH_RATIONALPOLYNOMIALSUPPORT_HPP

#include <libmaus2/math/Rational.hpp>

namespace libmaus2
{
	namespace math
	{
		template<typename _number_type = int64_t>
		struct RationalPolynomialSupport
		{
			typedef _number_type number_type;

			static Rational<number_type> poleval(
				std::vector < Rational<number_type> > const & A,
				Rational<number_type> const & x
			)
			{
				Rational<number_type> P(1);
				Rational<number_type> S(0);

				for ( uint64_t i = 0; i < A.size(); ++i )
				{
					S += A[i] * P;
					P *= x;
				}

				return S;
			}

			static std::vector < Rational<number_type> > polmul(
				std::vector < Rational<number_type> > const & A,
				std::vector < Rational<number_type> > const & B
			)
			{
				std::vector < Rational<number_type> > P(A.size()+B.size());

				for ( uint64_t i = 0; i < A.size(); ++i )
					for ( uint64_t j = 0; j < B.size(); ++j )
						P[i+j] += A[i] * B[j];

				while ( P.size() && P.back().c == number_type() )
					P.pop_back();

				return P;
			}

			static std::vector < Rational<number_type> > polsum(
				std::vector < Rational<number_type> > const & A,
				std::vector < Rational<number_type> > const & B
			)
			{
				std::vector < Rational<number_type> > S(std::max(A.size(),B.size()));

				for ( uint64_t i = 0; i < A.size(); ++i )
					S[i] += A[i];
				for ( uint64_t i = 0; i < B.size(); ++i )
					S[i] += B[i];

				while ( S.size() && S.back() == number_type() )
					S.pop_back();

				return S;
			}

			static std::vector < Rational<number_type> > polpow(std::vector < Rational<number_type> > const & A, uint64_t const n)
			{
				std::vector < Rational<number_type> > R(1,Rational<number_type>(1));

				for ( uint64_t i = 0; i < n; ++i )
					R = polmul(R,A);

				return R;
			}

		};
	}
}
#endif
