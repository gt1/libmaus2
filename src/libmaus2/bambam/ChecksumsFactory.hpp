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
#if ! defined(LIBMAUS2_BAMBAM_CHECKSUMSFACTORY_HPP)
#define LIBMAUS2_BAMBAM_CHECKSUMSFACTORY_HPP

#include <libmaus2/bambam/ChecksumsInterface.hpp>
#include <libmaus2/bambam/BamHeaderLowMem.hpp>

namespace libmaus2
{
	namespace bambam
	{
		struct ChecksumsFactory
		{
			static std::vector<std::string> getSupportedHashVariants();
			static std::string getSupportedHashVariantsList();
			static libmaus2::bambam::ChecksumsInterface::unique_ptr_type construct(std::string const & hash, libmaus2::bambam::BamHeader const & header);
			static libmaus2::bambam::ChecksumsInterface::unique_ptr_type construct(std::string const & hash, libmaus2::bambam::BamHeaderLowMem const & header);
			
			template<typename header_type>
			static libmaus2::bambam::ChecksumsInterface::shared_ptr_type constructShared(std::string const & hash, header_type const & header)
			{
				libmaus2::bambam::ChecksumsInterface::unique_ptr_type tptr(construct(hash,header));
				libmaus2::bambam::ChecksumsInterface::shared_ptr_type sptr(tptr.release());
				return sptr;
			}
		};
	}
}
#endif
