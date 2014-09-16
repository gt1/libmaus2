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
#if ! defined(LIBMAUS_LZ_LINESPLITTINGGZIPOUTPUTSTREAMBUFFER_HPP)
#define LIBMAUS_LZ_LINESPLITTINGGZIPOUTPUTSTREAMBUFFER_HPP

#include <zlib.h>
#include <ostream>
#include <libmaus/aio/PosixFdOutputStream.hpp>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/lz/GzipHeader.hpp>

namespace libmaus
{
	namespace lz
	{
		struct LineSplittingGzipOutputStreamBuffer : public ::std::streambuf
		{
			private:
			std::string fn;
			int level;
			uint64_t fileno;
			uint64_t linemod;
			uint64_t linecnt;
			
			libmaus::aio::PosixFdOutputStream::unique_ptr_type Pout;
			
			uint64_t const buffersize;
			::libmaus::autoarray::AutoArray<char> inbuffer;
			::libmaus::autoarray::AutoArray<char> outbuffer;
			z_stream strm;
			uint32_t crc;
			uint32_t isize;
			uint64_t usize;
			
			bool reopenpending;
			
			std::string openfilename;

			void doInit(int const level)
			{
				isize = 0;
				usize = 0;
				crc = crc32(0,0,0);
				
				memset ( &strm , 0, sizeof(z_stream) );
				strm.zalloc = Z_NULL;
				strm.zfree = Z_NULL;
				strm.opaque = Z_NULL;
				int ret = deflateInit2(&strm, level, Z_DEFLATED, -15 /* window size */, 
					8 /* mem level, gzip default */, Z_DEFAULT_STRATEGY);
				if ( ret != Z_OK )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "LineSplittingGzipOutputStreamBuffer::inti(): deflateInit2 failed";
					se.finish();
					throw se;
				}
			}
			
			void doSync(char * p, int64_t const n)
			{
				strm.avail_in = n;
				strm.next_in  = reinterpret_cast<Bytef *>(p);

				crc = crc32(crc, strm.next_in, strm.avail_in);        
				isize += strm.avail_in;
				usize += strm.avail_in;

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
					Pout->write(reinterpret_cast<char const *>(outbuffer.begin()),have);
				} 
				while (strm.avail_out == 0);

				assert ( strm.avail_in == 0);					
			}
			
			void doSync()
			{
				int64_t n = pptr()-pbase();
				pbump(-n);
				char * p = pbase();

				while ( n )
				{
					if ( reopenpending )
					{
						doTerminate();
						doOpen();

						reopenpending = false;
					}

					char * pc = p;
					char * pe = pc+n;
					
					while ( pc != pe )
						if ( *(pc++) == '\n' && ((++linecnt)%linemod==0) )
						{
							reopenpending = true;
							break;
						}
					
					uint64_t const t = pc - p;
					
					doSync(p,t);
					
					p += t;
					n -= t;					
				}
			}

			void doFullFlush()
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
						se.getStream() << "LineSplittingGzipOutputStreamBuffer::deflate() failed.";
						se.finish();
						throw se;
					}
					uint64_t have = outbuffer.size() - strm.avail_out;
					Pout->write(reinterpret_cast<char const *>(outbuffer.begin()),have);
					
				} while (strm.avail_out == 0);
						
				assert ( ret == Z_OK );
			}

			void doFinish()
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
						se.getStream() << "LineSplittingGzipOutputStreamBuffer::deflate() failed.";
						se.finish();
						throw se;
					}
					uint64_t have = outbuffer.size() - strm.avail_out;
					Pout->write(reinterpret_cast<char const *>(outbuffer.begin()),have);
					
				} while (strm.avail_out == 0);
						
				assert ( ret == Z_STREAM_END );
			}
			
			void doTerminate()
			{
				doFinish();

				deflateEnd(&strm);

				// crc
				Pout->put ( (crc >> 0)  & 0xFF );
				Pout->put ( (crc >> 8)  & 0xFF );
				Pout->put ( (crc >> 16) & 0xFF );
				Pout->put ( (crc >> 24) & 0xFF );
				// uncompressed size
				Pout->put ( (isize >> 0)  & 0xFF );
				Pout->put ( (isize >> 8)  & 0xFF );
				Pout->put ( (isize >> 16) & 0xFF );
				Pout->put ( (isize >> 24) & 0xFF );

				// reset output stream
				Pout.reset();
				
				openfilename = std::string();
				usize = 0;
			}
			
			void doOpen()
			{
				// create new file name
				std::ostringstream fnostr;
				fnostr << fn << "_" << std::setw(6) << std::setfill('0') <<  fileno++ << std::setw(0) << ".gz";
				std::string const filename = fnostr.str();
				
				// open output stream
				libmaus::aio::PosixFdOutputStream::unique_ptr_type Tout(new libmaus::aio::PosixFdOutputStream(filename));
				Pout = UNIQUE_PTR_MOVE(Tout);

				// write gzip header
				libmaus::lz::GzipHeaderConstantsBase::writeSimpleHeader(*Pout);
				// init compressor
				doInit(level);
				
				openfilename = filename;
			}

			public:
			LineSplittingGzipOutputStreamBuffer(
				std::string const & rfn, 
				uint64_t const rlinemod,
				uint64_t const rbuffersize, 
				int const rlevel = Z_DEFAULT_COMPRESSION
			)
			: fn(rfn), level(rlevel), fileno(0), linemod(rlinemod), linecnt(0), buffersize(rbuffersize), inbuffer(buffersize,false), outbuffer(buffersize,false), reopenpending(false)
			{
				doOpen();
				setp(inbuffer.begin(),inbuffer.end()-1);
			}

			~LineSplittingGzipOutputStreamBuffer()
			{
				doSync();
				
				bool const deletefile = (fileno==1) && (usize==0);
				std::string deletefilename = openfilename;
				
				doTerminate();
				
				if ( deletefile )
					remove(deletefilename.c_str());
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
				Pout->flush();	
				return 0; // no error, -1 for error
			}			
		};
	}
}
#endif
