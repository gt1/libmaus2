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
#if ! defined(LIBMAUS2_BAMBAM_PARALLEL_GENERICINPUTMERGEWORKPACKAGE_HPP)
#define LIBMAUS2_BAMBAM_PARALLEL_GENERICINPUTMERGEWORKPACKAGE_HPP

#include <libmaus2/util/FiniteSizeHeap.hpp>
#include <libmaus2/parallel/SimpleThreadWorkPackage.hpp>
#include <libmaus2/bambam/parallel/GenericInputSingleData.hpp>
#include <libmaus2/bambam/parallel/GenericInputControlMergeHeapEntry.hpp>
#include <libmaus2/bambam/parallel/AlignmentBuffer.hpp>

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{
			template<typename _heap_element_type>
			struct GenericInputMergeWorkPackage : public libmaus2::parallel::SimpleThreadWorkPackage
			{
				typedef _heap_element_type heap_element_type;
				typedef GenericInputMergeWorkPackage<heap_element_type> this_type;
				typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef typename libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				libmaus2::autoarray::AutoArray<GenericInputSingleData::unique_ptr_type> * data;
				libmaus2::util::FiniteSizeHeap<heap_element_type> * mergeheap;
				libmaus2::bambam::parallel::AlignmentBuffer::shared_ptr_type algn;

				GenericInputMergeWorkPackage() : libmaus2::parallel::SimpleThreadWorkPackage(), data(0), mergeheap(0) {}
				GenericInputMergeWorkPackage(
					uint64_t const rpriority,
					uint64_t const rdispatcherid,
					libmaus2::autoarray::AutoArray<GenericInputSingleData::unique_ptr_type> * rdata,
					libmaus2::util::FiniteSizeHeap<heap_element_type> * rmergeheap,
					libmaus2::bambam::parallel::AlignmentBuffer::shared_ptr_type ralgn
				)
				: libmaus2::parallel::SimpleThreadWorkPackage(rpriority,rdispatcherid), data(rdata), mergeheap(rmergeheap), algn(ralgn)
				{

				}

				char const * getPackageName() const
				{
					return "GenericInputMergeWorkPackage";
				}
			};
		}
	}
}
#endif
