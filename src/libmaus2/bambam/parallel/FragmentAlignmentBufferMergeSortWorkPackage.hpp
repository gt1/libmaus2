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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_FRAGMENTALIGNMENTBUFFERMERGESORTWORKPACKAGE_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_FRAGMENTALIGNMENTBUFFERMERGESORTWORKPACKAGE_HPP

#include <libmaus/bambam/parallel/FragmentAlignmentBuffer.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferSortContextMergePackageFinished.hpp>
#include <libmaus/parallel/SimpleThreadWorkPackage.hpp>
#include <libmaus/sorting/ParallelStableSort.hpp>

namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{
			template<typename _order_type>
			struct FragmentAlignmentBufferMergeSortWorkPackage : public libmaus::parallel::SimpleThreadWorkPackage
			{
				typedef uint8_t ** iterator;
				typedef _order_type order_type;
				
				typedef FragmentAlignmentBufferMergeSortWorkPackage<order_type> this_type;
				typedef typename libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef typename libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
							
				typedef libmaus::sorting::ParallelStableSort::MergeRequest<iterator,order_type> request_type;
			
				request_type * request;
				FragmentAlignmentBufferSortContextMergePackageFinished * mergedInterface;
	
				FragmentAlignmentBufferMergeSortWorkPackage() : libmaus::parallel::SimpleThreadWorkPackage(), request(0) {}
				
				FragmentAlignmentBufferMergeSortWorkPackage(
					uint64_t const rpriority, 
					request_type * rrequest,
					FragmentAlignmentBufferSortContextMergePackageFinished * rmergedInterface,
					uint64_t const rdispatcherId
				)
				: libmaus::parallel::SimpleThreadWorkPackage(rpriority,rdispatcherId), request(rrequest), mergedInterface(rmergedInterface)
				{
				}
			
				char const * getPackageName() const
				{
					return "FragmentAlignmentBufferMergeSortWorkPackage";
				}
			};
		}
	}
}
#endif
