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
#if ! defined(LIBMAUS2_INDEX_EXTERNALMEMORYINDEXDECODERFINDLARGESTSMALLERRESULT_HPP)
#define LIBMAUS2_INDEX_EXTERNALMEMORYINDEXDECODERFINDLARGESTSMALLERRESULT_HPP

#include <utility>
#include <libmaus2/types/types.hpp>
#include <ostream>

namespace libmaus2
{
	namespace index
	{
		template<typename _data_type>
		struct ExternalMemoryIndexDecoderFindLargestSmallerResult
		{
			typedef _data_type data_type;
		
			std::pair<uint64_t,uint64_t> P;
			uint64_t blockid;
			data_type D;
			
			ExternalMemoryIndexDecoderFindLargestSmallerResult() : P(), blockid(0), D() {}
			ExternalMemoryIndexDecoderFindLargestSmallerResult(
				std::pair<uint64_t,uint64_t> const rP, uint64_t const rblockid,
				data_type const & rD
			)
			: P(rP), blockid(rblockid), D(rD) {}
			
			bool operator==(ExternalMemoryIndexDecoderFindLargestSmallerResult const & O) const
			{
				return P == O.P && blockid == O.blockid;
			}
		};
		
		template<typename data_type>
		std::ostream & operator<<(std::ostream & out, ExternalMemoryIndexDecoderFindLargestSmallerResult<data_type> const & O)
		{
			out << "ExternalMemoryIndexDecoderFindLargestSmallerResult("
				<< "[" << O.P.first << "," << O.P.second << "]"
				<< ","
				<< O.blockid
				<< ","
				<< O.D
				<< ")";
			return out;
		}
	}
}
#endif
