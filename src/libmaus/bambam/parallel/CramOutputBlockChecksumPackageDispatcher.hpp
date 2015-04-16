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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_CRAMOUTPUTBLOCKCHECKSUMPACKAGEDISPATCHER_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_CRAMOUTPUTBLOCKCHECKSUMPACKAGEDISPATCHER_HPP

#include <libmaus/bambam/parallel/CramOutputBlockChecksumComputedInterface.hpp>
#include <libmaus/bambam/parallel/CramOutputBlockChecksumPackageReturnInterface.hpp>
#include <libmaus/parallel/SimpleThreadWorkPackageDispatcher.hpp>

namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{
			struct CramOutputBlockChecksumPackageDispatcher : public libmaus::parallel::SimpleThreadWorkPackageDispatcher
			{
				typedef CramOutputBlockChecksumPackage this_type;
				typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
				
				CramOutputBlockChecksumPackageReturnInterface & packageReturnInterface;
				CramOutputBlockChecksumComputedInterface & checksumComputedInterface;
						
				CramOutputBlockChecksumPackageDispatcher(
					CramOutputBlockChecksumPackageReturnInterface & rpackageReturnInterface,
					CramOutputBlockChecksumComputedInterface & rchecksumComputedInterface
				) : packageReturnInterface(rpackageReturnInterface), checksumComputedInterface(rchecksumComputedInterface) {}
				~CramOutputBlockChecksumPackageDispatcher() {}
				void dispatch(libmaus::parallel::SimpleThreadWorkPackage * P, libmaus::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & /* tpi */)
				{
					CramOutputBlockChecksumPackage * BP = dynamic_cast<CramOutputBlockChecksumPackage *>(P);

					CramOutputBlock::shared_ptr_type block = BP->block;
					libmaus::digest::DigestInterface * filechecksum = BP->filechecksum;

					if ( block->fill )
						filechecksum->vupdate(reinterpret_cast<uint8_t const *>(block->A->begin()),block->fill);

					checksumComputedInterface.cramOutputBlockChecksumComputed(BP->block);
					packageReturnInterface.cramOutputBlockChecksumPackageReturn(BP);
				}
			};
		}
	}
}
#endif
