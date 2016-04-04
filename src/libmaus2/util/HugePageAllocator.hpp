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
#if ! defined(LIBMAUS2_UTIL_HUGEPAGEALLOCATOR_HPP)
#define LIBMAUS2_UTIL_HUGEPAGEALLOCATOR_HPP

#include <libmaus2/util/HugePages.hpp>

namespace libmaus2
{
	namespace util
	{
		template<typename _T>
		struct HugePageAllocator
		{
			typedef _T T;
			typedef T value_type;
			typedef value_type * pointer;
			typedef value_type const * const_pointer;
			typedef value_type & reference;
			typedef value_type const & const_reference;
			typedef ::std::size_t size_type;
			typedef ::std::ptrdiff_t difference_type;

			HugePages & HP;

			HugePageAllocator() : HP(HugePages::getHugePageObject()) {}
			HugePageAllocator(HugePageAllocator<value_type> &) : HP(HugePages::getHugePageObject()) {}
			template<typename O> HugePageAllocator(HugePageAllocator<O> &) : HP(HugePages::getHugePageObject()) {}
			~HugePageAllocator() throw() {}

			/**
			 * maximum supported size for allocations
			 **/
			size_type max_size () const throw()
			{
				return std::numeric_limits< ::std::size_t >::max() / sizeof(value_type);
			}

			// allocate object without initialising them
			pointer allocate (size_type n, void const * = 0)
			{
				void * v = HP.malloc(n * sizeof(value_type), std::min(sizeof(value_type),static_cast< ::std::size_t>(4096ull)));

				if ( ! v )
					throw ::std::bad_alloc();

				return reinterpret_cast<pointer>(v);
			}

			// free memory without calling destructor
			void deallocate(pointer p, size_type)
			{
				HP.free(p);
			}

			// destruct object pointed to by p
			void destroy(pointer p)
			{
				// call destructor
				p->~value_type();
			}

			// construct
			void construct (pointer p, value_type const & v)
			{
				new((void*)p)value_type(v);
			}

			// comparison
			template<typename O>
			bool operator==(HugePageAllocator<O> const &) const
			{
				return true;
			}

			template<typename O>
			bool operator!=(HugePageAllocator<O> const &) const
			{
				return false;
			}

			pointer address (reference v) const
			{
				return &v;
			}

			const_pointer address (const_reference v) const
			{
				return &v;
			}

			template <typename O>
			struct rebind
			{
				typedef HugePageAllocator<O> other;
			};
		};
	}
}
#endif
