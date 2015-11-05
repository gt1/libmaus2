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


#if ! defined(MEMINBUFFER_HPP)
#define MEMINBUFFER_HPP

#include <libmaus2/types/types.hpp>
#include <libmaus2/util/unique_ptr.hpp>

namespace libmaus2
{
	namespace aio
	{
		/**
		 * in memoery array based input buffer
		 **/
		template<typename _value_type>
		struct MemInBuffer
		{
			//! value type
			typedef _value_type value_type;
			//! this type
			typedef MemInBuffer<value_type> this_type;
			//! unique pointer type
			typedef typename ::libmaus2::util::unique_ptr < this_type >::type unique_ptr_type;

			private:
			//! current input pointer
			value_type const * in;
			//! end of input pointer
			value_type const * const ine;

			public:
			/**
			 * constructor
			 *
			 * @param rin start of buffer pointer
			 * @param n length of buffer in elements
			 **/
			MemInBuffer(value_type const * const rin, uint64_t const n)
			: in(rin), ine(in+n)
			{

			}

			/**
			 * get next element
			 * @param v reference for storing next element
			 * @return true iff next element was available
			 **/
			bool getNext(value_type & v)
			{
				if ( in == ine )
					return false;
				else
				{
					v = *(in++);
					return true;
				}
			}

			/**
			 * flush (do nothing)
			 **/
			void flush()
			{

			}
		};
	}
}
#endif
