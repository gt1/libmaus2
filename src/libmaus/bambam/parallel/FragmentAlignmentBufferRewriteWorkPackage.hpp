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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_FRAGMENTALIGNMENTBUFFERREWRITEWORKPACKAGE_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_FRAGMENTALIGNMENTBUFFERREWRITEWORKPACKAGE_HPP

#include <libmaus/parallel/SimpleThreadWorkPackage.hpp>
#include <libmaus/bambam/parallel/AlignmentBuffer.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBuffer.hpp>

namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{
			struct FragmentAlignmentBufferRewriteWorkPackage  : public libmaus::parallel::SimpleThreadWorkPackage
			{
				typedef FragmentAlignmentBufferRewriteWorkPackage this_type;
				typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
				
				AlignmentBuffer::shared_ptr_type algn;
				FragmentAlignmentBuffer::shared_ptr_type FAB;
				uint64_t j;
			
				FragmentAlignmentBufferRewriteWorkPackage()
				:
					libmaus::parallel::SimpleThreadWorkPackage(),
					algn(), FAB(), j(0)
				{
				}

				FragmentAlignmentBufferRewriteWorkPackage(
					uint64_t const rpriority, 
					AlignmentBuffer::shared_ptr_type ralgn,
					FragmentAlignmentBuffer::shared_ptr_type rFAB,
					uint64_t rj,
					uint64_t const rreadDispatcherId
				)
				: 
					libmaus::parallel::SimpleThreadWorkPackage(rpriority,rreadDispatcherId), 
					algn(ralgn), FAB(rFAB), j(rj)
				{
				}
								
				char const * getPackageName() const
				{
					return "FragmentAlignmentBufferRewriteWorkPackage";
				}                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                
			};
		}
	}
}
#endif
