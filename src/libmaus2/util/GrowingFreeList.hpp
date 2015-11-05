/*
    libmaus2
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#if ! defined(LIBMAUS2_UTIL_GROWINGFREELIST_HPP)
#define LIBMAUS2_UTIL_GROWINGFREELIST_HPP

#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/util/FreeList.hpp>

namespace libmaus2
{
	namespace util
	{
		template<
			typename _element_type,
			typename _allocator_type = libmaus2::util::FreeListDefaultAllocator<_element_type>,
			typename _type_info_type = libmaus2::util::FreeListDefaultTypeInfo<_element_type>
		>
		struct GrowingFreeList
		{
			typedef _element_type element_type;
			typedef _allocator_type allocator_type;
			typedef _type_info_type type_info_type;
			typedef GrowingFreeList<element_type,allocator_type,type_info_type> this_type;

			private:
			libmaus2::autoarray::AutoArray<typename type_info_type::pointer_type> alloclist;
			libmaus2::autoarray::AutoArray<typename type_info_type::pointer_type> freelist;
			uint64_t freelistfill;
			allocator_type allocator;

			void cleanup()
			{
				for ( uint64_t i = 0; i < alloclist.size(); ++i )
					alloclist[i] = type_info_type::deallocate(alloclist[i]);
				alloclist = libmaus2::autoarray::AutoArray<typename type_info_type::pointer_type>(0);
				freelist = libmaus2::autoarray::AutoArray<typename type_info_type::pointer_type>(0);
				freelistfill = 0;
			}

			public:
			GrowingFreeList(allocator_type rallocator = allocator_type())
			: alloclist(0), freelist(0), freelistfill(0), allocator(rallocator)
			{

			}

			size_t capacity()
			{
				return alloclist.size();
			}

			size_t free()
			{
				return freelistfill;
			}

			~GrowingFreeList()
			{
				cleanup();
			}

			size_t getAllSize() const
			{
				return freelistfill;
			}

			std::vector < typename type_info_type::pointer_type > getAll()
			{
				std::vector < typename type_info_type::pointer_type > V;
				while ( freelistfill )
					V.push_back(freelist[--freelistfill]);
				return V;
			}

			void put(std::vector < typename type_info_type::pointer_type > V)
			{
				for ( uint64_t i = 0; i < V.size(); ++i )
					put(V[i]);
			}

			typename type_info_type::pointer_type get()
			{
				if ( ! freelistfill )
				{
					// allocate more alignment objects
					libmaus2::autoarray::AutoArray<typename type_info_type::pointer_type> nalloclist(
						std::max(
							static_cast<uint64_t>(1),
							static_cast<uint64_t>(2*alloclist.size())
						)
						,false
					);

					std::copy(alloclist.begin(),alloclist.end(),nalloclist.begin());
					typename type_info_type::pointer_type nullp = type_info_type::getNullPointer();
					std::fill(nalloclist.begin()+alloclist.size(),nalloclist.end(),nullp);

					for ( typename type_info_type::pointer_type* p = nalloclist.begin()+alloclist.size();
						p != nalloclist.end(); ++p )
						*p = allocator();

					libmaus2::autoarray::AutoArray<typename type_info_type::pointer_type> nfreelist(
						std::max(
							static_cast<uint64_t>(1),
							static_cast<uint64_t>(2*freelist.size())
						)
						,false
					);

					std::copy(freelist.begin(),freelist.end(),nfreelist.begin());
					std::fill(nfreelist.begin()+freelist.size(),nfreelist.end(),nullp);

					freelist = nfreelist;

					for ( typename type_info_type::pointer_type* p = nalloclist.begin()+alloclist.size();
						p != nalloclist.end(); ++p )
						freelist[freelistfill++] = *p;

					alloclist = nalloclist;
				}

				return freelist[--freelistfill];
			}

			void put(typename type_info_type::pointer_type p)
			{
				freelist[freelistfill++] = p;
			}

			size_t byteSize()
			{
				typedef typename type_info_type::pointer_type pointer_type;
				std::vector<pointer_type> V = getAll();
				size_t s = 0;
				for ( uint64_t i = 0; i < V.size(); ++i )
					s += V[i]->byteSize();
				put(V);

				s += alloclist.byteSize();
				s += freelist.byteSize();
				s += sizeof(freelistfill);
				s += sizeof(allocator);

				return s;
			}

			allocator_type & getAllocator()
			{
				return allocator;
			}
		};
	}
}
#endif
