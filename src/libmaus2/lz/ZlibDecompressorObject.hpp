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
#if ! defined(LIBMAUS_LZ_ZLIBDECOMPRESSOROBJECT_HPP)
#define LIBMAUS_LZ_ZLIBDECOMPRESSOROBJECT_HPP

#include <libmaus2/lz/DecompressorObject.hpp>
#include <libmaus2/lz/BgzfInflateZStreamBase.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct ZlibDecompressorObject : public DecompressorObject
		{
			typedef ZlibDecompressorObject this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			
			::libmaus2::lz::BgzfInflateZStreamBase inflatebase;
			
			ZlibDecompressorObject()
			: inflatebase()
			{
				
			}
			virtual ~ZlibDecompressorObject() {}
			virtual bool rawuncompress(char const * compressed, size_t compressed_length, char * uncompressed, size_t uncompressed_length)
			{
				try
				{
					inflatebase.zdecompress(
						const_cast<uint8_t *>(reinterpret_cast<uint8_t const *>(compressed)),
						compressed_length,
						uncompressed,
						uncompressed_length);
					return true;
				}
				catch(...)
				{
					return false;
				}
			}
			virtual std::string getDescription() const
			{
				return "ZlibDecompressorObject";
			}
		};
	}
}
#endif
