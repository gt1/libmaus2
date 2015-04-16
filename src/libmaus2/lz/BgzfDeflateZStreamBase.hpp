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
#if ! defined(LIBMAUS_LZ_BGZFDEFLATEZSTREAMBASE_HPP)
#define LIBMAUS_LZ_BGZFDEFLATEZSTREAMBASE_HPP

#include <libmaus2/lz/BgzfDeflateHeaderFunctions.hpp>
#include <libmaus2/lz/BgzfDeflateInputBufferBase.hpp>
#include <libmaus2/lz/BgzfDeflateOutputBufferBase.hpp>
#include <libmaus2/lz/BgzfDeflateZStreamBaseFlushInfo.hpp>
#include <libmaus2/lz/IGzipDeflate.hpp>

#if defined(LIBMAUS_HAVE_IGZIP)
#include <libmaus2/util/I386CacheLineSize.hpp>
#endif

namespace libmaus2
{
	namespace lz
	{
		struct BgzfDeflateZStreamBase : public BgzfDeflateHeaderFunctions
		{
			typedef BgzfDeflateZStreamBase this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
		
			private:
			z_stream strm;
			unsigned int deflbound;
			int level;
		
			void deflatedestroy()
			{
				if ( level >= Z_DEFAULT_COMPRESSION && level <= Z_BEST_COMPRESSION )
					deflatedestroyz(&strm);
			}
			
			void deflateinit(int const rlevel)
			{
				level = rlevel;
				
				if ( level >= Z_DEFAULT_COMPRESSION && level <= Z_BEST_COMPRESSION )
				{
					deflateinitz(&strm,level);

					// search for number of bytes that will never produce more compressed space than we have
					unsigned int bound = getBgzfMaxBlockSize();
				
					while ( deflateBound(&strm,bound) > (getBgzfMaxBlockSize()-(getBgzfHeaderSize()+getBgzfFooterSize())) )
						--bound;

					deflbound = bound;
				}
				#if defined(LIBMAUS_HAVE_IGZIP)
				else if ( level == libmaus2::lz::IGzipDeflate::getCompressionLevel() )
				{
					// half a block should fit
					deflbound = (getBgzfMaxBlockSize()-(getBgzfHeaderSize()+getBgzfFooterSize()))/2;
				}
				#endif
				else
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "BgzfDeflateZStreamBase::deflateinit(): unknown/unsupported compression level " << level << std::endl;
					se.finish();
					throw se;							
				}
			}
			

			void resetz()
			{
				if ( deflateReset(&strm) != Z_OK )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "deflateReset failed." << std::endl;
					se.finish();
					throw se;		
				}			
			}

