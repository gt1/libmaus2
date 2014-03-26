/*
    libmaus
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
#if ! defined(LIBMAUS_LZ_ZLIBCOMPRESSOROBJECT_HPP)
#define LIBMAUS_LZ_ZLIBCOMPRESSOROBJECT_HPP

#include <libmaus/lz/CompressorObject.hpp>
#include <libmaus/lz/BgzfDeflateBase.hpp>

namespace libmaus
{
	namespace lz
	{
		struct ZlibCompressorObject : public CompressorObject
		{
			typedef ZlibCompressorObject this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			
			int const level;
			z_stream strm;
			
			libmaus::autoarray::AutoArray<uint8_t> outbuf;
			uint64_t inBound;
			
			ZlibCompressorObject(int const rlevel = Z_DEFAULT_COMPRESSION) 
			: level(rlevel), outbuf(), inBound(0)
			{
				BgzfDeflateHeaderFunctions::deflateinitz(&strm,level);		
			}
			~ZlibCompressorObject() 
			{
				BgzfDeflateHeaderFunctions::deflatedestroyz(&strm);
			}
			
			virtual std::string compress(std::string const & s)
			{
				deflateReset(&strm);

				if ( (!inBound) || (s.size() > inBound) )
				{
					uint64_t const outbound = deflateBound(&strm,s.size());
					
					if ( outbound > outbuf.size() )
						outbuf = libmaus::autoarray::AutoArray<uint8_t>(outbound,false);
										
					inBound = s.size();
				}

				// maximum number of output bytes
				strm.avail_out = outbuf.size();
				// next compressed output byte
				strm.next_out  = reinterpret_cast<Bytef *>(outbuf.begin());
				// number of bytes to be compressed
				strm.avail_in  = s.size();
				// data to be compressed
				strm.next_in   = const_cast<Bytef *>(reinterpret_cast<Bytef const *>(s.c_str()));
				
				int const retcode = deflate(&strm,Z_FINISH);
				
				// std::cerr << "avail_out=" << strm.avail_out << std::endl;
				// std::cerr << "avail_in=" << strm.avail_in << std::endl;

				// call deflate
				if ( retcode != Z_STREAM_END )
				{
					libmaus::exception::LibMausException se;
					se.getStream() << "deflate() failed: " << retcode << ", " << zError(retcode) << std::endl;
					se.finish(false /* do not translate stack trace */);
					throw se;
				}
				
				uint64_t const compsize = outbuf.size() - strm.avail_out;
				char const * c = reinterpret_cast<char const *>(outbuf.begin());
				
				return std::string(c,c+compsize);
			}
		};
	}
}
#endif
