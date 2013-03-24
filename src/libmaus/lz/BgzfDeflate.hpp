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
#if ! defined(LIBMAUS_LZ_BGZFDEFLATE_HPP)
#define LIBMAUS_LZ_BGZFDEFLATE_HPP

#include <libmaus/lz/GzipHeader.hpp>
#include <zlib.h>

namespace libmaus
{
	namespace lz
	{
		template<typename _stream_type>
		struct BgzfDeflate
		{
			static unsigned int const maxblocksize = 64*1024;
			static unsigned int const headersize = 18;
			static unsigned int const footersize = 8;
			static unsigned int const maxpayload = maxblocksize - (headersize+footersize);
			
			typedef _stream_type stream_type;

			stream_type & stream;
			z_stream strm;
			unsigned int deflbound;
			::libmaus::autoarray::AutoArray<uint8_t> inbuf;
			::libmaus::autoarray::AutoArray<uint8_t> outbuf;
			
			uint8_t * const pa;
			uint8_t * pc;
			uint8_t * const pe;
			
			void destroy()
			{
				deflateEnd(&strm);		
			}
			
			void init(int const level = Z_DEFAULT_COMPRESSION)
			{
				memset ( &strm , 0, sizeof(z_stream) );
				strm.zalloc = Z_NULL;
				strm.zfree = Z_NULL; 
				strm.opaque = Z_NULL;
				int ret = deflateInit2(&strm, level, Z_DEFLATED, -15 /* window size */,
				8 /* mem level, gzip default */, Z_DEFAULT_STRATEGY);
				if ( ret != Z_OK )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "deflateInit2 failed." << std::endl;
					se.finish();
					throw se;
				}

				// search for number of bytes that will never produce more compressed space than we have
				unsigned int bound = 64*1024;
				
				while ( deflateBound(&strm,bound) > (maxblocksize-(headersize+footersize)) )
					--bound;

				deflbound = bound;
			}
			
			void reinit(int const level = Z_DEFAULT_COMPRESSION)
			{
				destroy();
				init(level);
			}

			BgzfDeflate(stream_type & rstream, int const level = Z_DEFAULT_COMPRESSION)
			: stream(rstream), inbuf(maxblocksize), outbuf(maxblocksize),
			  pa(inbuf.begin()),
			  pc(pa),
			  pe(inbuf.end())
			{
				init(level);        

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
			~BgzfDeflate()
			{
				destroy();
			}
			
			void writeBlock(unsigned int const payloadsize, unsigned int const uncompsize)
			{
				// set size of compressed block in header
				unsigned int const blocksize = headersize/*header*/+8/*footer*/+payloadsize-1;
				assert ( blocksize < 64*1024 );
				outbuf[16] = (blocksize >> 0) & 0xFF;
				outbuf[17] = (blocksize >> 8) & 0xFF;
				
				uint8_t * footptr = outbuf.begin() + headersize + payloadsize;

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
				
				// write data
				stream.write(reinterpret_cast<char const *>(outbuf.begin()), footptr - outbuf.begin());
				if ( ! stream )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "failed to write compressed data to bgzf stream." << std::endl;
					se.finish();
					throw se;				
				}		
			}

			void flushBound()
			{
				if ( deflateReset(&strm) != Z_OK )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "deflateReset failed." << std::endl;
					se.finish();
					throw se;		
				}
				
				unsigned int const toflush = std::min(static_cast<unsigned int>(pc-pa),deflbound);
				unsigned int const unflushed = (pc-pa)-toflush;

				strm.avail_out = maxpayload;
				strm.next_out  = reinterpret_cast<Bytef *>(outbuf.begin()) + headersize;
				strm.avail_in  = toflush;
				strm.next_in   = pa;
				
				if ( deflate(&strm,Z_FINISH) != Z_STREAM_END )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "deflate failed." << std::endl;
					se.finish();
					throw se;		
				}
				
				/*
				 * write out compressed data
				 */
				unsigned int const payloadsize = maxpayload - strm.avail_out;
				writeBlock(payloadsize,toflush);
				
				/*
				 * copy rest of uncompressed data to front of buffer
				 */
				if ( unflushed )
					memmove(pa,pc-unflushed,unflushed);		

				// set new output pointer
				pc = pa + unflushed;
			}

			void flush()
			{
				if ( deflateReset(&strm) != Z_OK )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "deflateReset failed." << std::endl;
					se.finish();
					throw se;		
				}
				
				strm.avail_out = maxpayload;
				strm.next_out  = reinterpret_cast<Bytef *>(outbuf.begin()) + headersize;
				strm.avail_in  = pc-pa;
				strm.next_in   = pa;
				
				if ( deflate(&strm,Z_FINISH) != Z_STREAM_END )
				{
					flushBound();
				}
				else
				{
					unsigned int const payloadsize = maxpayload - strm.avail_out;
					writeBlock(payloadsize,pc-pa);
				
					pc = pa;
				}
			}
			
			void write(char const * const cp, unsigned int n)
			{
				uint8_t const * p = reinterpret_cast<uint8_t const *>(cp);
			
				while ( n )
				{
					unsigned int const towrite = std::min(n,static_cast<unsigned int>(pe-pc));
					std::copy(p,p+towrite,pc);

					p += towrite;
					pc += towrite;
					n -= towrite;
					
					if ( pc == pe )
						flush();
				}
			}
			
			void put(uint8_t const c)
			{
				*(pc++) = c;
				if ( pc == pe )
					flush();
			}
			
			void addEOFBlock()
			{
				// flush, if there is any uncompressed data
				if ( pc != pa )
					flush();
				// set compression level to default
				reinit();
				// write empty block
				flush();
			}
		};                                                                                                                                                                                                                                                                                                                                                                                                                                                        
	}
}
#endif
