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
#if ! defined(LIBMAUS2_UTIL_COUNTPUTITERATOR)
#define LIBMAUS2_UTIL_COUNTPUTITERATOR

#include <libmaus2/types/types.hpp>

namespace libmaus2
{
	namespace util
	{
		/**
		 * class mapping output iterator operations
		 * to put calls on a wrapped object
		 **/
		template<typename _output_type, typename _value_type>
		struct CountPutIterator
		{
			//! output type
			typedef _output_type output_type;
			//! value type
			typedef _value_type value_type;
			//! this type
			typedef CountPutIterator<output_type,value_type> this_type;
		
			//! wrapped object written elements are trasnfered to
			output_type * out;
			//! number of elements written
			uint64_t c;
		
			/**
			 * constructor
			 *
			 * @param rout pointer to wrapped object
			 **/
			CountPutIterator(output_type * const rout)
			: out(rout), c(0) {}
			
			/**
			 * @return state of wrapped object
			 **/
			operator bool() const
			{
				return *out;
			}
			
			/**
			 * put element v
			 *
			 * @param v element to be put
			 **/
			void put(value_type const v)
			{
				c += 1;
				out->put(v);
			}
			
			/**
			 * put string s by putting its symbols
			 *
			 * @param s string
			 **/
			void put(std::string const & s)
			{
				uint8_t const * u = reinterpret_cast<uint8_t const *>(s.c_str());
				uint8_t const * const ue = u + s.size();

				while ( u != ue )
					put(*(u++));
			}
			
			/**
			 * assignment operator calling put operation on wrapped object for v
			 *
			 * @param v element to be transferred to wrapped object
			 * @return *this
			 **/
			this_type & operator=(value_type const v)
			{
				put(v);
				return *this;
			}
			
			/**
			 * dereference operator (does nothing)
			 *
			 * @return *this
			 **/
			this_type & operator*() { return *this; }
			/**
			 * prefix increment (does nothing)
			 *
			 * @return *this
			 **/
			this_type & operator++() { return *this; }
			/**
			 * postfix increment (does nothing)
			 *
			 * @return *this
			 **/
			this_type & operator++(int) { return *this; }		
		};	
	}
}
#endif
