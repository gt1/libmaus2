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
#if ! defined(LIBMAUS2_BAMBAM_PARALLEL_FRAGMENTALIGNMENTBUFFERMERGESORTPACKAGE_HPP)
#define LIBMAUS2_BAMBAM_PARALLEL_FRAGMENTALIGNMENTBUFFERMERGESORTPACKAGE_HPP

#include <libmaus2/bambam/parallel/FragmentAlignmentBuffer.hpp>
#include <libmaus2/bambam/parallel/FragmentAlignmentBufferSortContextMergePackageFinished.hpp>
#include <libmaus2/parallel/SimpleThreadWorkPackage.hpp>
#include <libmaus2/sorting/ParallelStableSort.hpp>

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{
			template<typename _order_type>
			struct FragmentAlignmentBufferMergeSortPackage : public libmaus2::parallel::SimpleThreadWorkPackage
			{
				typedef uint8_t ** iterator;
				typedef _order_type order_type;

				typedef FragmentAlignmentBufferMergeSortPackage<order_type> this_type;
				typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef typename libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				typedef libmaus2::sorting::ParallelStableSort::MergeRequest<iterator,order_type> request_type;

				request_type * request;
				FragmentAlignmentBufferSortContextMergePackageFinished * mergedInterface;

				FragmentAlignmentBufferMergeSortPackage() : libmaus2::parallel::SimpleThreadWorkPackage(), request(0) {}

				FragmentAlignmentBufferMergeSortPackage(
					uint64_t const rpriority,
					request_type * rrequest,
					FragmentAlignmentBufferSortContextMergePackageFinished * rmergedInterface,
					uint64_t const rdispatcherId
				)
				: libmaus2::parallel::SimpleThreadWorkPackage(rpriority,rdispatcherId), request(rrequest), mergedInterface(rmergedInterface)
				{
				}

				char const * getPackageName() const
				{
					return "FragmentAlignmentBufferMergeSortPackage";
				}
			};
		}
	}
}
#endif
