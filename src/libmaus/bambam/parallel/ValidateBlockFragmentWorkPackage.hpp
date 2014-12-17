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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_VALIDATEBLOCKFRAGMENTWORKPACKAGE_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_VALIDATEBLOCKFRAGMENTWORKPACKAGE_HPP

#include <libmaus/bambam/parallel/AlignmentBuffer.hpp>
#include <libmaus/bambam/parallel/ValidationFragment.hpp>
#include <libmaus/parallel/SimpleThreadWorkPackage.hpp>

namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{
			struct ValidateBlockFragmentWorkPackage : public libmaus::parallel::SimpleThreadWorkPackage
			{
				typedef ValidateBlockFragmentWorkPackage this_type;
				typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
				AlignmentBuffer::shared_ptr_type parseBlock;
				ValidationFragment fragment;
	
				ValidateBlockFragmentWorkPackage() : libmaus::parallel::SimpleThreadWorkPackage(), parseBlock() {}
				
				ValidateBlockFragmentWorkPackage(
					uint64_t const rpriority, 
					ValidationFragment const & rfragment,
					uint64_t const rparseDispatcherId
				)
				: libmaus::parallel::SimpleThreadWorkPackage(rpriority,rparseDispatcherId), fragment(rfragment)
				{
				}
			
				char const * getPackageName() const
				{
					return "ValidateBlockFragmentWorkPackage";
				}
				
				bool dispatch()
				{
					return fragment.dispatch();
				}
			};
		}
	}
}
#endif
