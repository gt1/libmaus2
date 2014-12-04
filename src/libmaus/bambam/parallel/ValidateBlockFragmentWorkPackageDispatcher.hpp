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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_VALIDATEBLOCKWORKPACKAGEDISPATCHER_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_VALIDATEBLOCKWORKPACKAGEDISPATCHER_HPP

#include <libmaus/bambam/parallel/ValidateBlockFragmentPackageReturnInterface.hpp>
#include <libmaus/bambam/parallel/ValidateBlockFragmentAddPendingInterface.hpp>
#include <libmaus/bambam/parallel/ValidateBlockFragmentWorkPackage.hpp>

namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{
			// dispatcher for block validation
			struct ValidateBlockFragmentWorkPackageDispatcher : public libmaus::parallel::SimpleThreadWorkPackageDispatcher
			{
				ValidateBlockFragmentPackageReturnInterface   & packageReturnInterface;
				ValidateBlockFragmentAddPendingInterface & addValidatedPendingInterface;
	
				ValidateBlockFragmentWorkPackageDispatcher(
					ValidateBlockFragmentPackageReturnInterface & rpackageReturnInterface,
					ValidateBlockFragmentAddPendingInterface    & raddValidatedPendingInterface
				) : packageReturnInterface(rpackageReturnInterface), addValidatedPendingInterface(raddValidatedPendingInterface)
				{
				}
			
				virtual void dispatch(
					libmaus::parallel::SimpleThreadWorkPackage * P, 
					libmaus::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & tpi
				)
				{
					ValidateBlockFragmentWorkPackage * BP = dynamic_cast<ValidateBlockFragmentWorkPackage *>(P);
					assert ( BP );
	
					bool const ok = BP->dispatch();
									
					addValidatedPendingInterface.validateBlockFragmentFinished(BP->fragment,ok);
									
					// return the work package				
					packageReturnInterface.putReturnValidateBlockFragmentPackage(BP);				
				}		
			};
		}
	}
}
#endif
