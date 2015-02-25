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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_GENERICINPUTCONTROLBLOCKCOMPRESSIONWORKPACKAGE_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_GENERICINPUTCONTROLBLOCKCOMPRESSIONWORKPACKAGE_HPP

#include <libmaus/bambam/parallel/GenericInputControlCompressionPending.hpp>
#include <libmaus/parallel/SimpleThreadWorkPackage.hpp>

namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{
			struct GenericInputControlBlockCompressionWorkPackage : public libmaus::parallel::SimpleThreadWorkPackage
			{
				typedef GenericInputControlBlockCompressionWorkPackage this_type;
				typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
						
				GenericInputControlCompressionPending GICCP;
						
				GenericInputControlBlockCompressionWorkPackage() : libmaus::parallel::SimpleThreadWorkPackage(), GICCP() {}
				GenericInputControlBlockCompressionWorkPackage(
					uint64_t const rpriority, 
					uint64_t const rdispatcherid, 
					GenericInputControlCompressionPending rGICCP
				)
				: libmaus::parallel::SimpleThreadWorkPackage(rpriority,rdispatcherid), GICCP(rGICCP)
				{
							
				}
						
				char const * getPackageName() const
				{
					return "GenericInputControlBlockCompressionWorkPackage";
				}
			};
		}
	}
}
#endif
