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
#if ! defined(LIBMAUS2_UTIL_QUEUE_HPP)
#define LIBMAUS2_UTIL_QUEUE_HPP

#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/util/ContainerElementFreeList.hpp>

namespace libmaus2
{
	namespace util
	{
		template<typename _value_type>
		struct Queue
		{
			typedef _value_type value_type;
			typedef Queue<value_type> this_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			// free list
			libmaus2::util::ContainerElementFreeList<value_type> freelist;

			// element array
			libmaus2::autoarray::AutoArray<value_type> & Aelements;

			// the queue
			libmaus2::autoarray::AutoArray<uint64_t> Q;
			uint64_t qlow;
			uint64_t qhigh;

			Queue() : freelist(), Aelements(freelist.getElementsArray()), qlow(0), qhigh(0)
			{

			}

			bool empty() const
			{
				return qlow == qhigh;
			}

			void clear()
			{
				while ( ! empty() )
					pop();
			}

			void push(value_type const & V)
			{
				if ( qhigh-qlow == Q.size() )
				{
					uint64_t const oldsize = Q.size();
					uint64_t const one = 1ull;
					uint64_t const newsize = std::max(one,2*oldsize);
					libmaus2::autoarray::AutoArray<uint64_t> NQ(newsize);
					for ( uint64_t i = 0; i < oldsize; ++i )
						NQ[i] = Q[(qlow + i)%Q.size()];

					qlow = 0;
					qhigh = oldsize;
					Q = NQ;
				}

				uint64_t const node = freelist.getNewNode();
				Aelements[node] = V;
				Q[(qhigh++)%Q.size()] = node;
			}

			value_type pop()
			{
				assert ( ! empty() );
				uint64_t const node = Q[(qlow++)%Q.size()];
				value_type const V = Aelements[node];
				freelist.deleteNode(node);
				return V;
			}
		};
	}
}
#endif
