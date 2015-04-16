/*
    libmaus
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
#if ! defined(LIBMAUS_PARALLEL_SIMPLETHREADPOOLWORKPACKAGEFREELIST_HPP)
#define LIBMAUS_PARALLEL_SIMPLETHREADPOOLWORKPACKAGEFREELIST_HPP

#include <libmaus/parallel/PosixMutex.hpp>
#include <libmaus/parallel/PosixSpinLock.hpp>
#include <libmaus/autoarray/AutoArray.hpp>

namespace libmaus
{
	namespace parallel
	{
		template<typename _package_type>
		struct SimpleThreadPoolWorkPackageFreeList
		{
			typedef _package_type package_type;
			typedef typename package_type::unique_ptr_type package_ptr_type;
			
			libmaus::parallel::PosixSpinLock lock;
			libmaus::autoarray::AutoArray<package_ptr_type> packages;
			libmaus::autoarray::AutoArray<package_type *> freelist;
			uint64_t freelistFill;
			
			SimpleThreadPoolWorkPackageFreeList() : freelistFill(0) {}
			
			size_t size()
			{
				libmaus::parallel::ScopePosixSpinLock llock(lock);
				return packages.size();				
			}
			
			package_type * getPackage()
			{
				libmaus::parallel::ScopePosixSpinLock llock(lock);
			
				if ( ! freelistFill )
				{
					uint64_t const newlistsize = packages.size() ? 2*packages.size() : 1;
					
					libmaus::autoarray::AutoArray<package_ptr_type> newpackages(newlistsize);
					libmaus::autoarray::AutoArray<package_type *> newfreelist(newlistsize);
					
					for ( uint64_t i = 0; i < packages.size(); ++i )
					{
						newpackages[i] = UNIQUE_PTR_MOVE(packages[i]);
					}
					for ( uint64_t i = packages.size(); i < newpackages.size(); ++i )
					{
						package_ptr_type tptr(new package_type);
						newpackages[i] = UNIQUE_PTR_MOVE(tptr);
						newfreelist[freelistFill++] = newpackages[i].get();
					}
					
					packages = newpackages;
					freelist = newfreelist;
				}
				
				return freelist[--freelistFill];
			}
			
			void returnPackage(package_type * ptr)
			{
				libmaus::parallel::ScopePosixSpinLock llock(lock);
				freelist[freelistFill++] = ptr;
			}
		};
	}
}
#endif
