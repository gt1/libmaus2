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
#if ! defined(LIBMAUS_LZ_BGZFDEFLATEBASE_HPP)
#define LIBMAUS_LZ_BGZFDEFLATEBASE_HPP

#include <libmaus/lz/BgzfDeflateZStreamBase.hpp>

namespace libmaus
{
	namespace lz
	{
		struct BgzfDeflateBase : 
			public BgzfDeflateZStreamBase,
			public BgzfDeflateOutputBufferBase, 
			public BgzfDeflateInputBufferBase
		{
			typedef BgzfDeflateBase this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			/* flush mode: 
			   - true: completely empty buffer when it runs full, write more than
			            one block per flush if needed
			   - false: empty as much as possible from the buffer when it runs full
			            but never write more than one bgzf block at once
			 */
			bool flushmode;
			
			uint64_t objectid;
			uint64_t blockid;
			uint64_t compsize;
			
			BgzfDeflateBase(int const level = Z_DEFAULT_COMPRESSION, bool const rflushmode = false)
			:
			  BgzfDeflateZStreamBase(level),
			  BgzfDeflateOutputBufferBase(level),
			  BgzfDeflateInputBufferBase(),
			  flushmode(rflushmode),
			  objectid(0),
			  blockid(0),
			  compsize(0)
			{
			}

			uint64_t flush(bool const fullflush)
			{
				return BgzfDeflateZStreamBase::flush(*this,*this,fullflush);
			}
		};
	}
}
#endif
