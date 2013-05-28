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
#if ! defined(LIBMAUS_LZ_BGZFDEFLATEZSTREAMBASE_HPP)
#define LIBMAUS_LZ_BGZFDEFLATEZSTREAMBASE_HPP

#include <libmaus/lz/BgzfDeflateHeaderFunctions.hpp>
#include <libmaus/lz/BgzfDeflateInputBufferBase.hpp>
#include <libmaus/lz/BgzfDeflateOutputBufferBase.hpp>

namespace libmaus
{
	namespace lz
	{
		struct BgzfDeflateZStreamBase : public BgzfDeflateHeaderFunctions
		{
			z_stream strm;
			unsigned int deflbound;
		
			void deflatedestroy()
			{
				deflatedestroyz(&strm);		
			}
			
			void deflateinit(int const level = Z_DEFAULT_COMPRESSION)
			{
				deflateinitz(&strm,level);

				// search for number of bytes that will never produce more compressed space than we have
				unsigned int bound = maxblocksize;
				
				while ( deflateBound(&strm,bound) > (maxblocksize-(headersize+footersize)) )
					--bound;

				deflbound = bound;
			}
			
			void deflatereinit(int const level = Z_DEFAULT_COMPRESSION)
			{
				deflatedestroy();
				deflateinit(level);
			}

			void resetz()
			{
				if ( deflateReset(&strm) != Z_OK )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "deflateReset failed." << std::endl;
					se.finish();
					throw se;		
				}			
			}
			
			BgzfDeflateZStreamBase(int const level = Z_DEFAULT_COMPRESSION)
			{
				deflateinit(level);
			}
			
			~BgzfDeflateZStreamBase()
			{
				deflatedestroy();
			}

			uint64_t compressBlock(uint8_t * pa, uint64_t const len, uint8_t * outbuf)
			{
				resetz();
				
				strm.avail_out = maxpayload;
				strm.next_out  = reinterpret_cast<Bytef *>(outbuf) + headersize;
				strm.avail_in  = len;
				strm.next_in   = reinterpret_cast<Bytef *>(pa);
				
				if ( deflate(&strm,Z_FINISH) != Z_STREAM_END )
				{
					libmaus::exception::LibMausException se;
					se.getStream() << "deflate() failed." << std::endl;
					se.finish(false /* do not translate stack trace */);
					throw se;
				}
				
				return maxpayload - strm.avail_out;
			}

			uint64_t flushBound(
				BgzfDeflateInputBufferBase & in, 
				BgzfDeflateOutputBufferBase & out, 
				bool const fullflush
			)
			{
				if ( fullflush )
				{
					uint64_t const toflush = in.pc-in.pa;
					uint64_t const flush0 = (toflush+1)/2;
					uint64_t const flush1 = toflush-flush0;

					/* compress first half of data */
					uint64_t const payload0 = compressBlock(in.pa,flush0,out.outbuf.begin());
					fillHeaderFooter(in.pa,out.outbuf.begin(),payload0,flush0);
					
					/* compress second half of data */
					setupHeader(out.outbuf.begin()+headersize+payload0+footersize);
					uint64_t const payload1 = compressBlock(in.pa+flush0,flush1,out.outbuf.begin()+headersize+payload0+footersize);
					fillHeaderFooter(in.pa+flush0,out.outbuf.begin()+headersize+payload0+footersize,payload1,flush1);
					
					assert ( 2*headersize+2*footersize+payload0+payload1 <= out.outbuf.size() );
										
					in.pc = in.pa;

					/* return number of bytes in output buffer */
					return 2*headersize+2*footersize+payload0+payload1;
				}				
				else
				{
					unsigned int const toflush = std::min(static_cast<unsigned int>(in.pc-in.pa),deflbound);
					unsigned int const unflushed = (in.pc-in.pa)-toflush;
					
					/*
					 * write out compressed data
					 */
					uint64_t const payloadsize = compressBlock(in.pa,toflush,out.outbuf.begin());
					fillHeaderFooter(in.pa,out.outbuf.begin(),payloadsize,toflush);
					
					/*
					 * copy rest of uncompressed data to front of buffer
					 */
					if ( unflushed )
						memmove(in.pa,in.pc-unflushed,unflushed);		

					// set new output pointer
					in.pc = in.pa + unflushed;

					/* number number of bytes in output buffer */
					return headersize+footersize+payloadsize;
				}
			}

			uint64_t flush(
				BgzfDeflateInputBufferBase & in, 
				BgzfDeflateOutputBufferBase & out, 
				bool const fullflush
			)
			{
				try
				{
					uint64_t const payloadsize = compressBlock(in.pa,in.pc-in.pa,out.outbuf.begin());
					fillHeaderFooter(in.pa,out.outbuf.begin(),payloadsize,in.pc-in.pa);

					in.pc = in.pa;

					return headersize+footersize+payloadsize;
				}
				catch(...)
				{
					return flushBound(in,out,fullflush);
				}
			}
		};
	}
}
#endif
