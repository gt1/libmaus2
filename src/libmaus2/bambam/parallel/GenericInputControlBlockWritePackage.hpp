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
#if ! defined(LIBMAUS2_BAMBAM_PARALLEL_GENERICINPUTCONTROLBLOCKWRITEPACKAGE_HPP)
#define LIBMAUS2_BAMBAM_PARALLEL_GENERICINPUTCONTROLBLOCKWRITEPACKAGE_HPP

#include <libmaus2/bambam/parallel/GenericInputControlCompressionPending.hpp>
#include <libmaus2/parallel/SimpleThreadWorkPackage.hpp>
#include <ostream>

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{
			struct GenericInputControlBlockWritePackage : public libmaus2::parallel::SimpleThreadWorkPackage
			{
				typedef GenericInputControlBlockWritePackage this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				libmaus2::bambam::parallel::GenericInputControlCompressionPending GICCP;
				std::ostream * out;

				GenericInputControlBlockWritePackage() : libmaus2::parallel::SimpleThreadWorkPackage(), GICCP(), out(0) {}
				GenericInputControlBlockWritePackage(uint64_t const rpriority, uint64_t const rdispatcherid, libmaus2::bambam::parallel::GenericInputControlCompressionPending rGICCP, std::ostream * rout)
				: libmaus2::parallel::SimpleThreadWorkPackage(rpriority,rdispatcherid), GICCP(rGICCP), out(rout)
				{
				}
				char const * getPackageName() const
				{
					return "GenericInputControlBlockWritePackage";
				}
			};
		}
	}
}
#endif