			// compress block of length len from input pa to output outbuf
			// returns the number of compressed bytes produced
			uint64_t compressBlock(uint8_t * pa, uint64_t const len, uint8_t * outbuf)
			{
				if ( level >= Z_DEFAULT_COMPRESSION && level <= Z_BEST_COMPRESSION )
				{				
					// reset zlib object
					resetz();
					
					// maximum number of output bytes
					strm.avail_out = getBgzfMaxPayLoad();
					// next compressed output byte
					strm.next_out  = reinterpret_cast<Bytef *>(outbuf) + getBgzfHeaderSize();
					// number of bytes to be compressed
					strm.avail_in  = len;
					// data to be compressed
					strm.next_in   = reinterpret_cast<Bytef *>(pa);
					
					// call deflate
					if ( deflate(&strm,Z_FINISH) != Z_STREAM_END )
					{
						libmaus2::exception::LibMausException se;
						se.getStream() << "deflate() failed." << std::endl;
						se.finish(false /* do not translate stack trace */);
						throw se;
					}
					
					return getBgzfMaxPayLoad() - strm.avail_out;
				}
				#if defined(LIBMAUS_HAVE_IGZIP)
				else if ( level == libmaus2::lz::IGzipDeflate::getCompressionLevel() )
				{
					int64_t const compsize = libmaus2::lz::IGzipDeflate::deflate(
						pa,len,outbuf+getBgzfHeaderSize(),getBgzfMaxPayLoad()
					);
					
					if ( compsize < 0 )
					{
						libmaus2::exception::LibMausException se;
						se.getStream() << "deflate() failed." << std::endl;
						se.finish(false /* do not translate stack trace */);
						throw se;					
					}
					
					return compsize;
				}
				#endif
				else
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "BgzfDeflateZStreamBase::compressBlock(): unknown/unsupported compression level " << level << std::endl;
					se.finish();
					throw se;
				}
			}

			BgzfDeflateZStreamBaseFlushInfo flushBound(
				BgzfDeflateInputBufferBase & in, 
				BgzfDeflateOutputBufferBase & out, 
				bool const fullflush 
			)
			{
				// full flush, compress block in two halves
				if ( fullflush )
				{
					uint64_t const toflush = in.pc-in.pa;
					uint64_t const flush0 = (toflush+1)/2;
					uint64_t const flush1 = toflush-flush0;

					/* compress first half of data */
					uint64_t const payload0 = compressBlock(in.pa,flush0,out.outbuf.begin());
					fillHeaderFooter(in.pa,out.outbuf.begin(),payload0,flush0);
					
					/* compress second half of data */
					setupHeader(out.outbuf.begin()+getBgzfHeaderSize()+payload0+getBgzfFooterSize());
					uint64_t const payload1 = compressBlock(in.pa+flush0,flush1,out.outbuf.begin()+getBgzfHeaderSize()+payload0+getBgzfFooterSize());
					fillHeaderFooter(in.pa+flush0,out.outbuf.begin()+getBgzfHeaderSize()+payload0+getBgzfFooterSize(),payload1,flush1);
					
					assert ( 2*getBgzfHeaderSize()+2*getBgzfFooterSize()+payload0+payload1 <= out.outbuf.size() );
										
					in.pc = in.pa;

					return
						BgzfDeflateZStreamBaseFlushInfo(
							flush0,
							getBgzfHeaderSize()+getBgzfFooterSize()+payload0,
							flush1,
							getBgzfHeaderSize()+getBgzfFooterSize()+payload1
						);
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
					
					#if 0
					/*
					 * copy rest of uncompressed data to front of buffer
					 */
					if ( unflushed )
						memmove(in.pa,in.pc-unflushed,unflushed);		

					// set new output pointer
					in.pc = in.pa + unflushed;
					#endif

					/* number number of bytes in output buffer */
					// return getBgzfHeaderSize()+getBgzfFooterSize()+payloadsize;
					return BgzfDeflateZStreamBaseFlushInfo(
						toflush,
						getBgzfHeaderSize()+getBgzfFooterSize()+payloadsize,
						in.pa, // moveto
						in.pc-unflushed, // movefrom
						unflushed // movesize
					);
				}
			}
			
			public:
			static uint64_t computeDeflateBound(int const rlevel)
			{
				z_stream strm;
				deflateinitz(&strm,rlevel);

				// search for number of bytes that will never produce more compressed space than we have
				unsigned int bound = getBgzfMaxBlockSize();
				
				while ( 
					deflateBound(&strm,bound) > 
					(getBgzfMaxBlockSize()-(getBgzfHeaderSize()+getBgzfFooterSize())) 
				)
					--bound;
					
				return bound;
			}

			BgzfDeflateZStreamBase(int const rlevel = Z_DEFAULT_COMPRESSION)
			{
				#if defined(LIBMAUS_HAVE_IGZIP)
				if ( rlevel == libmaus2::lz::IGzipDeflate::getCompressionLevel() )
				{
					if ( ! libmaus2::util::I386CacheLineSize::hasSSE42() )
					{
						::libmaus2::exception::LibMausException se;
						se.getStream() << "BgzfDeflateZStreamBase(): igzip requested, but machine does not support SSE4.2" << std::endl;
						se.finish();
						throw se;
					}
				}
				#endif

				deflateinit(rlevel);
			}
			
			~BgzfDeflateZStreamBase()
			{
				deflatedestroy();
			}

			// flush input buffer into output buffer
			BgzfDeflateZStreamBaseFlushInfo flush(
				BgzfDeflateInputBufferBase & in, 
				BgzfDeflateOutputBufferBase & out, 
				bool const fullflush
			)
			{
				try
				{
					uint64_t const uncompressedsize = in.pc-in.pa;
					uint64_t const payloadsize = compressBlock(in.pa,uncompressedsize,out.outbuf.begin());
					fillHeaderFooter(in.pa,out.outbuf.begin(),payloadsize,uncompressedsize);

					in.pc = in.pa;

					return BgzfDeflateZStreamBaseFlushInfo(uncompressedsize,getBgzfHeaderSize()+getBgzfFooterSize()+payloadsize);
					// return getBgzfHeaderSize()+getBgzfFooterSize()+payloadsize;
				}
				catch(...)
				{
					return flushBound(in,out,fullflush);
				}
			}
			
			BgzfDeflateZStreamBaseFlushInfo flush(uint8_t * const pa, uint8_t * const pe, BgzfDeflateOutputBufferBase & out)
			{
				if ( pe-pa > getBgzfMaxBlockSize() )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "BgzfDeflateZStreamBase()::flush: block is too big for BGZF" << std::endl;
					se.finish();
					throw se;				
				}
			
				try
				{
					uint64_t const uncompressedsize = pe-pa;
					uint64_t const payloadsize = compressBlock(pa,uncompressedsize,out.outbuf.begin());
					fillHeaderFooter(pa,out.outbuf.begin(),payloadsize,uncompressedsize);
					return BgzfDeflateZStreamBaseFlushInfo(uncompressedsize,getBgzfHeaderSize()+getBgzfFooterSize()+payloadsize);
				}
				catch(...)
				{
					uint64_t const toflush = pe-pa;
					uint64_t const flush0 = (toflush+1)/2;
					uint64_t const flush1 = toflush-flush0;

					/* compress first half of data */
					uint64_t const payload0 = compressBlock(pa,flush0,out.outbuf.begin());
					fillHeaderFooter(pa,out.outbuf.begin(),payload0,flush0);
					
					/* compress second half of data */
					setupHeader(out.outbuf.begin()+getBgzfHeaderSize()+payload0+getBgzfFooterSize());
					uint64_t const payload1 = compressBlock(pa+flush0,flush1,out.outbuf.begin()+getBgzfHeaderSize()+payload0+getBgzfFooterSize());
					fillHeaderFooter(pa+flush0,out.outbuf.begin()+getBgzfHeaderSize()+payload0+getBgzfFooterSize(),payload1,flush1);
					
					assert ( 2*getBgzfHeaderSize()+2*getBgzfFooterSize()+payload0+payload1 <= out.outbuf.size() );
										
					return
						BgzfDeflateZStreamBaseFlushInfo(
							flush0,
							getBgzfHeaderSize()+getBgzfFooterSize()+payload0,
							flush1,
							getBgzfHeaderSize()+getBgzfFooterSize()+payload1
						);
				}				
			}

			void deflatereinit(int const rlevel = Z_DEFAULT_COMPRESSION)
			{
				deflatedestroy();
				deflateinit(rlevel);
			}
		};
	}
}
#endif
