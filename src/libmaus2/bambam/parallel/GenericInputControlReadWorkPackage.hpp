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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_GENERICINPUTCONTROLREADWORKPACKAGE_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_GENERICINPUTCONTROLREADWORKPACKAGE_HPP

#include <libmaus2/bambam/parallel/GenericInputSingleData.hpp>
#include <libmaus2/parallel/SimpleThreadWorkPackage.hpp>

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{
			struct GenericInputControlReadWorkPackage : public libmaus2::parallel::SimpleThreadWorkPackage
			{
				typedef GenericInputControlReadWorkPackage this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
			
				GenericInputSingleDataReadBase * data;
			
				GenericInputControlReadWorkPackage() : libmaus2::parallel::SimpleThreadWorkPackage(), data(0) {}
				GenericInputControlReadWorkPackage(uint64_t const rpriority, uint64_t const rdispatcherid, GenericInputSingleDataReadBase * rdata)
				: libmaus2::parallel::SimpleThreadWorkPackage(rpriority,rdispatcherid), data(rdata)
				{
				
				}
			
				char const * getPackageName() const
				{
					return "GenericInputControlReadWorkPackage";
				}
			};
		}
	}
}
#endif
