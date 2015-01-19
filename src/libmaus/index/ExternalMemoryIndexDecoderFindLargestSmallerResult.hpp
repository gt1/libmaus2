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
#if ! defined(LIBMAUS_INDEX_EXTERNALMEMORYINDEXDECODERFINDLARGESTSMALLERRESULT_HPP)
#define LIBMAUS_INDEX_EXTERNALMEMORYINDEXDECODERFINDLARGESTSMALLERRESULT_HPP

#include <utility>
#include <libmaus/types/types.hpp>

namespace libmaus
{
	namespace index
	{
		struct ExternalMemoryIndexDecoderFindLargestSmallerResult
		{
			std::pair<uint64_t,uint64_t> P;
			uint64_t blockid;
			
			ExternalMemoryIndexDecoderFindLargestSmallerResult() : P(), blockid(0) {}
			ExternalMemoryIndexDecoderFindLargestSmallerResult(std::pair<uint64_t,uint64_t> const rP, uint64_t const rblockid)
			: P(rP), blockid(rblockid) {}
			
			bool operator==(ExternalMemoryIndexDecoderFindLargestSmallerResult const & O) const
			{
				return P == O.P && blockid == O.blockid;
			}
		};
	}
}
#endif
