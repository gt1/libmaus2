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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_BAMBLOCKINDEXINGWORKPACKAGE_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_BAMBLOCKINDEXINGWORKPACKAGE_HPP

#include <libmaus/bambam/parallel/GenericInputControlCompressionPending.hpp>
#include <libmaus/bambam/BamIndexGenerator.hpp>
#include <libmaus/parallel/SimpleThreadWorkPackage.hpp>
			
namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{
			struct BamBlockIndexingWorkPackage : public libmaus::parallel::SimpleThreadWorkPackage
			{
				typedef BamBlockIndexingWorkPackage this_type;
				typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

				libmaus::bambam::parallel::GenericInputControlCompressionPending GICCP;
				libmaus::bambam::BamIndexGenerator * bamindexgenerator;
				
				BamBlockIndexingWorkPackage() : libmaus::parallel::SimpleThreadWorkPackage() {}               
				BamBlockIndexingWorkPackage(
					uint64_t const rpriority, 
					uint64_t const rdispatcherid, 
					libmaus::bambam::parallel::GenericInputControlCompressionPending rGICCP,
					libmaus::bambam::BamIndexGenerator * rbamindexgenerator
				)
				: libmaus::parallel::SimpleThreadWorkPackage(rpriority,rdispatcherid), GICCP(rGICCP), bamindexgenerator(rbamindexgenerator)
				{
				}

				char const * getPackageName() const
				{
					return "BamBlockIndexingWorkPackage";
				}
			};
		}
	}
}
#endif
