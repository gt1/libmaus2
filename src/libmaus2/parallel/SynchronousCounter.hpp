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

#if ! defined(SYNCHRONOUSCOUNTER_HPP)
#define SYNCHRONOUSCOUNTER_HPP

#include <libmaus2/LibMausConfig.hpp>
#include <libmaus2/parallel/OMPLock.hpp>
#include <ostream>

namespace libmaus2
{
	namespace parallel
	{
		template<typename _value_type>
		struct SynchronousCounter
		{
			typedef _value_type value_type;
			typedef SynchronousCounter<value_type> this_type;
		
			#if ! defined(LIBMAUS2_HAVE_SYNC_OPS)
			mutable ::libmaus2::parallel::OMPLock lock;
			#endif
			volatile value_type cnt;
			
			SynchronousCounter(value_type const rcnt = value_type()) : cnt(rcnt) {}
			SynchronousCounter(SynchronousCounter const & o) : cnt(o.cnt) {}
			SynchronousCounter & operator=(SynchronousCounter const & o)
			{
				if ( this != &o )
					this->cnt = o.cnt;
				return *this;
			}
			
			value_type operator++()
			{
				#if defined(LIBMAUS2_HAVE_SYNC_OPS)
				return __sync_add_and_fetch(&cnt,1);
				#else
				value_type lcnt;
				lock.lock();
				lcnt = ++cnt;
				lock.unlock();

				return lcnt;
				#endif
			}
			
			value_type operator+=(value_type const v)
			{
				#if defined(LIBMAUS2_HAVE_SYNC_OPS)
				return __sync_add_and_fetch(&cnt,v);
				#else
				value_type lcnt;
				lock.lock();
				cnt += v;
				lcnt = cnt;
				lock.unlock();
				return lcnt;
				#endif
			
			}

			value_type operator++(int)
			{
				#if defined(LIBMAUS2_HAVE_SYNC_OPS)
				return __sync_fetch_and_add(&cnt,1);
				#else
				value_type lcnt;
				lock.lock();
				lcnt = cnt++;
				lock.unlock();
				return lcnt;
				#endif
			}
			
			value_type get() const
			{
				#if defined(LIBMAUS2_HAVE_SYNC_OPS)
				return cnt;
				#else
				value_type lcnt;
				lock.lock();
				lcnt = cnt;
				lock.unlock();		
				
				return lcnt;
				#endif
			}
			
			operator value_type() const
			{
				return get();
			}
		};
		
		template<typename value_type>
		inline std::ostream & operator<<(std::ostream & out, SynchronousCounter<value_type> const & S)
		{
			out << S.get();
			return out;
		}
	}
}
#endif
