/*
    libmaus2
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#if ! defined(LIBMAUS2_LZ_ZLIBCOMPRESSOROBJECT_HPP)
#define LIBMAUS2_LZ_ZLIBCOMPRESSOROBJECT_HPP

#include <libmaus2/lz/CompressorObject.hpp>
#include <libmaus2/lz/BgzfDeflateBase.hpp>
#include <sstream>

namespace libmaus2
{
	namespace lz
	{
		struct ZlibCompressorObject : public CompressorObject
		{
			typedef ZlibCompressorObject this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			libmaus2::lz::ZlibInterface::unique_ptr_type zintf;
			int const level;

			public:
			uint64_t inputBound;
			uint64_t outputBound;

			ZlibCompressorObject(int const rlevel = Z_DEFAULT_COMPRESSION) : zintf(libmaus2::lz::ZlibInterface::construct()), level(rlevel), inputBound(0), outputBound(0)
			{
				BgzfDeflateHeaderFunctions::deflateinitz(zintf.get(),level);
				outputBound = zintf->z_deflateBound(inputBound);
			}
			~ZlibCompressorObject()
			{
				BgzfDeflateHeaderFunctions::deflatedestroyz(zintf.get());
			}

			virtual size_t compress(char const * input, size_t inputLength, libmaus2::autoarray::AutoArray<char> & output)
			{
				zintf->z_deflateReset();

				if ( inputLength > inputBound )
				{
					inputBound = inputLength;
					outputBound = zintf->z_deflateBound(inputBound);
				}

				if ( outputBound > output.size() )
					output = libmaus2::autoarray::AutoArray<char>(outputBound,false);

				// maximum number of output bytes
				zintf->setAvailOut(output.size());
				// next compressed output byte
				zintf->setNextOut(reinterpret_cast<Bytef *>(output.begin()));
				// number of bytes to be compressed
				zintf->setAvailIn(inputLength);
				// data to be compressed
				zintf->setNextIn(const_cast<Bytef *>(reinterpret_cast<Bytef const *>(input)));

				int const retcode = zintf->z_deflate(Z_FINISH);

				// std::cerr << "avail_out=" << strm.avail_out << std::endl;
				// std::cerr << "avail_in=" << strm.avail_in << std::endl;

				// call deflate
				if ( retcode != Z_STREAM_END )
				{
					libmaus2::exception::LibMausException se;
					se.getStream() << "deflate() failed: " << retcode << ", " << zError(retcode) << std::endl;
					se.finish(false /* do not translate stack trace */);
					throw se;
				}

				uint64_t const compsize = output.size() - zintf->getAvailOut();

				return compsize;
			}

			virtual std::string getDescription() const
			{
				std::ostringstream ostr;
				ostr << "ZlibCompressorObject(" << level << ")";
				return ostr.str();
			}
		};
	}
}
#endif
