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
			bool terminated;
			bool flushed;
			
			std::string openfilename;
			
			static int doDeflate(z_stream * strm, int flush)
			{
				#if defined(LINESPLITTINGGZIPOUTPUTSTREAMDEBUG)
				std::cerr << "deflate(";
				
				switch ( flush )
				{
					case Z_NO_FLUSH: std::cerr << "Z_NO_FLUSH"; break;
					case Z_PARTIAL_FLUSH: std::cerr << "Z_PARTIAL_FLUSH"; break;
					case Z_SYNC_FLUSH: std::cerr << "Z_SYNC_FLUSH"; break;
					case Z_FULL_FLUSH: std::cerr << "Z_FULL_FLUSH"; break;
					case Z_FINISH: std::cerr << "Z_FINISH"; break;				
				}
				
				std::cerr << ")\n";
				#endif
			
				return ::deflate(strm,flush);
			}

			void doInit(int const level)
			{
				#if defined(LINESPLITTINGGZIPOUTPUTSTREAMDEBUG)
				std::cerr << "doInit()" << std::endl;
				#endif
			
				isize = 0;
				usize = 0;
				flushed = true;
				crc = crc32(0,0,0);
				
				memset ( &strm , 0, sizeof(z_stream) );
				strm.zalloc = Z_NULL;
				strm.zfree = Z_NULL;
				strm.opaque = Z_NULL;
				int ret = deflateInit2(&strm, level, Z_DEFLATED, -15 /* window size */, 
					8 /* mem level, gzip default */, Z_DEFAULT_STRATEGY);
				if ( ret != Z_OK )
				{
					#if defined(LINESPLITTINGGZIPOUTPUTSTREAMDEBUG)
					std::cerr << "doInit() deflateInit2 failed" << std::endl;
					#endif
				
					::libmaus::exception::LibMausException se;
					se.getStream() << "LineSplittingGzipOutputStreamBuffer::doInit(): deflateInit2 failed";
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
					int ret = doDeflate(&strm,Z_NO_FLUSH);
					if ( ret < 0 )
					{
						#if defined(LINESPLITTINGGZIPOUTPUTSTREAMDEBUG)
						std::cerr << "doSync() deflate failed" << std::endl;
						#endif
					
						::libmaus::exception::LibMausException se;
						se.getStream() << "LineSplittingGzipOutputStreamBuffer::doSync: deflate failed: " << ret << " (" << strm.msg << ")";
						se.finish();
						throw se;
					}
					flushed = false;
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
				if ( (! terminated) && (!flushed) )
				{
					int ret;
					
					do
					{
						strm.avail_in = 0;
						strm.next_in = 0;
						strm.avail_out = outbuffer.size();
						strm.next_out = reinterpret_cast<Bytef *>(outbuffer.begin());
						ret = doDeflate(&strm,Z_FULL_FLUSH);
						if ( ret < 0 )
						{
							#if defined(LINESPLITTINGGZIPOUTPUTSTREAMDEBUG)
							std::cerr << "doFullFlush deflate failed" << std::endl;
							#endif
						
							::libmaus::exception::LibMausException se;
							se.getStream() << "LineSplittingGzipOutputStreamBuffer::doFullFlush: deflate failed with error " << ret << " (" << strm.msg << ")";
							se.finish();
							throw se;
						}
						flushed = true;
						uint64_t have = outbuffer.size() - strm.avail_out;
						Pout->write(reinterpret_cast<char const *>(outbuffer.begin()),have);
						
					} while (strm.avail_out == 0);
											
					assert ( ret == Z_OK );
				}
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
					ret = doDeflate(&strm,Z_FINISH);
					if ( ret == Z_STREAM_ERROR )
					{
						#if defined(LINESPLITTINGGZIPOUTPUTSTREAMDEBUG)
						std::cerr << "doFinish deflate failed" << std::endl;
						#endif
					
						::libmaus::exception::LibMausException se;
						se.getStream() << "LineSplittingGzipOutputStreamBuffer::doFinish(): deflate failed: " << ret << " (" << strm.msg << ")";
						se.finish();
						throw se;
					}
					flushed = true;
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
				terminated = true;

				#if defined(LINESPLITTINGGZIPOUTPUTSTREAMDEBUG)
				std::cerr << "doTerminate() " << terminated << std::endl;
				#endif
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
				terminated = false;
			}

			public:
			LineSplittingGzipOutputStreamBuffer(
				std::string const & rfn, 
				uint64_t const rlinemod,
				uint64_t const rbuffersize, 
				int const rlevel = Z_DEFAULT_COMPRESSION
			)
			: fn(rfn), level(rlevel), fileno(0), linemod(rlinemod), linecnt(0), buffersize(rbuffersize), inbuffer(buffersize,false), outbuffer(buffersize,false), reopenpending(false), terminated(true), flushed(false)
			{
				doOpen();
				setp(inbuffer.begin(),inbuffer.end()-1);
			}

			~LineSplittingGzipOutputStreamBuffer()
			{
				#if defined(LINESPLITTINGGZIPOUTPUTSTREAMDEBUG)
				std::cerr << "Destructor doSync()" << std::endl;
				#endif
			
				doSync();
				
				bool const deletefile = (fileno==1) && (usize==0);
				std::string deletefilename = openfilename;
				
				#if defined(LINESPLITTINGGZIPOUTPUTSTREAMDEBUG)
				std::cerr << "Destructor doTerminate()" << std::endl;
				#endif
				
				doTerminate();
				
				if ( deletefile )
					remove(deletefilename.c_str());
					
				#if defined(LINESPLITTINGGZIPOUTPUTSTREAMDEBUG)
				std::cerr << "Destructor done." << std::endl;
				#endif
			}

			int_type overflow(int_type c = traits_type::eof())
			{
				#if defined(LINESPLITTINGGZIPOUTPUTSTREAMDEBUG)
				std::cerr << "overflow()" << std::endl;
				#endif
				
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
				#if defined(LINESPLITTINGGZIPOUTPUTSTREAMDEBUG)
				std::cerr << "sync()" << std::endl;
				#endif
				
				// flush input buffer
				doSync();
				
				#if defined(LINESPLITTINGGZIPOUTPUTSTREAMDEBUG)
				std::cerr << "sync doSync() done" << std::endl;
				#endif

				if ( ! terminated )
				{
					#if defined(LINESPLITTINGGZIPOUTPUTSTREAMDEBUG)
					std::cerr << "sync() entring doFullFlush" << std::endl;
					#endif
				
					// flush zlib state
					doFullFlush();
				}
			
				#if defined(LINESPLITTINGGZIPOUTPUTSTREAMDEBUG)
				std::cerr << "sync() flushing output stream" << std::endl;
				#endif
					
				// flush output stream
				Pout->flush();	
			
				#if defined(LINESPLITTINGGZIPOUTPUTSTREAMDEBUG)
				std::cerr << "sync() done." << std::endl;
				#endif
			
				return 0; // no error, -1 for error
			}			
		};
	}
}
#endif
