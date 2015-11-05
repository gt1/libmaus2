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
#if ! defined(LIBMAUS2_BAMBAM_PARALLEL_READENDSMERGEREQUEST_HPP)
#define LIBMAUS2_BAMBAM_PARALLEL_READENDSMERGEREQUEST_HPP

#include <vector>
#include <utility>
#include <libmaus2/bitio/BitVector.hpp>
#include <libmaus2/bambam/ReadEndsBlockDecoderBaseCollectionInfoBase.hpp>

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{
			struct ReadEndsMergeRequest
			{
				libmaus2::bitio::BitVector * dupbitvec;
				libmaus2::util::shared_ptr< std::vector< ::libmaus2::bambam::ReadEndsBlockDecoderBaseCollectionInfoBase > >::type MI;
				std::vector< std::pair<uint64_t,uint64_t> > SMI;

				ReadEndsMergeRequest()
				: dupbitvec(0), MI(), SMI()
				{
				}

				ReadEndsMergeRequest(
					libmaus2::bitio::BitVector * rdupbitvec,
					libmaus2::util::shared_ptr< std::vector< ::libmaus2::bambam::ReadEndsBlockDecoderBaseCollectionInfoBase > >::type rMI,
					std::vector< std::pair<uint64_t,uint64_t> > const & rSMI
				)
				: dupbitvec(rdupbitvec), MI(rMI), SMI(rSMI)
				{
				}
			};
		}
	}
}
#endif
