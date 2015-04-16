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
#if ! defined(LIBMAUS_LZ_BGZFDEFLATEOUTPUTCALLBACKMD5_HPP)
#define LIBMAUS_LZ_BGZFDEFLATEOUTPUTCALLBACKMD5_HPP

#include <libmaus/util/md5.h>
#include <libmaus/lz/BgzfDeflateOutputCallback.hpp>
#include <libmaus/aio/CheckedOutputStream.hpp>
#include <sstream>
#include <iomanip>

namespace libmaus
{
	namespace lz
	{
		struct BgzfDeflateOutputCallbackMD5 : public ::libmaus::lz::BgzfDeflateOutputCallback
		{
			typedef BgzfDeflateOutputCallbackMD5 this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

			md5_state_t pms;
			md5_byte_t digest[16];
			std::string sdigest;
			
			BgzfDeflateOutputCallbackMD5()
			{
				md5_init(&pms);	
			}
			
			void operator()(
				uint8_t const * /* in */, 
				uint64_t const /* incnt */,
				uint8_t const * out,
				uint64_t const outcnt
			)
			{
				md5_append(&pms,reinterpret_cast<md5_byte_t const *>(out),outcnt);
			}

			std::string getDigest()
			{
				if ( ! sdigest.size() )
				{
					md5_finish(&pms,digest);

					std::ostringstream ostr;
					for ( uint64_t i = 0; i < sizeof(digest)/sizeof(digest[0]); ++i )
						ostr << std::hex << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(digest[i]);
				
					sdigest = ostr.str();
				}
				
				return sdigest;
			}
			
			template<typename stream_type>
			void saveDigest(stream_type & stream)
			{
				stream << getDigest();
			}
			
			void saveDigestAsFile(std::string const & filename)
			{
				libmaus::aio::CheckedOutputStream COS(filename);
				saveDigest(COS);
				COS.flush();
				COS.close();
			}
		};
	}
}
#endif
