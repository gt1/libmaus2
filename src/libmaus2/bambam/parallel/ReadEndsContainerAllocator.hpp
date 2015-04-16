/*
    libmaus2
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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_READENDCONTAINERALLOCATOR_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_READENDCONTAINERALLOCATOR_HPP

#include <libmaus2/bambam/ReadEndsContainer.hpp>
#include <libmaus2/util/TempFileRemovalContainer.hpp>

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{
			struct ReadEndsContainerAllocator
			{
				uint64_t blocksize;
				std::string filenamebase;
				libmaus2::parallel::LockedCounter nextid;
				
				ReadEndsContainerAllocator() : blocksize(0), filenamebase(), nextid(0) {}
				ReadEndsContainerAllocator(
					uint64_t const & rblocksize,
					std::string const & rfilenamebase
				) : blocksize(rblocksize), filenamebase(rfilenamebase), nextid(0) {}
				
				libmaus2::bambam::ReadEndsContainer::shared_ptr_type operator()()
				{
					uint64_t const id = nextid.increment();
					std::string const datafilename = filenamebase + libmaus2::util::NumberSerialisation::formatNumber(id,6);
					std::string const indexfilename = datafilename + ".index";
					libmaus2::util::TempFileRemovalContainer::addTempFile(datafilename);
					libmaus2::util::TempFileRemovalContainer::addTempFile(indexfilename);
					libmaus2::bambam::ReadEndsContainer::shared_ptr_type ptr(new ReadEndsContainer(blocksize,datafilename,indexfilename));
					return ptr;
				}
			};
		}
	}
}
#endif
