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
#if ! defined(LIBMAUS_PARALLEL_LOCKEDCOUNTER_HPP)
#define LIBMAUS_PARALLEL_LOCKEDCOUNTER_HPP

#include <libmaus/parallel/PosixSpinLock.hpp>

namespace libmaus
{
	namespace parallel
	{
		struct LockedCounter
		{
			private:
			libmaus::parallel::PosixSpinLock lock;
			volatile uint64_t v;
						
			public:
			LockedCounter(uint64_t const rv = 0) : v(rv) {}
			LockedCounter(LockedCounter const & o)
			{
				v = o.v;
			}
			LockedCounter & operator=(LockedCounter const & o)
			{
				v = o.v;
				return *this;
			}
			
			uint64_t increment()
			{
				libmaus::parallel::ScopePosixSpinLock slock(lock);
				v += 1;
				return v;			
			}
			
			LockedCounter & operator++(int)
			{
				libmaus::parallel::ScopePosixSpinLock slock(lock);
				v += 1;
				return *this;
			}
			LockedCounter & operator--(int)
			{
				libmaus::parallel::ScopePosixSpinLock slock(lock);
				v -= 1;
				return *this;
			}
			LockedCounter & operator+=(uint64_t const o)
			{
				libmaus::parallel::ScopePosixSpinLock slock(lock);
				v += o;
				return *this;
			}
			LockedCounter & operator-=(uint64_t const o)
			{
				libmaus::parallel::ScopePosixSpinLock slock(lock);
				v -= o;
				return *this;
			}
			operator uint64_t()
			{
				libmaus::parallel::ScopePosixSpinLock slock(lock);
				uint64_t const lv = v;
				return lv;
			}
		};
	}
}
#endif
