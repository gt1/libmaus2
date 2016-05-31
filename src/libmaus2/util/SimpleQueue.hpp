/*
    libmaus2
    Copyright (C) 2016 German Tischler

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
#if ! defined(LIBMAUS2_UTIL_SIMPLEQUEUE_HPP)
#define LIBMAUS2_UTIL_SIMPLEQUEUE_HPP

#include <libmaus2/autoarray/AutoArray.hpp>

namespace libmaus2
{
	namespace util
	{
		template<typename _value_type>
		struct SimpleQueue
		{
			typedef _value_type value_type;
			typedef SimpleQueue<value_type> this_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			libmaus2::autoarray::AutoArray<value_type> Q;
			uint64_t m;

			uint64_t low;
			uint64_t high;

			void bump()
			{
				libmaus2::autoarray::AutoArray<value_type> NQ(Q.size()<<1,false);

				uint64_t j = 0;
				for ( uint64_t i = low; i < high; ++i )
					NQ[j++] = Q[i&m];

				high = high-low;
				low = 0;
				Q = NQ;
				m = Q.size()-1;
			}

			SimpleQueue()
			: Q(1), m(Q.size()-1), low(0), high(0)
			{

			}

			bool empty() const
			{
				return low == high;
			}

			void push_back(value_type v)
			{
				if ( high-low == Q.size() )
					bump();
				assert ( high-low < Q.size() );
				Q [ ( high++ ) & m ] = v;
			}

			value_type pop_front()
			{
				assert ( low != high );
				return Q [ (low++) & m ];
			}

			value_type back() const
			{
				return
					Q [ (high-1) & m ];
			}

			value_type front() const
			{
				return Q [ low & m ];
			}
		};
	}
}
#endif
