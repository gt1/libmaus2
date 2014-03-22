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
#if ! defined(LIBMAUS_PARALLEL_SIMPLETHREADWORKPACKAGE_HPP)
#define LIBMAUS_PARALLEL_SIMPLETHREADWORKPACKAGE_HPP

#include <libmaus/util/unique_ptr.hpp>
#include <libmaus/util/shared_ptr.hpp>
#include <ostream>

namespace libmaus
{
	namespace parallel
	{
		struct SimpleThreadWorkPackage
		{
			typedef SimpleThreadWorkPackage this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
		
			uint64_t priority;
			uint64_t dispatcherid;
			uint64_t packageid;
	
			SimpleThreadWorkPackage()
			: priority(0), dispatcherid(0), packageid(0)
			{
			
			}		
			SimpleThreadWorkPackage(uint64_t const rpriority, uint64_t const rdispatcherid, uint64_t const rpackageid = 0)
			: priority(rpriority), dispatcherid(rdispatcherid), packageid(rpackageid)
			{
			
			}
			void reset(uint64_t const rpriority, uint64_t const rdispatcherid, uint64_t const rpackageid = 0)
			{
				priority = rpriority;
				dispatcherid = rdispatcherid;
				packageid = rpackageid;
			}
			virtual ~SimpleThreadWorkPackage() {}
		};

		std::ostream & operator<<(std::ostream & out, SimpleThreadWorkPackage const & T);
		std::ostream & operator<<(std::ostream & out, SimpleThreadWorkPackage const * T);
	}
}
#endif
