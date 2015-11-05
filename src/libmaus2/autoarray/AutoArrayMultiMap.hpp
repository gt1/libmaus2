/*
    libmaus2
    Copyright (C) 2015 German Tischler

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
#if ! defined(LIBMAUS2_AUTOARRAY_AUTOARRAYMULTIMAP_HPP)
#define LIBMAUS2_AUTOARRAY_AUTOARRAYMULTIMAP_HPP

#include <libmaus2/autoarray/AutoArray.hpp>
#include <algorithm>

namespace libmaus2
{
	namespace autoarray
	{
		template<typename _key_type, typename _value_type>
		struct AutoArrayMultiMap
		{
			typedef _key_type key_type;
			typedef _value_type value_type;
			typedef AutoArrayMultiMap<key_type,value_type> this_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
			typedef std::pair<key_type,value_type> const * const_iterator;
			typedef std::pair<key_type,value_type> * iterator;
			typedef key_type const * const_key_iterator;
			typedef key_type * key_iterator;

			struct FirstComparator
			{
				bool operator()(std::pair<key_type,value_type> const & A, std::pair<key_type,value_type> const & B) const
				{
					return A.first < B.first;
				}
			};

			struct AutoArrayMultiMapValueAdapter
			{
				typedef _key_type key_type;
				typedef _value_type value_type;
				typedef AutoArrayMultiMapValueAdapter this_type;
				typedef AutoArrayMultiMap<key_type,value_type> map_type;
				typedef typename map_type::const_iterator const_iterator;

				const_iterator it;
				uint64_t n;

				AutoArrayMultiMapValueAdapter(const_iterator const & rit, uint64_t const rn)
				: it(rit), n(rn)
				{

				}

				value_type const & operator[](uint64_t const i) const
				{
					return it[i].second;
				}

				uint64_t size() const
				{
					return n;
				}
			};

			typedef AutoArrayMultiMapValueAdapter value_adapter_type;

			libmaus2::autoarray::AutoArray< std::pair<key_type,value_type> > A;
			uint64_t f;
			bool sorted;
			libmaus2::autoarray::AutoArray< key_type > K;
			uint64_t fk;

			AutoArrayMultiMap() : A(), f(0), sorted(true), K(), fk(0) {}

			void insert(key_type const & key, value_type const & value)
			{
				if ( f == A.size() )
				{
					uint64_t const oldsize = A.size();
					uint64_t const newsize = std::max(static_cast<uint64_t>(1),oldsize<<1);
					A.resize(newsize);
					K.resize(newsize);
				}
				assert ( f < A.size() );
				K[f  ] = key;
				A[f++] = std::pair<key_type,value_type>(key,value);
				sorted = false;
			}

			void sort()
			{
				if ( ! sorted )
				{
					std::stable_sort(A.begin(),A.begin()+f,FirstComparator());
					std::stable_sort(K.begin(),K.begin()+f);
					fk = (std::unique(K.begin(),K.begin()+f)-K.begin());
					sorted = true;
				}
			}

			void clear()
			{
				f = 0;
				fk = 0;
				sorted = true;
			}

			iterator lower_bound(key_type const & key)
			{
				assert ( sorted );
				return std::lower_bound(
					A.begin(),
					A.begin()+f,
					std::pair<key_type,value_type>(key,value_type()),
					FirstComparator()
				);
			}

			iterator upper_bound(key_type const & key)
			{
				assert ( sorted );
				return std::upper_bound(
					A.begin(),
					A.begin()+f,
					std::pair<key_type,value_type>(key,value_type()),
					FirstComparator()
				);
			}

			std::pair<iterator,iterator> equal_range(key_type const & key)
			{
				return std::pair<iterator,iterator>(lower_bound(key),upper_bound(key));
			}

			const_iterator lower_bound(key_type const & key) const
			{
				assert ( sorted );
				return std::lower_bound(
					A.begin(),
					A.begin()+f,
					std::pair<key_type,value_type>(key,value_type()),
					FirstComparator()
				);
			}

			const_iterator upper_bound(key_type const & key) const
			{
				assert ( sorted );
				return std::upper_bound(
					A.begin(),
					A.begin()+f,
					std::pair<key_type,value_type>(key,value_type()),
					FirstComparator()
				);
			}

			std::pair<const_iterator,const_iterator> equal_range(key_type const & key) const
			{
				return std::pair<const_iterator,const_iterator>(lower_bound(key),upper_bound(key));
			}

			const_key_iterator kbegin() const
			{
				assert ( sorted );
				return K.begin();
			}

			const_key_iterator kend() const
			{
				assert ( sorted );
				return K.begin() + fk;
			}

			key_iterator kbegin()
			{
				assert ( sorted );
				return K.begin();
			}

			key_iterator kend()
			{
				assert ( sorted );
				return K.begin() + fk;
			}

			value_adapter_type getVector(key_type const & key) const
			{
				std::pair<const_iterator,const_iterator> eq = equal_range(key);
				return value_adapter_type(eq.first,eq.second-eq.first);
			}
		};
	}
}
#endif
