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
#if ! defined(LIBMAUS_LZ_GZIPOUTPUTSTREAMBUFFER_HPP)
#define LIBMAUS_LZ_GZIPOUTPUTSTREAMBUFFER_HPP

#include <zlib.h>
#include <ostream>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/lz/GzipHeader.hpp>

namespace libmaus
{
	namespace lz
	{
		struct GzipOutputStreamBuffer : public ::std::streambuf
		{
			private:
			std::ostream & out;
			uint64_t const buffersize;
			::libmaus::autoarray::AutoArray<char> inbuffer;
			::libmaus::autoarray::AutoArray<char> outbuffer;
			z_stream strm;
			uint32_t crc;
			uint32_t isize;
			
			uint64_t compressedwritten;
			bool terminated;

			void init(int const level)
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
					se.getStream() << "GzipOutputStreamBuffer::inti(): deflateInit2 failed";
					se.finish();
					throw se;
				}
			}

			void doSync()
			{
				int64_t const n = pptr()-pbase();
				pbump(-n);

				if ( ! terminated )
				{
					strm.avail_in = n;
					strm.next_in  = reinterpret_cast<Bytef *>(pbase());

					crc = crc32(crc, strm.next_in, strm.avail_in);        
					isize += strm.avail_in;

					do
					{
						strm.avail_out = outbuffer.size();
						strm.next_out  = reinterpret_cast<Bytef *>(outbuffer.begin());
						int ret = deflate(&strm,Z_NO_FLUSH);
						if ( ret == Z_STREAM_ERROR )
						{
							::libmaus::exception::LibMausException se;
							se.getStream() << "deflate failed (Z_STREAM_ERROR).";
							se.finish();
							throw se;
						}
						uint64_t const have = outbuffer.size() - strm.avail_out;
						out.write(reinterpret_cast<char const *>(outbuffer.begin()),have);
						compressedwritten += have;
					} 
					while (strm.avail_out == 0);

					assert ( strm.avail_in == 0);					
				}
				else if ( n )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "GzipOutputStreamBuffer: data was written on terminated stream.";
					se.finish();
					throw se;					
				}
			}

			void doFullFlush()
			{
				if ( ! terminated )
				{
					int ret;
					
					do
					{
						strm.avail_in = 0;
						strm.next_in = 0;
						strm.avail_out = outbuffer.size();
						strm.next_out = reinterpret_cast<Bytef *>(outbuffer.begin());
						ret = deflate(&strm,Z_FULL_FLUSH);
						if ( ret == Z_STREAM_ERROR )
						{
							::libmaus::exception::LibMausException se;
							se.getStream() << "GzipOutputStreamBuffer::deflate() failed.";
							se.finish();
							throw se;
						}
						uint64_t have = outbuffer.size() - strm.avail_out;
						out.write(reinterpret_cast<char const *>(outbuffer.begin()),have);
						compressedwritten += have;
						
						// std::cerr << "Writing " << have << " bytes in flush" << std::endl;
					} while (strm.avail_out == 0);
							
					assert ( ret == Z_OK );
				}			
			}


			void doFinish()
			{
				if ( ! terminated )
				{
					int ret;

					do
					{
						strm.avail_in = 0;
						strm.next_in = 0;
						strm.avail_out = outbuffer.size();
						strm.next_out = reinterpret_cast<Bytef *>(outbuffer.begin());
						ret = deflate(&strm,Z_FINISH );
						if ( ret == Z_STREAM_ERROR )
						{
							::libmaus::exception::LibMausException se;
							se.getStream() << "GzipOutputStreamBuffer::deflate() failed.";
							se.finish();
							throw se;
						}
						uint64_t have = outbuffer.size() - strm.avail_out;
						out.write(reinterpret_cast<char const *>(outbuffer.begin()),have);
						compressedwritten += have;
						
						// std::cerr << "Writing " << have << " bytes in flush" << std::endl;
					} while (strm.avail_out == 0);
							
					assert ( ret == Z_STREAM_END );
				}
			}
			
			uint64_t doTerminate()
			{
				if ( ! terminated )
				{
					doSync();
					doFinish();

					deflateEnd(&strm);

					// crc
					out . put ( (crc >> 0)  & 0xFF );
					out . put ( (crc >> 8)  & 0xFF );
					out . put ( (crc >> 16) & 0xFF );
					out . put ( (crc >> 24) & 0xFF );
					// uncompressed size
					out . put ( (isize >> 0)  & 0xFF );
					out . put ( (isize >> 8)  & 0xFF );
					out . put ( (isize >> 16) & 0xFF );
					out . put ( (isize >> 24) & 0xFF );
									
					compressedwritten += 8;

					out.flush();

					terminated = true;			
				}
				
				return compressedwritten;
			}

			public:
			GzipOutputStreamBuffer(std::ostream & rout, uint64_t const rbuffersize, int const level = Z_DEFAULT_COMPRESSION)
			: out(rout), buffersize(rbuffersize), inbuffer(buffersize,false), outbuffer(buffersize,false),
			  crc(crc32(0,0,0)), isize(0), compressedwritten(0), terminated(false)
			{
				compressedwritten += libmaus::lz::GzipHeaderConstantsBase::writeSimpleHeader(out);
				init(level);
				setp(inbuffer.begin(),inbuffer.end()-1);
			}
			
			~GzipOutputStreamBuffer()
			{
				doTerminate();
			}
			
			/**
			 * terminate stream
			 *
			 * @return number of bytes written to compressed stream (complete gzip file)
			 **/
			uint64_t terminate()
			{
				return doTerminate();
			}
			
			int_type overflow(int_type c = traits_type::eof())
			{
				if ( c != traits_type::eof() )
				{
					*pptr() = c;
					pbump(1);
					doSync();
				}

				return c;
			}
			
			
			int sync()
			{
				// flush input buffer
				doSync();
				// flush zlib state
				doFullFlush();
				// flush output stream
				out.flush();	
				return 0; // no error, -1 for error
			}			
		};
	}
}
#endif
