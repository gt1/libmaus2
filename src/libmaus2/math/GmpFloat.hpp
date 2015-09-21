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
			GmpFloat & operator-();
			bool operator<(GmpFloat const & o) const;
			bool operator<=(GmpFloat const & o) const;
			bool operator==(GmpFloat const & o) const;
			bool operator!=(GmpFloat const & o) const;
			bool operator>(GmpFloat const & o) const;
			bool operator>=(GmpFloat const & o) const;
			operator double() const;
			operator uint64_t() const;
			operator int64_t() const;
		};
		
		GmpFloat operator+(GmpFloat const & A, GmpFloat const & B);
		GmpFloat operator-(GmpFloat const & A, GmpFloat const & B);
		GmpFloat operator*(GmpFloat const & A, GmpFloat const & B);
		GmpFloat operator/(GmpFloat const & A, GmpFloat const & B);
		std::ostream & operator<<(std::ostream & out, GmpFloat const & G);
	}
}
#endif
