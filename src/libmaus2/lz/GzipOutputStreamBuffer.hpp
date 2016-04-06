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
#if ! defined(LIBMAUS2_LZ_GZIPOUTPUTSTREAMBUFFER_HPP)
#define LIBMAUS2_LZ_GZIPOUTPUTSTREAMBUFFER_HPP

#include <zlib.h>
#include <ostream>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/lz/GzipHeader.hpp>
#include <libmaus2/lz/ZlibInterface.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct GzipOutputStreamBuffer : public ::std::streambuf
		{
			private:
			std::ostream & out;
			uint64_t const buffersize;
			::libmaus2::autoarray::AutoArray<char> inbuffer;
			::libmaus2::autoarray::AutoArray<char> outbuffer;
			libmaus2::lz::ZlibInterface::unique_ptr_type zintf;
			uint32_t crc;
			uint32_t isize;

			uint64_t compressedwritten;
			bool terminated;

			void init(int const level)
			{
				zintf->eraseContext();
				zintf->setZAlloc(Z_NULL);
				zintf->setZFree(Z_NULL);
				zintf->setOpaque(Z_NULL);
				int ret = zintf->z_deflateInit2(level, Z_DEFLATED, -15 /* window size */,
					8 /* mem level, gzip default */, Z_DEFAULT_STRATEGY);
				if ( ret != Z_OK )
				{
					::libmaus2::exception::LibMausException se;
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
					zintf->setAvailIn(n);
					zintf->setNextIn(reinterpret_cast<Bytef *>(pbase()));

					crc = crc32(crc, zintf->getNextIn(), zintf->getAvailIn());
					isize += zintf->getAvailIn();

					do
					{
						zintf->setAvailOut(outbuffer.size());
						zintf->setNextOut(reinterpret_cast<Bytef *>(outbuffer.begin()));
						int ret = zintf->z_deflate(Z_NO_FLUSH);
						if ( ret == Z_STREAM_ERROR )
						{
							::libmaus2::exception::LibMausException se;
							se.getStream() << "deflate failed (Z_STREAM_ERROR).";
							se.finish();
							throw se;
						}
						uint64_t const have = outbuffer.size() - zintf->getAvailOut();
						out.write(reinterpret_cast<char const *>(outbuffer.begin()),have);
						compressedwritten += have;
					}
					while (zintf->getAvailOut() == 0);

					assert ( zintf->getAvailIn() == 0);
				}
				else if ( n )
				{
					::libmaus2::exception::LibMausException se;
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
						zintf->setAvailIn(0);
						zintf->setNextIn(Z_NULL);
						zintf->setAvailOut(outbuffer.size());
						zintf->setNextOut(reinterpret_cast<Bytef *>(outbuffer.begin()));
						ret = zintf->z_deflate(Z_FULL_FLUSH);
						if ( ret == Z_STREAM_ERROR )
						{
							::libmaus2::exception::LibMausException se;
							se.getStream() << "GzipOutputStreamBuffer::deflate() failed.";
							se.finish();
							throw se;
						}
						uint64_t have = outbuffer.size() - zintf->getAvailOut();
						out.write(reinterpret_cast<char const *>(outbuffer.begin()),have);
						compressedwritten += have;

						// std::cerr << "Writing " << have << " bytes in flush" << std::endl;
					} while (zintf->getAvailOut() == 0);

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
						zintf->setAvailIn(0);
						zintf->setNextIn(Z_NULL);
						zintf->setAvailOut(outbuffer.size());
						zintf->setNextOut(reinterpret_cast<Bytef *>(outbuffer.begin()));
						ret = zintf->z_deflate(Z_FINISH);
						if ( ret == Z_STREAM_ERROR )
						{
							::libmaus2::exception::LibMausException se;
							se.getStream() << "GzipOutputStreamBuffer::deflate() failed.";
							se.finish();
							throw se;
						}
						uint64_t have = outbuffer.size() - zintf->getAvailOut();
						out.write(reinterpret_cast<char const *>(outbuffer.begin()),have);
						compressedwritten += have;

						// std::cerr << "Writing " << have << " bytes in flush" << std::endl;
					} while (zintf->getAvailOut() == 0);

					assert ( ret == Z_STREAM_END );
				}
			}

			uint64_t doTerminate()
			{
				if ( ! terminated )
				{
					doSync();
					doFinish();

					zintf->z_deflateEnd();

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
			  zintf(libmaus2::lz::ZlibInterface::construct()),
			  crc(crc32(0,0,0)), isize(0), compressedwritten(0), terminated(false)
			{
				compressedwritten += libmaus2::lz::GzipHeaderConstantsBase::writeSimpleHeader(out);
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
