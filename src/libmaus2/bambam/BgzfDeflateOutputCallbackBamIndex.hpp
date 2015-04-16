/*
    libmaus2
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
#if ! defined(LIBMAUS_LZ_BGZFDEFLATEOUTPUTCALLBACKBAMINDEX_HPP)
#define LIBMAUS_LZ_BGZFDEFLATEOUTPUTCALLBACKBAMINDEX_HPP

#include <libmaus2/digest/md5.h>
#include <libmaus2/lz/BgzfDeflateOutputCallback.hpp>
#include <libmaus2/bambam/BamIndexGenerator.hpp>
#include <sstream>
#include <iomanip>

namespace libmaus2
{
	namespace bambam
	{
		struct BgzfDeflateOutputCallbackBamIndex : public ::libmaus2::lz::BgzfDeflateOutputCallback
		{
			typedef BgzfDeflateOutputCallbackBamIndex this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
			
			::libmaus2::bambam::BamIndexGenerator generator;
		
			BgzfDeflateOutputCallbackBamIndex(
				std::string const & tmpfileprefix,
				unsigned int const rverbose = 0,
				bool const rvalidate = true, bool const rdebug = false
			) : generator(tmpfileprefix,rverbose,rvalidate,rdebug)
			{

			}
			
			void operator()(
				uint8_t const * in, 
				uint64_t const incnt,
				uint8_t const * /* out */,
				uint64_t const outcnt
			)
			{
				generator.addBlock(in /* uncomp data */,outcnt /* comp size */,incnt /* uncomp size */);
			}
			
			template<typename stream_type>
			void flush(stream_type & stream)
			{
				generator.flush(stream);
			}
			
			void flush(std::string const & filename)
			{
				libmaus2::aio::CheckedOutputStream COS(filename);
				flush(COS);
			}
		};
	}
}
#endif
