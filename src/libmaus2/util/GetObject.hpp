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
#if ! defined(LIBMAUS2_UTIL_GETOBJECT_HPP)
#define LIBMAUS2_UTIL_GETOBJECT_HPP

#include <iterator>
#include <libmaus2/util/unique_ptr.hpp>

namespace libmaus2
{
	namespace util
	{
		/**
		 * class mapping get operations to iterator reads
		 **/
		template<typename _iterator>
		struct GetObject
		{
			//! iterator type
			typedef _iterator iterator;
			//! this type
			typedef GetObject<iterator> this_type;
			//! unique pointer type
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			//! value type
			typedef typename ::std::iterator_traits<iterator>::value_type value_type;
			//! data type
			typedef value_type data_type;
			//! iterator
			iterator p;

			/**
			 * constructor
			 *
			 * @param rp iterator
			 **/
			GetObject(iterator rp) : p(rp) {}

			/**
			 * @return next element
			 **/
			value_type get() { return *(p++); }

			/**
			 * get next element
			 **/
			bool getNext(value_type & v) { v = *(p++); return true; }

			/**
			 * read next n elements and store them starting at q
			 *
			 * @param q output iterator
			 * @param n number of elements to extract
			 **/
			void read(value_type * q, uint64_t n)
			{
				while ( n-- )
					*(q++) = *(p++);
			}
		};
	}
}
#endif
