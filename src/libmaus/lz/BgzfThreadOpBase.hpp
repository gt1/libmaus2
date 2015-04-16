/*
    libmaus
    Copyright (C) 2009-2013 German Tischler
    Copyright (C) 2011-2013 Genome Research Limited

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
#if ! defined(LIBMAUS_LZ_BGZFTHREADOPBASE_HPP)
#define LIBMAUS_LZ_BGZFTHREADOPBASE_HPP
#include <map>
#include <ostream>

namespace libmaus
{
	namespace lz
	{
		struct BgzfThreadOpBase
		{
			enum libmaus_lz_bgzf_op_type { 
				libmaus_lz_bgzf_op_write_block = 0,
				libmaus_lz_bgzf_op_compress_block = 1, 
				libmaus_lz_bgzf_op_decompress_block = 2, 
				libmaus_lz_bgzf_op_read_block = 3, 
				libmaus_lz_bgzf_op_none = 4
			};
		};
		
		inline std::ostream & operator<<(std::ostream & out, BgzfThreadOpBase::libmaus_lz_bgzf_op_type const op)
		{
			switch ( op )
			{
				case BgzfThreadOpBase::libmaus_lz_bgzf_op_read_block:
					out << "libmaus_lz_bgzf_op_read_block"; break;
				case BgzfThreadOpBase::libmaus_lz_bgzf_op_decompress_block:
					out << "libmaus_lz_bgzf_op_decompress_block"; break;
				case BgzfThreadOpBase::libmaus_lz_bgzf_op_compress_block:
					out << "libmaus_lz_bgzf_op_compress_block"; break;
				case BgzfThreadOpBase::libmaus_lz_bgzf_op_write_block:
					out << "libmaus_lz_bgzf_op_write_block"; break;
				case BgzfThreadOpBase::libmaus_lz_bgzf_op_none:
					out << "libmaus_lz_bgzf_op_none"; break;
			}
			
			return out;
		}
				
		struct BgzfThreadQueueElement
		{
			BgzfThreadOpBase::libmaus_lz_bgzf_op_type op;
			uint64_t objectid;
			uint64_t blockid;
				
			BgzfThreadQueueElement(
				BgzfThreadOpBase::libmaus_lz_bgzf_op_type const rop = BgzfThreadOpBase::libmaus_lz_bgzf_op_none, 
				uint64_t const robjectid = 0,
				uint64_t const rblockid = 0
			)
			: op(rop), objectid(robjectid), blockid(rblockid)
			{
			
			}
			
			BgzfThreadQueueElement & operator++()
			{
				blockid++;
				return *this;
			}
			
			BgzfThreadQueueElement operator++(int)
			{
				BgzfThreadQueueElement copy = *this;
				blockid++;
				return copy;
			}
			
			bool operator<=(BgzfThreadQueueElement const & o) const
			{
				return blockid <= o.blockid;
			}

			bool operator<(BgzfThreadQueueElement const & o) const
			{
				return blockid < o.blockid;
			}
			
			bool operator==(BgzfThreadQueueElement const & o) const
			{
				return blockid == o.blockid;
			}
		};

		struct BgzfThreadQueueElementHeapComparator
		{
			bool operator()(BgzfThreadQueueElement const & a, BgzfThreadQueueElement const & b) const
			{
				if ( a.op != b.op )
					return static_cast<int>(a.op) > static_cast<int>(b.op);
				else if ( a.blockid != b.blockid )
					return a.blockid > b.blockid;
				else
					return a.objectid > b.objectid;
			}
		};
	}
}
#endif
