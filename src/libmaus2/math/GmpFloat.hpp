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
#if ! defined(LIBMAUS2_MATH_GMPFLOAT_HPP)
#define LIBMAUS2_MATH_GMPFLOAT_HPP

#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/math/numbits.hpp>

namespace libmaus2
{
	namespace math
	{
		struct GmpFloat;
		GmpFloat operator+(GmpFloat const & A, GmpFloat const & B);
		GmpFloat operator-(GmpFloat const & A, GmpFloat const & B);
		GmpFloat operator*(GmpFloat const & A, GmpFloat const & B);
		GmpFloat operator/(GmpFloat const & A, GmpFloat const & B);
		std::ostream & operator<<(std::ostream & out, GmpFloat const & A);

		/**
		 * interface to GNU multi precision (GMP) library signed integers
		 **/
		struct GmpFloat
		{
			private:
			void * v;

			friend GmpFloat operator+(GmpFloat const & A, GmpFloat const & B);
			friend GmpFloat operator-(GmpFloat const & A, GmpFloat const & B);
			friend GmpFloat operator*(GmpFloat const & A, GmpFloat const & B);
			friend GmpFloat operator/(GmpFloat const & A, GmpFloat const & B);

			public:
			GmpFloat(double rv = 0, unsigned int prec = 64);
			GmpFloat(GmpFloat const & o);
			~GmpFloat();
			GmpFloat & operator=(GmpFloat const & o);
			std::string toString() const;
			GmpFloat & operator+=(GmpFloat const & o);
			GmpFloat & operator-=(GmpFloat const & o);
			GmpFloat & operator*=(GmpFloat const & o);
			GmpFloat & operator/=(GmpFloat const & o);
			GmpFloat operator-() const;
			GmpFloat abs() const;
			bool operator<(GmpFloat const & o) const;
			bool operator<=(GmpFloat const & o) const;
			bool operator==(GmpFloat const & o) const;
			bool operator!=(GmpFloat const & o) const;
			bool operator>(GmpFloat const & o) const;
			bool operator>=(GmpFloat const & o) const;
			operator double() const;
			operator uint64_t() const;
			operator int64_t() const;
			GmpFloat & pow_ui(unsigned int const p);
			GmpFloat & mul_2exp(unsigned int const p);
			GmpFloat & div_2exp(unsigned int const p);

			libmaus2::math::GmpFloat exp(unsigned int const prec = 64) const
			{
				libmaus2::math::GmpFloat const & x = *this;

				libmaus2::math::GmpFloat f(1.0,prec);
				libmaus2::math::GmpFloat s(1.0,prec);
				libmaus2::math::GmpFloat const o(1.0,prec);

				libmaus2::math::GmpFloat limit(1.0/2.0,prec);
				limit.div_2exp(prec);

				libmaus2::math::GmpFloat i(1.0,prec);
				while ( true )
				{
					f *= x;
					f /= libmaus2::math::GmpFloat(i,prec);

					if ( f.abs() < limit )
						break;

					s += f;
					i += o;
				}

				return s;
			}

			libmaus2::math::GmpFloat log(unsigned int const prec = 64) const
			{
				libmaus2::math::GmpFloat const & x = *this;

				if ( x > libmaus2::math::GmpFloat(2,prec) )
				{
					libmaus2::math::GmpFloat a(1,prec);
					libmaus2::math::GmpFloat z(2,prec);
					uint64_t power = 0;

					while ( a < x )
					{
						a = a*z;
						power++;
					}

					assert ( power );

					return
						(x / a).log(prec) +
						libmaus2::math::GmpFloat(power,prec) * libmaus2::math::GmpFloat(2.0, prec).log(prec);
				}

				libmaus2::math::GmpFloat const d = (x-libmaus2::math::GmpFloat(1,prec)) / (x+libmaus2::math::GmpFloat(1,prec));
				libmaus2::math::GmpFloat f = d;

				libmaus2::math::GmpFloat limit(1.0/2.0,prec);
				limit.div_2exp(prec);

				uint64_t i = 1;

				libmaus2::math::GmpFloat s(0,prec);

				while ( true )
				{
					libmaus2::math::GmpFloat const a = f / libmaus2::math::GmpFloat(i,prec);

					if ( a.abs() < limit )
						break;

					s += a;

					f *= d;
					f *= d;
					i += 2;
				}

				return s * libmaus2::math::GmpFloat(2.0,prec);
			}
		};

		GmpFloat operator+(GmpFloat const & A, GmpFloat const & B);
		GmpFloat operator-(GmpFloat const & A, GmpFloat const & B);
		GmpFloat operator*(GmpFloat const & A, GmpFloat const & B);
		GmpFloat operator/(GmpFloat const & A, GmpFloat const & B);
		std::ostream & operator<<(std::ostream & out, GmpFloat const & G);
	}
}
#endif
