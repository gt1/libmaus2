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

#if ! defined(LIBMAUS_UTIL_ITERATOR_HPP)
#define LIBMAUS_UTIL_ITERATOR_HPP

#include <libmaus/types/types.hpp>
#include <iterator>

namespace libmaus
{
	namespace util
	{
		template<typename owner_type, typename data_type>
		struct AssignmentProxy
		{
			private:
			AssignmentProxy<owner_type,data_type> & operator=(AssignmentProxy<owner_type,data_type> &)
			{
				return *this;
			}
			
			public:
			owner_type * owner;
			int64_t i;

			AssignmentProxy() : owner(0), i(0) {}
			AssignmentProxy(owner_type * const rowner, int64_t const ri = 0) : owner(rowner), i(ri) 
			{
			}
			~AssignmentProxy()
			{
			}
			
			AssignmentProxy<owner_type,data_type> & operator=(data_type const v)
			{
				owner->set ( i, v );							
				return *this;
			}

			operator data_type() const
			{
				return owner->get(i);
			}
		};
	
		template<typename _owner_type, typename _data_type>
		struct AssignmentProxyIterator
		{
			typedef _owner_type owner_type;
			typedef _data_type data_type;
			typedef AssignmentProxy<owner_type,data_type> proxy_type;
			typedef AssignmentProxyIterator<owner_type,data_type> this_type;
		
			typedef std::random_access_iterator_tag iterator_category;
			typedef proxy_type   reference;
			typedef proxy_type * pointer;
			typedef typename ::std::iterator< ::std::random_access_iterator_tag, data_type>::value_type value_type;
			typedef typename ::std::iterator< ::std::random_access_iterator_tag, data_type>::difference_type difference_type;

			owner_type * owner;
			int64_t i;
			
			AssignmentProxyIterator() : owner(0), i(0) {}
			AssignmentProxyIterator(owner_type * const rowner, int64_t const ri = 0) : owner(rowner), i(ri) {}
			AssignmentProxyIterator(this_type const & o) : owner(o.owner), i(o.i) {}
			
			AssignmentProxyIterator & operator=(this_type const & o)
			{
				if ( this != &o )
				{
					owner = o.owner;
					i = o.i;
				}
				return *this;
			}
			
			proxy_type operator*()
			{
				return proxy_type(owner,i);
			}
			
			proxy_type operator[](int64_t j) const
			{
				return proxy_type(owner,i+j);
			}

			this_type & operator++()
			{
				i++;
				return *this;
			}
			this_type operator++(int)
			{
				this_type temp = *this;
				i++;
				return temp;
			}
			this_type & operator--()
			{
				i--;
				return *this;
			}
			this_type operator--(int)
			{
				this_type temp = *this;
				i--;
				return temp;
			}

			this_type & operator+=(int64_t j)
			{
				i += j;
				return *this;
			}
			this_type & operator-=(int64_t j)
			{
				i -= j;
				return *this;
			}
			
			bool operator<(this_type I) const
			{
				return i < I.i;
			}
			bool operator>(this_type I) const
			{
				return i > I.i;
			}
			bool operator==(this_type I) const
			{
				return (owner==I.owner) && (i == I.i);
			}
			bool operator!=(this_type I) const
			{
				return ! (operator==(I));
			}
			bool operator<=(this_type I) const
			{
				return i <= I.i;
			}
		};

		template<typename _owner_type, typename _data_type>
		inline AssignmentProxyIterator<_owner_type,_data_type> operator+ ( AssignmentProxyIterator<_owner_type,_data_type> const & I, int64_t j )
		{
			AssignmentProxyIterator<_owner_type,_data_type> J = I;
			J += j;
			return J;
		}

		template<typename _owner_type, typename _data_type>
		inline AssignmentProxyIterator<_owner_type,_data_type> operator- ( AssignmentProxyIterator<_owner_type,_data_type> const & I, int64_t j )
		{
			AssignmentProxyIterator<_owner_type,_data_type> J = I;
			J -= j;
			return J;
		}

		template<typename _owner_type, typename _data_type>
		inline int64_t operator-(AssignmentProxyIterator<_owner_type,_data_type> const & A, AssignmentProxyIterator<_owner_type,_data_type> const & B)
		{
			return static_cast<int64_t>(A.i) - static_cast<int64_t>(B.i);
		}

		template<typename _owner_type, typename _data_type>
		struct ConstIterator : public ::std::iterator< ::std::random_access_iterator_tag, _data_type>
		{
			typedef _owner_type owner_type;
			typedef _data_type data_type;
			typedef ConstIterator<owner_type,data_type> this_type;
			typedef AssignmentProxyIterator<owner_type,data_type> iterator;

			typedef ::std::random_access_iterator_tag iterator_category;
			typedef typename ::std::iterator< ::std::random_access_iterator_tag, data_type>::value_type value_type;
			typedef typename ::std::iterator< ::std::random_access_iterator_tag, data_type>::difference_type difference_type;
			typedef typename ::std::iterator< ::std::random_access_iterator_tag, data_type>::reference reference;
			typedef typename ::std::iterator< ::std::random_access_iterator_tag, data_type>::pointer pointer;

			owner_type const * owner;
			int64_t i;
			
