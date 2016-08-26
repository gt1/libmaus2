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
#if ! defined(LIBMAUS2_MATH_GMPINTEGER_HPP)
#define LIBMAUS2_MATH_GMPINTEGER_HPP

#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/math/numbits.hpp>

namespace libmaus2
{
	namespace math
	{
		struct GmpInteger;
		GmpInteger operator+(GmpInteger const & A, GmpInteger const & B);
		GmpInteger operator-(GmpInteger const & A, GmpInteger const & B);
		GmpInteger operator*(GmpInteger const & A, GmpInteger const & B);
		GmpInteger operator/(GmpInteger const & A, GmpInteger const & B);
		GmpInteger operator%(GmpInteger const & A, GmpInteger const & B);
		std::ostream & operator<<(std::ostream & out, GmpInteger const & A);

		/**
		 * interface to GNU multi precision (GMP) library signed integers
		 **/
		struct GmpInteger
		{
			private:
			#if defined(LIBMAUS2_HAVE_GMP)
			void * v;
			#endif

			friend GmpInteger operator+(GmpInteger const & A, GmpInteger const & B);
			friend GmpInteger operator-(GmpInteger const & A, GmpInteger const & B);
			friend GmpInteger operator*(GmpInteger const & A, GmpInteger const & B);
			friend GmpInteger operator/(GmpInteger const & A, GmpInteger const & B);
			friend GmpInteger operator%(GmpInteger const & A, GmpInteger const & B);

			public:
			GmpInteger(int64_t rv = 0);
			GmpInteger(GmpInteger const & o);
			~GmpInteger();
			GmpInteger & operator=(GmpInteger const & o);
			std::string toString() const;
			GmpInteger & operator+=(GmpInteger const & o);
			GmpInteger & operator-=(GmpInteger const & o);
			GmpInteger & operator*=(GmpInteger const & o);
			GmpInteger & operator/=(GmpInteger const & o);
			GmpInteger & operator%=(GmpInteger const & o);
			GmpInteger & operator-();
			bool operator<(GmpInteger const & o) const;
			bool operator<=(GmpInteger const & o) const;
			bool operator==(GmpInteger const & o) const;
			bool operator!=(GmpInteger const & o) const;
			bool operator>(GmpInteger const & o) const;
			bool operator>=(GmpInteger const & o) const;
			operator double() const;
			operator uint64_t() const;
			operator int64_t() const;
		};

		GmpInteger operator+(GmpInteger const & A, GmpInteger const & B);
		GmpInteger operator-(GmpInteger const & A, GmpInteger const & B);
		GmpInteger operator*(GmpInteger const & A, GmpInteger const & B);
		GmpInteger operator/(GmpInteger const & A, GmpInteger const & B);
		GmpInteger operator%(GmpInteger const & A, GmpInteger const & B);
		std::ostream & operator<<(std::ostream & out, GmpInteger const & G);
	}
}
#endif
