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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_READENDSMERGEREQUEST_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_READENDSMERGEREQUEST_HPP

#include <vector>
#include <utility>
#include <libmaus/bitio/BitVector.hpp>
#include <libmaus/bambam/ReadEndsBlockDecoderBaseCollectionInfoBase.hpp>

namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{
			struct ReadEndsMergeRequest
			{
				libmaus::bitio::BitVector * dupbitvec;
				libmaus::util::shared_ptr< std::vector< ::libmaus::bambam::ReadEndsBlockDecoderBaseCollectionInfoBase > >::type MI;
				std::vector< std::pair<uint64_t,uint64_t> > SMI;
				
				ReadEndsMergeRequest()
				: dupbitvec(0), MI(0), SMI(0)
				{
				}
				
				ReadEndsMergeRequest(
					libmaus::bitio::BitVector * rdupbitvec,
					libmaus::util::shared_ptr< std::vector< ::libmaus::bambam::ReadEndsBlockDecoderBaseCollectionInfoBase > >::type rMI,
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
