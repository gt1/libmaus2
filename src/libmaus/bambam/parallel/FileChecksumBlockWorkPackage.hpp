/*
    libmaus
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2015 Genome Research Limited

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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_FILECHECKSUMBLOCKWORKPACKAGE_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_FILECHECKSUMBLOCKWORKPACKAGE_HPP

#include <libmaus/parallel/SimpleThreadWorkPackage.hpp>
#include <libmaus/bambam/parallel/GenericInputControlCompressionPending.hpp>

namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{			
			template<typename _file_checksum_type>
			struct FileChecksumBlockWorkPackage : public libmaus::parallel::SimpleThreadWorkPackage
			{
				typedef _file_checksum_type file_checksum_type;
				typedef FileChecksumBlockWorkPackage<file_checksum_type> this_type;
				typedef typename libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef typename libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
				
				libmaus::bambam::parallel::GenericInputControlCompressionPending GICCP;
				file_checksum_type * checksum;

				FileChecksumBlockWorkPackage() : libmaus::parallel::SimpleThreadWorkPackage(), GICCP(), checksum(0) {}
				FileChecksumBlockWorkPackage(
					uint64_t const rpriority, 
					uint64_t const rdispatcherid, 
					libmaus::bambam::parallel::GenericInputControlCompressionPending rGICCP,
					file_checksum_type * rchecksum
				)
				: libmaus::parallel::SimpleThreadWorkPackage(rpriority,rdispatcherid), GICCP(rGICCP), checksum(rchecksum)
				{
				}
                        
				virtual char const * getPackageName() const { return "FileChecksumBlockWorkPackage"; }
			};
		}
	}
}
#endif
