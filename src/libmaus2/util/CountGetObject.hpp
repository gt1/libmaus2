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
#if ! defined(LIBMAUS_UTIL_COUNTGETOBJECT_HPP)
#define LIBMAUS_UTIL_COUNTGETOBJECT_HPP

#include <iterator>

namespace libmaus2
{
	namespace util
	{
		/**
		 * class wrapping an iterator and providing get and read operations
		 **/
		template<typename _iterator>
		struct CountGetObject
		{
			//! iterator type
			typedef _iterator iterator;
			//! value type
			typedef typename ::std::iterator_traits<iterator>::value_type value_type;
		
			//! wrapped iterator
			iterator p;
			//! elements extracted by last read or get operation
			uint64_t gcnt;
			
			/**
			 * constructor
			 *
			 * @param iterator rp
			 **/
			CountGetObject(iterator rp) : p(rp), gcnt(0) {}
			/**
			 * @return next element
			 **/
			value_type get() { gcnt=1; return *(p++); }
			/**
			 * read n elements and store them in q
			 *
			 * @param q output iterator
			 * @param n number of elements to copy
			 **/
			template<typename value_copy_type>
			void read(value_copy_type * q, uint64_t n)
			{
				gcnt = n;
				while ( n-- )
					*(q++) = *(p++);
			}
			/**
			 * @return number of elements extracted by last get or read operation
			 **/
			uint64_t gcount() const
			{
				return gcnt;
			}
		};
		/**
		 * specialisation of CountGetObject class for char const * iterators;
		 * the get functions returns unsigned char values casted to int
		 * (as the get function of an istream object would)
		 **/
		template<>
		struct CountGetObject<char const *>
		{
			//! iterator type
			typedef char const * iterator;
			//! value_type
			typedef ::std::iterator_traits<iterator>::value_type value_type;
		
			//! iterator
			iterator p;
			//! number of elements extracted by last read/get operation
			uint64_t gcnt;
			
			/**
			 * constructor
			 *
			 * @param iterator rp
			 **/
			CountGetObject(iterator rp) : p(rp), gcnt(0) {}
			/**
			 * @return next element
			 **/
			int get() { gcnt=1; return *(reinterpret_cast<uint8_t const *>(p++)); }
			/**
			 * read n elements and store them in q
			 *
			 * @param q output iterator
			 * @param n number of elements to copy
			 **/
			void read(value_type * q, uint64_t n)
			{
				gcnt = n;
				while ( n-- )
					*(q++) = *(p++);
			}
			/**
			 * @return number of elements extracted by last get or read operation
			 **/
			uint64_t gcount() const
			{
				return gcnt;
			}
		};
	}
}
#endif
