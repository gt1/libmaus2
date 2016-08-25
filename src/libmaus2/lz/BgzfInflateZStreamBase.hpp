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
#if ! defined(LIBMAUS2_LZ_BGZFINFLATEZSTREAMBASE_HPP)
#define LIBMAUS2_LZ_BGZFINFLATEZSTREAMBASE_HPP

#include <zlib.h>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/lz/BgzfConstants.hpp>
#include <libmaus2/lz/BgzfInflateHeaderBase.hpp>
#include <libmaus2/lz/ZlibInterface.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct BgzfInflateZStreamBase
		{
			typedef BgzfInflateZStreamBase this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			libmaus2::lz::ZlibInterface::unique_ptr_type zintf;

			public:
			void zinit()
			{
				zintf->eraseContext();
				zintf->setZAlloc(Z_NULL);
				zintf->setZFree(Z_NULL);
				zintf->setOpaque(Z_NULL);
				zintf->setAvailIn(0);
				zintf->setNextIn(Z_NULL);

				int const ret = zintf->z_inflateInit2(-15);

				if (ret != Z_OK)
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "BgzfInflate::init() failed in inflateInit2";
					se.finish();
					throw se;
				}
			}

			BgzfInflateZStreamBase()
			: zintf(libmaus2::lz::ZlibInterface::construct())
			{
				zinit();
			}

			~BgzfInflateZStreamBase()
			{
				zintf->z_inflateEnd();
			}

			void zreset()
			{
				if ( zintf->z_inflateReset() != Z_OK )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "BgzfInflateZStreamBase::zreset(): inflateReset failed";
					se.finish();
					throw se;
				}
			}

			uint32_t computeCrc(uint8_t const * data, uint64_t const l)
			{
				uint32_t crc = zintf->z_crc32(0L,Z_NULL,0);
				crc = zintf->z_crc32(crc,reinterpret_cast<Bytef const*>(data),l);
				return crc;
			}

			void zdecompress(
				uint8_t       * const in,
				unsigned int const inlen,
				char          * const out,
				unsigned int const outlen
			)
			{
				zreset();

				zintf->setAvailIn(inlen);
				zintf->setNextIn(in);
				zintf->setAvailOut(outlen);
				zintf->setNextOut(reinterpret_cast<Bytef *>(out));

				int const r = zintf->z_inflate(Z_FINISH);

				bool ok = (r == Z_STREAM_END)
				     && (zintf->getAvailOut() == 0)
				     && (zintf->getAvailIn() == 0);

				if ( !ok )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "BgzfInflateZStreamBase::zdecompress(): inflate failed";
					se.finish();
					throw se;
				}
			}
		};
	}
}
#endif
