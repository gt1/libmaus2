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
#if ! defined(LIBMAUS_BAMBAM_BAMPARALLELDECODINGMERGESORTWORKPACKAGEDISPATCHER_HPP)
#define LIBMAUS_BAMBAM_BAMPARALLELDECODINGMERGESORTWORKPACKAGEDISPATCHER_HPP

#include <libmaus/bambam/BamParallelDecodingAlignmentRewritePosMergeSortPackage.hpp>
#include <libmaus/bambam/BamParallelDecodingBaseMergeSortWorkPackageReturnInterface.hpp>
#include <libmaus/parallel/SimpleThreadWorkPackageDispatcher.hpp>

namespace libmaus
{
	namespace bambam
	{
		// dispatcher for block merging
		template<typename _order_type>
		struct BamParallelDecodingMergeSortWorkPackageDispatcher : public libmaus::parallel::SimpleThreadWorkPackageDispatcher
		{
			typedef _order_type order_type;
		
			BamParallelDecodingBaseMergeSortWorkPackageReturnInterface<order_type> & packageReturnInterface;
			
			BamParallelDecodingMergeSortWorkPackageDispatcher(
				BamParallelDecodingBaseMergeSortWorkPackageReturnInterface<order_type> & rpackageReturnInterface
			) : packageReturnInterface(rpackageReturnInterface) 
			{
			}
		
			virtual void dispatch(
				libmaus::parallel::SimpleThreadWorkPackage * P, 
				libmaus::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & tpi
			)
			{
				BamParallelDecodingAlignmentRewritePosMergeSortPackage<order_type> * BP = dynamic_cast<BamParallelDecodingAlignmentRewritePosMergeSortPackage<order_type> *>(P);
				assert ( BP );
				
				BP->request->dispatch();
				BP->mergedInterface->mergePackageFinished();
				
				// return the work package				
				packageReturnInterface.putBamParallelDecodingMergeSortWorkPackage(BP);				
			}		
		};
	}
}
#endif
