/**
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
**/
#if ! defined(LIBMAUS_LZ_BGZFDEFLATEBASE_HPP)
#define LIBMAUS_LZ_BGZFDEFLATEBASE_HPP

#include <libmaus/lz/GzipHeader.hpp>
#include <zlib.h>

namespace libmaus
{
	namespace lz
	{
		struct BgzfDeflateHeaderFunctions
		{
			static unsigned int const maxblocksize = 64*1024;
			static unsigned int const headersize = 18;
			static unsigned int const footersize = 8;
			static unsigned int const maxpayload = maxblocksize - (headersize+footersize);
			
			static void deflateinitz(z_stream * strm, int const level)
			{
				memset ( strm , 0, sizeof(z_stream) );
				strm->zalloc = Z_NULL;
				strm->zfree = Z_NULL; 
				strm->opaque = Z_NULL;
				int ret = deflateInit2(strm, level, Z_DEFLATED, -15 /* window size */,
					8 /* mem level, gzip default */, Z_DEFAULT_STRATEGY);
				if ( ret != Z_OK )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "deflateInit2 failed." << std::endl;
					se.finish();
					throw se;
				}
			}

			static void deflatedestroyz(z_stream * strm)
			{
				deflateEnd(strm);		
			}
			
			struct LocalDeflateInfo
			{
				z_stream strm;
				
				LocalDeflateInfo(int const level)
				{
					deflateinitz(&strm,level);
				}
				~LocalDeflateInfo()
				{
					deflatedestroyz(&strm);
				}
			};
			
			static uint64_t getFlushBound(unsigned int const blocksize, int const level)
			{
				LocalDeflateInfo LDI(level);
				return deflateBound(&(LDI.strm),blocksize);
			}
			
			static uint64_t getReqBufSpace(int const level)
			{
				return headersize + footersize + getFlushBound(maxblocksize,level);
			}

			static uint64_t getReqBufSpaceTwo(int const level)
			{
				uint64_t const halfblocksize = (maxblocksize+1)/2;
				uint64_t const singleblocksize = headersize + footersize + getFlushBound(halfblocksize,level);
				uint64_t const doubleblocksize = 2 * singleblocksize;
				return doubleblocksize;
			}
			
			static void setupHeader(uint8_t * const outbuf)
			{
				outbuf[0] = ::libmaus::lz::GzipHeader::ID1;
				outbuf[1] = ::libmaus::lz::GzipHeader::ID2;
				outbuf[2] = 8; // CM
				outbuf[3] = 4; // FLG, extra data
				outbuf[4] = outbuf[5] = outbuf[6] = outbuf[7] = 0; // MTIME
				outbuf[8] = 0; // XFL
				outbuf[9] = 255; // undefined OS
				outbuf[10] = 6; outbuf[11] = 0; // xlen=6
				outbuf[12] = 'B';
				outbuf[13] = 'C';
				outbuf[14] = 2; outbuf[15] = 0; // length of field
				outbuf[16] = outbuf[17] = 0; // block length, to be filled later
			}

			static uint8_t const * fillHeaderFooter(
				uint8_t const * const pa,
				uint8_t * const outbuf,
				unsigned int const payloadsize, 
				unsigned int const uncompsize
			)
			{
				// set size of compressed block in header
				unsigned int const blocksize = headersize/*header*/+8/*footer*/+payloadsize-1;
				assert ( blocksize < 64*1024 );
				outbuf[16] = (blocksize >> 0) & 0xFF;
				outbuf[17] = (blocksize >> 8) & 0xFF;
				
				uint8_t * footptr = outbuf + headersize + payloadsize;

				// compute crc of uncompressed data
				uint32_t crc = crc32(0,0,0);
				crc = crc32(crc, reinterpret_cast<Bytef const *>(pa), uncompsize);
				
				// crc
				*(footptr++) = (crc >> 0)  & 0xFF;
				*(footptr++) = (crc >> 8)  & 0xFF;
				*(footptr++) = (crc >> 16) & 0xFF;
				*(footptr++) = (crc >> 24) & 0xFF;
				// uncompressed size
				*(footptr++) = (uncompsize >> 0) & 0xFF;
				*(footptr++) = (uncompsize >> 8) & 0xFF;
				*(footptr++) = (uncompsize >> 16) & 0xFF;
				*(footptr++) = (uncompsize >> 24) & 0xFF;
				
				return footptr;
			}

			static uint64_t getOutBufSizeTwo(int const level)
			{
				return std::max(static_cast<uint64_t>(maxblocksize),getReqBufSpaceTwo(level));
			}
		};

		struct BgzfDeflateOutputBufferBase : public BgzfDeflateHeaderFunctions
		{
			::libmaus::autoarray::AutoArray<uint8_t> outbuf;

			BgzfDeflateOutputBufferBase(int const level) : outbuf(getOutBufSizeTwo(level)) 
			{
				setupHeader(outbuf.begin());			
			} 
		};
		
		struct BgzfDeflateInputBufferBase : public BgzfDeflateHeaderFunctions
		{
			::libmaus::autoarray::AutoArray<uint8_t> inbuf;
			
			uint8_t * const pa;
			uint8_t * pc;
			uint8_t * const pe;
		
			BgzfDeflateInputBufferBase() : inbuf(maxblocksize), pa(inbuf.begin()), pc(pa), pe(inbuf.end())
			{
			}
		};

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
