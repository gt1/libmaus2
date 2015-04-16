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
#if ! defined(LIBMAUS_BAMBAM_CHECKSUMSINTERFACEALLOCATOR_HPP)
#define LIBMAUS_BAMBAM_CHECKSUMSINTERFACEALLOCATOR_HPP

#include <libmaus2/bambam/ChecksumsFactory.hpp>

namespace libmaus2
{
	namespace bambam
	{
		struct ChecksumsInterfaceAllocator
		{
			std::string hash;
			libmaus2::bambam::BamHeaderLowMem * header;
			
			ChecksumsInterfaceAllocator(std::string rhash = std::string(), libmaus2::bambam::BamHeaderLowMem * rheader = 0)
			: hash(rhash), header(rheader)
			{
			
			}
		
			libmaus2::bambam::ChecksumsInterface::shared_ptr_type operator()()
			{
				libmaus2::bambam::ChecksumsInterface::shared_ptr_type sptr(ChecksumsFactory::constructShared(hash,*header));
				return sptr;
			}
		};
	}
}
#endif
