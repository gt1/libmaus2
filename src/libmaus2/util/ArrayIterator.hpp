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
#if ! defined(LIBMAUS2_UTIL_ARRAY_ITERATOR_HPP)
#define LIBMAUS2_UTIL_ARRAY_ITERATOR_HPP

#include <iterator>
#include <libmaus2/types/types.hpp>

namespace libmaus2
{
	namespace util
	{
		/**
		 * const iterator type for an object supporting
		 * indexed access ([]) operations
		 **/
		template<typename array_type>
		struct ArrayIterator : public ::std::iterator< ::std::random_access_iterator_tag, int64_t>
		{
			//! iterator category type
			typedef ::std::random_access_iterator_tag iterator_category;
			//! value type
			typedef ::std::iterator< ::std::random_access_iterator_tag, int64_t>::value_type value_type;
			//! difference type
			typedef ::std::iterator< ::std::random_access_iterator_tag, int64_t>::difference_type difference_type;
			//! reference type
			typedef ::std::iterator< ::std::random_access_iterator_tag, int64_t>::reference reference;
			//! pointer type
			typedef ::std::iterator< ::std::random_access_iterator_tag, int64_t>::pointer pointer;

			//! wrapped array
			array_type const * A;
			//! current index
			int64_t i;
			
			/**
			 * constructor for invalid iterator
			 **/
			ArrayIterator() : A(0), i(0) {}
			/**
			 * constructor from array pointing the constructed object at the start of the array (index 0)
			 **/
			ArrayIterator(array_type const * rA) : A(rA), i(0) {}
			
			/**
			 * dereference operator
			 *
			 * @return value at current position
			 **/
			value_type operator*() const
			{
				return (*A)[i];
			}
			
			/**
			 * access operator
			 *
			 * @param j index offset
			 * @return value at current position plus j
			 **/
			value_type operator[](int64_t j) const
			{
				return (*A)[i+j];
			}

			/**
			 * prefix increment, increment current position by one and return modified iterator
			 *
			 * @return prefix increment of iterator
			 **/
			ArrayIterator & operator++()
			{
				i++;
				return *this;
			}
			/**
			 * postfix increment, increment current position by one and return original iterator
			 *
			 * @return prefix increment of iterator
			 **/
			ArrayIterator operator++(int)
			{
				ArrayIterator temp = *this;
				i++;
				return temp;
			}
			/**
			 * prefix decrement
			 *
			 * @return prefix decrement of iterator
			 **/
			ArrayIterator & operator--()
			{
				i--;
				return *this;
			}
			/**
			 * postfix decrement
			 *
			 * @return postfix decrement of iterator
			 **/
			ArrayIterator operator--(int)
			{
				ArrayIterator temp = *this;
				i--;
				return temp;
			}

			/**
			 * add j to the current position
			 *
			 * @param j offset
			 * @return modfified iterator
			 **/
			ArrayIterator & operator+=(int64_t j)
			{
				i += j;
				return *this;
			}
			/**
			 * subtract j to the current position
			 *
			 * @param j offset
			 * @return modfified iterator
			 **/
			ArrayIterator & operator-=(int64_t j)
			{
				i -= j;
				return *this;
			}
			
			/**
			 * @param I other iterator
			 * @return true iff index of this iterator is smaller than index of other iterator
			 **/
			bool operator<(ArrayIterator I) const
			{
				return i < I.i;
			}
			/**
			 * @param I other iterator
			 * @return true iff index of this iterator equals index of other iterator
			 **/
			bool operator==(ArrayIterator I) const
			{
				return (A == I.A) && (i == I.i);
			}
			/**
			 * @param I other iterator
			 * @return true iff index of this iterator does not equal index of other iterator
			 **/
			bool operator!=(ArrayIterator I) const
			{
				return ! ( (*this) == I );
			}
		};

		/**
		 * @param I iterator
		 * @param j offset
		 * @return I+j
		 **/
		template<typename array_type>
		inline ArrayIterator<array_type> operator+ ( ArrayIterator<array_type> I, int64_t j )
		{
			ArrayIterator<array_type> J = I;
			J += j;
			return J;
		}

		/**
		 * @param I iterator
		 * @param j offset
		 * @return I-j
		 **/
		template<typename array_type>
		inline ArrayIterator<array_type> operator- ( ArrayIterator<array_type> I, int64_t j )
		{
			ArrayIterator<array_type> J = I;
			J -= j;
			return J;
		}

		/**
		 * @param I iterator
		 * @param J iterator
		 * @return distance between the indices of I and J
		 **/
		template<typename array_type>
		inline int64_t operator- ( ArrayIterator<array_type> I, ArrayIterator<array_type> J )
		{
			return static_cast<int64_t>(I.i) - static_cast<int64_t>(J.i);
		}
	}
}
#endif
