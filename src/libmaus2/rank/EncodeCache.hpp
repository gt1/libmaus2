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

#if ! defined(ENCODECACHE_HPP)
#define ENCODECACHE_HPP

#include <libmaus2/rank/CodeBase.hpp>
#include <libmaus2/util/unique_ptr.hpp>
#include <algorithm>

namespace libmaus2
{
	namespace rank
	{
		/**
		 * cache for encoding in enumerative code of input length n bits with type
		 **/
		template<unsigned int n, typename type>
		struct EncodeCache : public CodeBase
		{
			typedef EncodeCache<n,type> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			private:
			autoarray::AutoArray<type> cache_n;
			autoarray::AutoArray<type> max_n;
			autoarray::AutoArray<uint8_t> pcnt;

			static unsigned numbits(unsigned int i)
			{
				unsigned int numbits = 0;

				while ( i )
				{
					numbits++;
					i >>= 1;
				}

				return numbits;
			}

			public:
			/**
			 * bits_n[i] = number of bits for encoding of number with
			 * i significant bits
			 **/
			autoarray::AutoArray<uint8_t> bits_n;

			/**
			 * constructor
			 **/
			EncodeCache()
			: cache_n( (1u<<n), false ), max_n( n+1 ), pcnt( 1u<<n, false), bits_n(n+1)
			{
				for ( unsigned int i = 0; i < n+1; ++i )
					max_n[i] = 0;

				for ( unsigned int i = 0; i < (1u<<n); ++i )
				{
					unsigned int const b = popcnt(i);
					pcnt[i] = b;
					cache_n[ i ] =  CC64.encode(static_cast<type>(i),b);
					max_n[b] = std::max(max_n[b], cache_n[i]);
				}

				for ( unsigned int i = 0; i < n+1; ++i )
					bits_n[i] = numbits(max_n[i]);
			}

			/**
			 * encode number in enumerative code
			 * @param num
			 * @return enumerative code for num
			 **/
			type encode(type num) const
			{
				return cache_n [ num ];
			}
		};
	}
}
#endif
