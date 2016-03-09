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
#if ! defined(LIBMAUS2_UTIL_CONTAINERELEMENTFREELIST_HPP)
#define LIBMAUS2_UTIL_CONTAINERELEMENTFREELIST_HPP

#include <libmaus2/autoarray/AutoArray.hpp>

namespace libmaus2
{
	namespace util
	{
		template<typename _element_type>
		struct ContainerElementFreeList
		{
			typedef _element_type element_type;
			typedef ContainerElementFreeList<element_type> this_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			ContainerElementFreeList()
			: Aelements(), Afreelist(), freelisthigh(0)
			{

			}

			// nodes
			libmaus2::autoarray::AutoArray< element_type > Aelements;

			// node free list
			libmaus2::autoarray::AutoArray<uint64_t> Afreelist;
			// high water in free list
			uint64_t freelisthigh;

			/**
			 * get unused node (from free list or newly allocated)
			 **/
			uint64_t getNewNode()
			{
				if ( !freelisthigh )
				{
					uint64_t const oldsize = Aelements.size();
					Aelements.bump();
					uint64_t const newsize = Aelements.size();
					uint64_t const dif = newsize - oldsize;
					Afreelist.ensureSize(dif);
					for ( uint64_t i = 0; i < dif; ++i )
						Afreelist[freelisthigh++] = i+oldsize;
				}

				uint64_t const node = Afreelist[--freelisthigh];

				return node;
			}

			/**
			 * add node to the free list
			 **/
			void deleteNode(uint64_t node)
			{
				Afreelist.push(freelisthigh,node);
			}

			libmaus2::autoarray::AutoArray< element_type > & getElementsArray()
			{
				return Aelements;
			}
		};
	}
}
#endif
