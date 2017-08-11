/*
    libmaus2
    Copyright (C) 2017 German Tischler

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

#if ! defined(LIBMAUS2_MATH_CONSTANTSUMCOMBINATORICS_HPP)
#define LIBMAUS2_MATH_CONSTANTSUMCOMBINATORICS_HPP

#include <libmaus2/math/RationalPolynomialSupport.hpp>

namespace libmaus2
{
	namespace math
	{
		template<typename _number_type = int64_t>
		struct ConstantSumCombinatorics : public RationalPolynomialSupport<_number_type>
		{
			typedef _number_type number_type;
			typedef RationalPolynomialSupport<number_type> base_type;

			static std::vector < Rational<number_type> > first()
			{
				return std::vector < Rational<number_type> >(1,Rational<number_type>(1));
			}

			static std::vector < Rational<number_type> > next(std::vector < Rational<number_type> > const & prev)
			{
				std::vector < Rational<number_type> > sum;

				for ( uint64_t i = 0; i < prev.size(); ++i )
				{
					sum = base_type::polsum(sum,base_type::polmul(std::vector < Rational<number_type> >(1,prev[i]), Faulhaber<number_type>::polynomial(i)));

					if ( i == 0 )
						sum = base_type::polsum(sum,std::vector < Rational<number_type> >(1,prev[i]));
				}

				return sum;
			}

			static std::vector < Rational<number_type> > next(std::vector < Rational<number_type> > const & prev, libmaus2::math::BernoulliNumberCache<number_type> & Bcache)
			{
				std::vector < Rational<number_type> > sum;

				for ( uint64_t i = 0; i < prev.size(); ++i )
				{
					sum = base_type::polsum(sum,base_type::polmul(std::vector < Rational<number_type> >(1,prev[i]), Faulhaber<number_type>::polynomial(i,Bcache)));

					if ( i == 0 )
						sum = base_type::polsum(sum,std::vector < Rational<number_type> >(1,prev[i]));
				}

				return sum;
			}

			static std::vector < Rational<number_type> > nextpow(std::vector < Rational<number_type> > pol, uint64_t const n)
			{
				for ( uint64_t i = 0; i < n; ++i )
					pol = next(pol);
				return pol;
			}

			static std::vector < Rational<number_type> > kth(uint64_t const k)
			{
				return nextpow(first(),k);
			}
		};
	}
}
#endif
