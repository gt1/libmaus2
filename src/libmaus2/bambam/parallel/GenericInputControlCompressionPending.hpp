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
#if ! defined(LIBMAUS2_BAMBAM_PARALLEL_GENERICINPUTCONTROLCOMPRESSIONPENDING_HPP)
#define LIBMAUS2_BAMBAM_PARALLEL_GENERICINPUTCONTROLCOMPRESSIONPENDING_HPP

#include <libmaus2/lz/BgzfDeflateOutputBufferBase.hpp>
#include <libmaus2/lz/BgzfDeflateZStreamBaseFlushInfo.hpp>

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{
			struct GenericInputControlCompressionPending
			{
				typedef GenericInputControlCompressionPending this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
				
				// block number (-1 for header)
				 int64_t blockid;
				// sub id inside block
				uint64_t subid;
				// sequence number in set of all bgzf blocks
				uint64_t absid;
				// true if this is the last (EOF) block
				bool final;
				// uncompressed data range
				std::pair<uint8_t *,uint8_t *> P;
				// compressed data block
				libmaus2::lz::BgzfDeflateOutputBufferBase::shared_ptr_type outblock;
				// flush info from BGZF compressor
				libmaus2::lz::BgzfDeflateZStreamBaseFlushInfo flushinfo;
			
				GenericInputControlCompressionPending() {}	
				GenericInputControlCompressionPending(
					 int64_t const rblockid,
					uint64_t const rsubid,
					uint64_t const rabsid,
					bool const rfinal,
					std::pair<uint8_t *,uint8_t *> const & rP
				) : blockid(rblockid), subid(rsubid), absid(rabsid), final(rfinal), P(rP) {}
			};
		}
	}
}
#endif
