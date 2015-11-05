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
#if ! defined(LIBMAUS2_BAMBAM_PARALLEL_GENERICINPUTBAMPARSEWORKPACKAGEDISPATCHER_HPP)
#define LIBMAUS2_BAMBAM_PARALLEL_GENERICINPUTBAMPARSEWORKPACKAGEDISPATCHER_HPP

#include <libmaus2/parallel/SimpleThreadWorkPackageDispatcher.hpp>
#include <libmaus2/bambam/parallel/GenericInputBamParseWorkPackageReturnInterface.hpp>
#include <libmaus2/bambam/parallel/GenericInputBamParseWorkPackageBlockParsedInterface.hpp>

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{
			struct GenericInputBamParseWorkPackageDispatcher : public libmaus2::parallel::SimpleThreadWorkPackageDispatcher
			{
				typedef GenericInputBamParseWorkPackageDispatcher this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				GenericInputBamParseWorkPackageReturnInterface & packageReturnInterface;
				GenericInputBamParseWorkPackageBlockParsedInterface & blockParsedInterface;

				GenericInputBamParseWorkPackageDispatcher(
					GenericInputBamParseWorkPackageReturnInterface & rpackageReturnInterface,
					GenericInputBamParseWorkPackageBlockParsedInterface & rblockParsedInterface
				)
				: packageReturnInterface(rpackageReturnInterface),
				  blockParsedInterface(rblockParsedInterface)
				{

				}

				void dispatch(libmaus2::parallel::SimpleThreadWorkPackage * P, libmaus2::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & /* tpi */)
				{
					assert ( dynamic_cast<GenericInputBamParseWorkPackage *>(P) != 0 );
					GenericInputBamParseWorkPackage * BP = dynamic_cast<GenericInputBamParseWorkPackage *>(P);

					BP->data.dispatch();

					blockParsedInterface.genericInputBamParseWorkPackageBlockParsed(BP->data.block);
					packageReturnInterface.genericInputBamParseWorkPackageReturn(BP);
				}
			};
		}
	}
}
#endif
