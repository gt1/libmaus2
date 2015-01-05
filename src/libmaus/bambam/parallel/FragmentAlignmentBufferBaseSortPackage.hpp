/*
    libmaus
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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_ALIGNMENTREWRITEPOSSORTBASESORTPACKAGE_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_ALIGNMENTREWRITEPOSSORTBASESORTPACKAGE_HPP

#include <libmaus/bambam/parallel/FragmentAlignmentBufferSortContextBaseBlockSortedInterface.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBuffer.hpp>
#include <libmaus/parallel/SimpleThreadWorkPackage.hpp>
#include <libmaus/sorting/ParallelStableSort.hpp>

namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{
			template<typename _order_type>
			struct FragmentAlignmentBufferBaseSortPackage : public libmaus::parallel::SimpleThreadWorkPackage
			{
				typedef _order_type order_type;
				typedef uint8_t ** iterator;
			
				typedef FragmentAlignmentBufferBaseSortPackage<order_type> this_type;
				typedef typename libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef typename libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
				typedef libmaus::sorting::ParallelStableSort::BaseSortRequest<iterator,order_type> request_type;
				request_type * request;
				FragmentAlignmentBufferSortContextBaseBlockSortedInterface * blockSortedInterface;
	
				FragmentAlignmentBufferBaseSortPackage() : libmaus::parallel::SimpleThreadWorkPackage(), request(0) {}
				
				FragmentAlignmentBufferBaseSortPackage(
					uint64_t const rpriority, 
					request_type * rrequest,
					FragmentAlignmentBufferSortContextBaseBlockSortedInterface * rblockSortedInterface,
					uint64_t const rdispatcherId
				)
				: libmaus::parallel::SimpleThreadWorkPackage(rpriority,rdispatcherId), request(rrequest), blockSortedInterface(rblockSortedInterface)
				{
				}
			
				char const * getPackageName() const
				{
					return "FragmentAlignmentBufferBaseSortPackage";
				}
			};
		}
	}
}
#endif
