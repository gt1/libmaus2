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

#if ! defined(LIBMAUS2_LZ_INFLATE_HPP)
#define LIBMAUS2_LZ_INFLATE_HPP

#include <libmaus2/types/types.hpp>
#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/exception/LibMausException.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/util/NumberSerialisation.hpp>
#include <zlib.h>
#include <cassert>

#include <libmaus2/timing/RealTimeClock.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct Inflate
		{
			static unsigned int const input_buffer_size = 64*1024;

			private:
			z_stream strm;

			public:
			typedef libmaus2::aio::InputStreamInstance istr_file_type;
			typedef ::libmaus2::util::unique_ptr<istr_file_type>::type istr_file_ptr_type;

			istr_file_ptr_type pin;
			std::istream & in;
			::libmaus2::autoarray::AutoArray<uint8_t> inbuf;
			::libmaus2::autoarray::AutoArray<uint8_t,::libmaus2::autoarray::alloc_type_c> outbuf;
			uint64_t outbuffill;
			uint8_t const * op;
			int ret;

			void zreset()
			{
				if ( (ret=inflateReset(&strm)) != Z_OK )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "Inflate::zreset(): inflateReset failed";
					se.finish();
					throw se;
				}

				ret = Z_OK;
				outbuffill = 0;
				op = 0;
			}

			void init(int windowSizeLog)
			{
				memset(&strm,0,sizeof(z_stream));

				strm.zalloc = Z_NULL;
				strm.zfree = Z_NULL;
				strm.opaque = Z_NULL;
				strm.avail_in = 0;
				strm.next_in = Z_NULL;

				if ( windowSizeLog )
					ret = inflateInit2(&strm,windowSizeLog);
				else
					ret = inflateInit(&strm);

				if (ret != Z_OK)
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "inflate failed in inflateInit";
					se.finish();
					throw se;
				}
			}

			Inflate(std::istream & rin, int windowSizeLog = 0)
			: in(rin), inbuf(input_buffer_size,false), outbuf(16*1024,false), outbuffill(0), op(0)
			{
				init(windowSizeLog);
			}
			Inflate(std::string const & filename, int windowSizeLog = 0)
			: pin(new istr_file_type(filename)),
			  in(*pin), inbuf(input_buffer_size,false), outbuf(16*1024,false), outbuffill(0), op(0)
			{
				init(windowSizeLog);
			}
			~Inflate()
			{
				inflateEnd(&strm);
			}

			int get()
			{
				while ( ! outbuffill )
				{
					bool const ok = fillBuffer();
					if ( ! ok )
						return -1;

					// assert ( outbuffill );
				}

				// assert ( outbuffill );
				// std::cerr << "outbuffill=" << outbuffill << std::endl;

				int const c = *(op++);
				outbuffill--;

				return c;
			}

			uint64_t read(char * buffer, uint64_t const n)
			{
				uint64_t red = 0;

				while ( red < n )
				{
					while ( ! outbuffill )
					{
						bool const ok = fillBuffer();
						if ( ! ok )
							return red;
					}

					uint64_t const tocopy = std::min(outbuffill,n-red);
					std::copy ( op, op + tocopy, buffer + red );
					op += tocopy;
					red += tocopy;
					outbuffill -= tocopy;
				}

				return red;
			}

			bool fillBuffer()
			{
				if ( ret == Z_STREAM_END )
					return false;

				while ( (ret == Z_OK) && (! outbuffill) )
				{
					/* read a block of data */
					in.read ( reinterpret_cast<char *>(inbuf.get()), inbuf.size() );
					uint64_t const read = in.gcount();
					if ( ! read )
					{
						// std::cerr << "Found EOF in fillBuffer()" << std::endl;
						strm.avail_in = 0;
						strm.next_in = 0;
						return false;
					}
					else
					{
						// std::cerr << "Got " << read << " in fillBuffer()" << std::endl;
					}

					strm.avail_in = read;
					strm.next_in = reinterpret_cast<Bytef *>(inbuf.get());

					while ( (strm.avail_in != 0) && (ret == Z_OK) )
					{
						if ( outbuffill == outbuf.size() )
							outbuf.resize(outbuf.size() + 16*1024);

						uint64_t const avail_out = outbuf.size() - outbuffill;
						strm.avail_out = avail_out;
						strm.next_out = reinterpret_cast<Bytef *>(outbuf.get() + outbuffill);
						ret = inflate(&strm, Z_NO_FLUSH);

						if ( ret == Z_OK || ret == Z_STREAM_END )
							outbuffill += (avail_out - strm.avail_out);
					}
				}

				if ( ret != Z_OK && ret != Z_STREAM_END )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "Inflate::fillBuffer(): zlib state clobbered after inflate(), state is ";

					switch ( ret )
					{
						case Z_NEED_DICT: se.getStream() << "Z_NEED_DICT"; break;
						case Z_ERRNO: se.getStream() << "Z_ERRNO"; break;
						case Z_STREAM_ERROR: se.getStream() << "Z_STREAM_ERROR"; break;
						case Z_DATA_ERROR: se.getStream() << "Z_DATA_ERROR"; break;
						case Z_MEM_ERROR: se.getStream() << "Z_MEM_ERROR"; break;
						case Z_BUF_ERROR: se.getStream() << "Z_BUF_ERROR"; break;
						case Z_VERSION_ERROR: se.getStream() << "Z_VERSION_ERROR"; break;
						default: se.getStream() << "Unknown error code"; break;
					}
					se.getStream() << " output size " << outbuffill << std::endl;

					se.finish();
					throw se;
				}

				op = outbuf.begin();

				return true;
			}

			std::pair<uint8_t const *, uint8_t const *> getRest() const
			{
				return std::pair<uint8_t const *, uint8_t const *>(
					strm.next_in,strm.next_in+strm.avail_in
				);
			}

			void ungetRest()
			{
				if ( in.eof() )
					in.clear();

				for ( uint64_t i = 0; i < strm.avail_in; ++i )
					in.putback(strm.next_in[strm.avail_in-i-1]);
			}
		};
	}
}
#endif
