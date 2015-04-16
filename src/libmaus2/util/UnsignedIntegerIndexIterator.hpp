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

#if ! defined(LIBMAUS_UTIL_UNSIGNEDINTEGERINDEXITERATOR_HPP)
#define LIBMAUS_UTIL_UNSIGNEDINTEGERINDEXITERATOR_HPP

#include <libmaus2/math/UnsignedInteger.hpp>
#include <iterator>

namespace libmaus2
{
	namespace util
	{
		template<typename _owner_type, typename _data_type, size_t _k>
		struct UnsignedIntegerIndexIterator : public ::std::iterator< ::std::random_access_iterator_tag, _data_type>
		{
			typedef _owner_type owner_type;
			typedef _data_type data_type;
			static size_t const k = _k;
			typedef UnsignedIntegerIndexIterator<owner_type,data_type,k> this_type;

			typedef libmaus2::math::UnsignedInteger<k> index_type;

			typedef ::std::random_access_iterator_tag iterator_category;
			typedef typename ::std::iterator< ::std::random_access_iterator_tag, data_type>::value_type value_type;
			typedef index_type difference_type;
			typedef typename ::std::iterator< ::std::random_access_iterator_tag, data_type>::reference reference;
			typedef typename ::std::iterator< ::std::random_access_iterator_tag, data_type>::pointer pointer;
						
			owner_type const * owner;
			index_type i;
			
			UnsignedIntegerIndexIterator() : owner(0), i(0) {}
			UnsignedIntegerIndexIterator(owner_type const * rowner, int64_t const ri = 0) : owner(rowner), i(ri) {}
			UnsignedIntegerIndexIterator(owner_type const * rowner, index_type const & ri) : owner(rowner), i(ri) {}
			UnsignedIntegerIndexIterator(UnsignedIntegerIndexIterator const & o) : owner(o.owner), i(o.i) {}
			
			data_type operator*() const
			{
				return owner->get(i);
			}
			
			data_type operator[](int64_t j) const
			{
				return owner->get(i+j);
			}

			this_type & operator++()
			{
				i += index_type(1);
				return *this;
			}
			this_type operator++(int)
			{
				this_type temp = *this;
				i += index_type(1);
				return temp;
			}
			this_type & operator--()
			{
				i -= index_type(1);
				return *this;
			}
			this_type operator--(int)
			{
				this_type temp = *this;
				i -= index_type(1);
				return temp;
			}

			this_type & operator+=(index_type const & j)
			{
				i += j;
				return *this;
			}
			this_type & operator-=(index_type const & j)
			{
				i -= j;
				return *this;
			}
			
			bool operator<(this_type const & I) const
			{
				return i < I.i;
			}
			bool operator==(this_type const & I) const
			{
				return (owner==I.owner) && (i == I.i);
			}
			bool operator!=(this_type const & I) const
			{
				return ! ( operator==(I) );
			}
		};

		template<typename _owner_type, typename _data_type, size_t _k>
		inline UnsignedIntegerIndexIterator<_owner_type,_data_type,_k> operator+(UnsignedIntegerIndexIterator<_owner_type,_data_type,_k> const & I, libmaus2::math::UnsignedInteger<_k> const & j)
		{
			UnsignedIntegerIndexIterator<_owner_type,_data_type,_k> J = I;
			J += j;
			return J;
		}

		template<typename _owner_type, typename _data_type, size_t _k>
		inline UnsignedIntegerIndexIterator<_owner_type,_data_type,_k> operator- ( UnsignedIntegerIndexIterator<_owner_type,_data_type,_k> const & I, libmaus2::math::UnsignedInteger<_k> const & j)
		{
			UnsignedIntegerIndexIterator<_owner_type,_data_type,_k> J = I;
			J -= j;
			return J;
		}

		template<typename _owner_type, typename _data_type, size_t _k>
		inline libmaus2::math::UnsignedInteger<_k> operator- ( UnsignedIntegerIndexIterator<_owner_type,_data_type,_k> const & I, UnsignedIntegerIndexIterator<_owner_type,_data_type,_k> const & J )
		{
			return I.i - J.i;
		}
	}
}
#endif
