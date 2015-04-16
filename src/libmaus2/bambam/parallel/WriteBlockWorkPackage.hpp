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
#if ! defined(LIBMAUS2_BAMBAM_PARALLEL_WRITEBLOCKWORKPACKAGE_HPP)
#define LIBMAUS2_BAMBAM_PARALLEL_WRITEBLOCKWORKPACKAGE_HPP

#include <libmaus2/parallel/SimpleThreadWorkPackage.hpp>
#include <libmaus2/bambam/parallel/WritePendingObject.hpp>

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{
			struct WriteBlockWorkPackage : public libmaus2::parallel::SimpleThreadWorkPackage
			{
				typedef WriteBlockWorkPackage this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				WritePendingObject obj;			
	
				WriteBlockWorkPackage() : libmaus2::parallel::SimpleThreadWorkPackage(), obj()
				{
				}
				
				WriteBlockWorkPackage(
					uint64_t const rpriority,
					WritePendingObject const & robj,
					uint64_t const rdecompressDispatcherId
				) : libmaus2::parallel::SimpleThreadWorkPackage(rpriority,rdecompressDispatcherId), obj(robj)
				{
				}
			
				char const * getPackageName() const
				{
					return "WriteBlockWorkPackage";
				}
			};
		}
	}
}
#endif