			ConstIterator() : owner(0), i(0) {}
			ConstIterator(owner_type const * rowner, int64_t const ri = 0) : owner(rowner), i(ri) {}
			ConstIterator(ConstIterator const & o) : owner(o.owner), i(o.i) {}
			ConstIterator(iterator const & o) : owner(o.owner), i(o.i) {}
			
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
				i++;
				return *this;
			}
			this_type operator++(int)
			{
				this_type temp = *this;
				i++;
				return temp;
			}
			this_type & operator--()
			{
				i--;
				return *this;
			}
			this_type operator--(int)
			{
				this_type temp = *this;
				i--;
				return temp;
			}

			this_type & operator+=(int64_t j)
			{
				i += j;
				return *this;
			}
			this_type & operator-=(int64_t j)
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

		template<typename _owner_type, typename _data_type>
		inline ConstIterator<_owner_type,_data_type> operator+ ( ConstIterator<_owner_type,_data_type> const & I, int64_t j )
		{
			ConstIterator<_owner_type,_data_type> J = I;
			J += j;
			return J;
		}

		template<typename _owner_type, typename _data_type>
		inline ConstIterator<_owner_type,_data_type> operator- ( ConstIterator<_owner_type,_data_type> const & I, int64_t j )
		{
			ConstIterator<_owner_type,_data_type> J = I;
			J -= j;
			return J;
		}

		template<typename _owner_type, typename _data_type>
		inline int64_t operator- ( ConstIterator<_owner_type,_data_type> const & I, ConstIterator<_owner_type,_data_type> const & J )
		{
			return static_cast<int64_t>(I.i) - static_cast<int64_t>(J.i);
		}

		template<typename _owner_type, typename _data_type>
		inline int64_t operator-(AssignmentProxyIterator<_owner_type,_data_type> const & A, ConstIterator<_owner_type,_data_type> const & B)
		{
			return static_cast<int64_t>(A.i) - static_cast<int64_t>(B.i);
		}

		template<typename _owner_type, typename _data_type>
		inline int64_t operator-(ConstIterator<_owner_type,_data_type> const & A, AssignmentProxyIterator<_owner_type,_data_type> const & B)
		{
			return static_cast<int64_t>(A.i) - static_cast<int64_t>(B.i);
		}

		template<typename _owner_type, typename _data_type>
		struct ConstIteratorSharedPointer : public ::std::iterator< ::std::random_access_iterator_tag, _data_type>
		{
			typedef _owner_type owner_type;
			typedef _data_type data_type;
			typedef ConstIteratorSharedPointer<owner_type,data_type> this_type;
			typedef typename owner_type::shared_ptr_type owner_ptr_type;

			typedef ::std::random_access_iterator_tag iterator_category;
			typedef typename ::std::iterator< ::std::random_access_iterator_tag, data_type>::value_type value_type;
			typedef typename ::std::iterator< ::std::random_access_iterator_tag, data_type>::difference_type difference_type;
			typedef typename ::std::iterator< ::std::random_access_iterator_tag, data_type>::reference reference;
			typedef typename ::std::iterator< ::std::random_access_iterator_tag, data_type>::pointer pointer;

			owner_ptr_type owner;
			int64_t i;
			
			ConstIteratorSharedPointer() : owner(), i(0) {}
			ConstIteratorSharedPointer(owner_ptr_type const & rowner, int64_t const ri = 0) : owner(rowner), i(ri) {}
			ConstIteratorSharedPointer(ConstIteratorSharedPointer const & o) : owner(o.owner), i(o.i) {}
			
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
				i++;
				return *this;
			}
			this_type operator++(int)
			{
				this_type temp = *this;
				i++;
				return temp;
			}
			this_type & operator--()
			{
				i--;
				return *this;
			}
			this_type operator--(int)
			{
				this_type temp = *this;
				i--;
				return temp;
			}

			this_type & operator+=(int64_t j)
			{
				i += j;
				return *this;
			}
			this_type & operator-=(int64_t j)
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

		template<typename _owner_type, typename _data_type>
		inline ConstIteratorSharedPointer<_owner_type,_data_type> operator+ ( ConstIteratorSharedPointer<_owner_type,_data_type> const & I, int64_t j )
		{
			ConstIteratorSharedPointer<_owner_type,_data_type> J = I;
			J += j;
			return J;
		}

		template<typename _owner_type, typename _data_type>
		inline ConstIteratorSharedPointer<_owner_type,_data_type> operator- ( ConstIteratorSharedPointer<_owner_type,_data_type> const & I, int64_t j )
		{
			ConstIteratorSharedPointer<_owner_type,_data_type> J = I;
			J -= j;
			return J;
		}

		template<typename _owner_type, typename _data_type>
		inline int64_t operator- ( ConstIteratorSharedPointer<_owner_type,_data_type> const & I, ConstIteratorSharedPointer<_owner_type,_data_type> const & J )
		{
			return static_cast<int64_t>(I.i) - static_cast<int64_t>(J.i);
		}

		template<typename _owner_type, typename _data_type>
		inline int64_t operator-(AssignmentProxyIterator<_owner_type,_data_type> const & A, ConstIteratorSharedPointer<_owner_type,_data_type> const & B)
		{
			return static_cast<int64_t>(A.i) - static_cast<int64_t>(B.i);
		}

		template<typename _owner_type, typename _data_type>
		inline int64_t operator-(ConstIteratorSharedPointer<_owner_type,_data_type> const & A, AssignmentProxyIterator<_owner_type,_data_type> const & B)
		{
			return static_cast<int64_t>(A.i) - static_cast<int64_t>(B.i);
		}

	}
}
#endif
