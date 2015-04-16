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

#if ! defined(DECODECACHE_HPP)
#define DECODECACHE_HPP

#include <libmaus2/rank/CodeBase.hpp>
#include <libmaus2/util/unique_ptr.hpp>

namespace libmaus2
{
	namespace rank
	{
		/**
		 * cache for decoding enumerative code up to n bits
		 **/
		template<unsigned int n, typename type>
		struct DecodeCache : public CodeBase
		{
			typedef DecodeCache<n,type> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			
			private:
			::libmaus2::autoarray::AutoArray<type> cache_n;
			::libmaus2::autoarray::AutoArray<unsigned int> offset_n;
			
			void setOffsets()
			{
				unsigned int s = 0;
				for ( unsigned int i = 0; i <= n; ++i )
				{
					offset_n[i] = s;
					s += CC64(n,i);
				}
			}
			
			public:
			/**
			 * constructor
			 **/
			DecodeCache()
			: cache_n( CC64.sum(n), false ), offset_n(n+1)
			{
				setOffsets();
				
				for ( unsigned int i = 0; i < (1u<<n); ++i )
				{
					unsigned int const b = popcnt(i);
					cache_n[ offset_n[b] + CC64.encode(static_cast<type>(i),b) ] = i;
				}
			}
			
			/**
			 * decode num with u significant bits
			 * @param num code
			 * @param u number of significant bits
			 * @return decoded number
			 **/
			type decode(type num, unsigned int u) const
			{
				return cache_n [ offset_n[u] + num ];
			}
		};
	}
}
#endif
