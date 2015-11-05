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
#if ! defined(LIBMAUS2_BAMBAM_PARALLEL_CRAMOUTPUTBLOCKCHECKSUMPACKAGE_HPP)
#define LIBMAUS2_BAMBAM_PARALLEL_CRAMOUTPUTBLOCKCHECKSUMPACKAGE_HPP

#include <libmaus2/bambam/parallel/CramOutputBlock.hpp>
#include <libmaus2/parallel/SimpleThreadWorkPackage.hpp>
#include <libmaus2/digest/DigestInterface.hpp>

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{
			struct CramOutputBlockChecksumPackage : public libmaus2::parallel::SimpleThreadWorkPackage
			{
				typedef CramOutputBlockChecksumPackage this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				CramOutputBlock::shared_ptr_type block;
				libmaus2::digest::DigestInterface * filechecksum;

				CramOutputBlockChecksumPackage() : libmaus2::parallel::SimpleThreadWorkPackage(), block(), filechecksum(0) {}
				CramOutputBlockChecksumPackage(
					uint64_t const rpriority,
					uint64_t const rdispatcherid,
					CramOutputBlock::shared_ptr_type rblock,
					libmaus2::digest::DigestInterface * rfilechecksum
				)
				: libmaus2::parallel::SimpleThreadWorkPackage(rpriority,rdispatcherid), block(rblock), filechecksum(rfilechecksum)
				{

				}
				virtual ~CramOutputBlockChecksumPackage() {}

				virtual char const * getPackageName() const
				{
					return "CramOutputBlockChecksumPackage";
				}
			};
		}
	}
}
#endif
