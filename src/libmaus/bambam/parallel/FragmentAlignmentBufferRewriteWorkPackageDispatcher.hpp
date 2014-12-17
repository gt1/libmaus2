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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_FRAGMENTALIGNMENTBUFFERREWRITEWORKPACKAGEDISPATCHER_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_FRAGMENTALIGNMENTBUFFERREWRITEWORKPACKAGEDISPATCHER_HPP

#include <libmaus/bambam/parallel/FragmentAlignmentBufferRewriteWorkPackageReturnInterface.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferRewriteFragmentCompleteInterface.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferRewriteWorkPackage.hpp>
#include <libmaus/parallel/SimpleThreadWorkPackageDispatcher.hpp>

namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{

			/**
			 * block rewrite dispatcher
			 **/
			struct FragmentAlignmentBufferRewriteWorkPackageDispatcher : public libmaus::parallel::SimpleThreadWorkPackageDispatcher
			{
				FragmentAlignmentBufferRewriteWorkPackageReturnInterface & packageReturnInterface;
				FragmentAlignmentBufferRewriteFragmentCompleteInterface & fragmentCompleteInterface;
				
				FragmentAlignmentBufferRewriteWorkPackageDispatcher(
					FragmentAlignmentBufferRewriteWorkPackageReturnInterface & rpackageReturnInterface,
					FragmentAlignmentBufferRewriteFragmentCompleteInterface & rfragmentCompleteInterface
				) : packageReturnInterface(rpackageReturnInterface), fragmentCompleteInterface(rfragmentCompleteInterface)
				{
				
				}
			
				virtual void dispatch(
					libmaus::parallel::SimpleThreadWorkPackage * P, 
					libmaus::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & tpi
				)
				{
					// get type cast work package pointer
					FragmentAlignmentBufferRewriteWorkPackage * BP = dynamic_cast<FragmentAlignmentBufferRewriteWorkPackage *>(P);
					assert ( BP );
					
					BP->dispatch();
	
					fragmentCompleteInterface.fragmentAlignmentBufferRewriteFragmentComplete(BP->algn,BP->FAB,BP->j);

					// return the work package
					packageReturnInterface.returnFragmentAlignmentBufferRewriteWorkPackage(BP);
				}		
			};
		}
	}
}
#endif
