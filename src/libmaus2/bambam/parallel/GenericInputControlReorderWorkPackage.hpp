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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_GENERICINPUTCONTROLREORDERWORKPACKAGE_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_GENERICINPUTCONTROLREORDERWORKPACKAGE_HPP

#include <libmaus/parallel/SimpleThreadWorkPackage.hpp>
#include <libmaus/bambam/parallel/AlignmentBuffer.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBuffer.hpp>

namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{
			struct GenericInputControlReorderWorkPackage : public libmaus::parallel::SimpleThreadWorkPackage
			{
				typedef GenericInputControlReorderWorkPackage this_type;
				typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
						
				libmaus::bambam::parallel::AlignmentBuffer::shared_ptr_type in;
				libmaus::bambam::parallel::FragmentAlignmentBuffer::shared_ptr_type out;
				std::pair<uint64_t,uint64_t> I;
				uint64_t index;
						
				GenericInputControlReorderWorkPackage() : libmaus::parallel::SimpleThreadWorkPackage(), in(), out(), I(), index(0) {}
				GenericInputControlReorderWorkPackage(
					uint64_t const rpriority, 
					uint64_t const rdispatcherid, 
					libmaus::bambam::parallel::AlignmentBuffer::shared_ptr_type rin,
					libmaus::bambam::parallel::FragmentAlignmentBuffer::shared_ptr_type rout,
					std::pair<uint64_t,uint64_t> const rI,
					uint64_t const rindex
				)
				: libmaus::parallel::SimpleThreadWorkPackage(rpriority,rdispatcherid), in(rin), out(rout), I(rI), index(rindex)
				{
							
				}
						
				char const * getPackageName() const
				{
					return "GenericInputControlReorderWorkPackage";
				}
			};
		}
	}
}
#endif
