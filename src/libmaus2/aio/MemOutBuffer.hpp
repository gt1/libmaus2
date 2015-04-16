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


#if ! defined(MEMOUTBUFFER_HPP)
#define MEMOUTBUFFER_HPP

#include <libmaus2/types/types.hpp>
#include <libmaus2/util/unique_ptr.hpp>

namespace libmaus2
{
	namespace aio
	{
		/**
		 * in memory output buffer
		 **/
		template<typename _value_type>
		struct MemOutBuffer
		{
			//! value type
			typedef _value_type value_type;
			//! this type
			typedef MemOutBuffer<value_type> this_type;
			//! unique pointer type
			typedef typename ::libmaus2::util::unique_ptr < this_type >::type unique_ptr_type;

			private:
			//! output pointer
			value_type * out;
			
			public:
			/**
			 * constructor
			 *
			 * @param rout output buffer pointer
			 **/
			MemOutBuffer(value_type * const rout)
			: out(rout)
			{
			
			}
			
			/**
			 * put element
			 *
			 * @param v element
			 **/
			void put(value_type const v)
			{
				*(out++) = v;
			}
			
			/**
			 * flush buffer (do nothing)
			 **/
			void flush()
			{
			
			}
		};
	}
}
#endif
