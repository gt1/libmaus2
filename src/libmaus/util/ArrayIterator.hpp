/**
    libmaus
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
**/
#if ! defined(LIBMAUS_UTIL_ARRAY_ITERATOR_HPP)
#define LIBMAUS_UTIL_ARRAY_ITERATOR_HPP

#include <iterator>
#include <libmaus/types/types.hpp>

namespace libmaus
{
	namespace util
	{
		template<typename array_type>
		struct ArrayIterator : public ::std::iterator< ::std::random_access_iterator_tag, int64_t>
		{
			typedef ::std::random_access_iterator_tag iterator_category;
			typedef ::std::iterator< ::std::random_access_iterator_tag, int64_t>::value_type value_type;
			typedef ::std::iterator< ::std::random_access_iterator_tag, int64_t>::difference_type difference_type;
			typedef ::std::iterator< ::std::random_access_iterator_tag, int64_t>::reference reference;
			typedef ::std::iterator< ::std::random_access_iterator_tag, int64_t>::pointer pointer;

			array_type const * A;
			int64_t i;
			
			ArrayIterator() : A(0), i(0) {}
			ArrayIterator(array_type const * rA) : A(rA), i(0) {}
			
			value_type operator*() const
			{
				return (*A)[i];
			}
			
			value_type operator[](int64_t j) const
			{
				return (*A)[i+j];
			}

			ArrayIterator & operator++()
			{
				i++;
				return *this;
			}
			ArrayIterator operator++(int)
			{
				ArrayIterator temp = *this;
				i++;
				return temp;
			}
			ArrayIterator & operator--()
			{
				i--;
				return *this;
			}
			ArrayIterator operator--(int)
			{
				ArrayIterator temp = *this;
				i--;
				return temp;
			}

			ArrayIterator & operator+=(int64_t j)
			{
				i += j;
				return *this;
			}
			ArrayIterator & operator-=(int64_t j)
			{
				i -= j;
				return *this;
			}
			
			bool operator<(ArrayIterator I) const
			{
				return i < I.i;
			}
			bool operator==(ArrayIterator I) const
			{
				return (A == I.A) && (i == I.i);
			}
			bool operator!=(ArrayIterator I) const
			{
				return ! ( (*this) == I );
			}
		};

		template<typename array_type>
		inline ArrayIterator<array_type> operator+ ( ArrayIterator<array_type> I, int64_t j )
		{
			ArrayIterator<array_type> J = I;
			J += j;
			return J;
		}

		template<typename array_type>
		inline ArrayIterator<array_type> operator- ( ArrayIterator<array_type> I, int64_t j )
		{
			ArrayIterator<array_type> J = I;
			J -= j;
			return J;
		}

		template<typename array_type>
		inline int64_t operator- ( ArrayIterator<array_type> I, ArrayIterator<array_type> J )
		{
			return static_cast<int64_t>(I.i) - static_cast<int64_t>(J.i);
		}
	}
}
#endif
