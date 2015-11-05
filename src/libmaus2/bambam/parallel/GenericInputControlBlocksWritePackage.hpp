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
#if ! defined(LIBMAUS2_BAMBAM_PARALLEL_GENERICINPUTCONTROLBLOCKSWRITEPACKAGE_HPP)
#define LIBMAUS2_BAMBAM_PARALLEL_GENERICINPUTCONTROLBLOCKSWRITEPACKAGE_HPP

#include <libmaus2/bambam/parallel/GenericInputControlCompressionPending.hpp>
#include <libmaus2/parallel/SimpleThreadWorkPackage.hpp>
#include <ostream>

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{
			struct GenericInputControlBlocksWritePackage : public libmaus2::parallel::SimpleThreadWorkPackage
			{
				typedef GenericInputControlBlocksWritePackage this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				std::vector<libmaus2::bambam::parallel::GenericInputControlCompressionPending> GICCPV;
				std::ostream * out;

				GenericInputControlBlocksWritePackage() : libmaus2::parallel::SimpleThreadWorkPackage(), GICCPV(), out(0) {}
				GenericInputControlBlocksWritePackage(uint64_t const rpriority, uint64_t const rdispatcherid,
					std::vector<libmaus2::bambam::parallel::GenericInputControlCompressionPending> rGICCPV, std::ostream * rout)
				: libmaus2::parallel::SimpleThreadWorkPackage(rpriority,rdispatcherid), GICCPV(rGICCPV), out(rout)
				{
				}
				char const * getPackageName() const
				{
					return "GenericInputControlBlocksWritePackage";
				}
			};
		}
	}
}
#endif
