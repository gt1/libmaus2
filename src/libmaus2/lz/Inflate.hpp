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
#include <cassert>

#include <libmaus2/timing/RealTimeClock.hpp>

#include <zlib.h>
#include <libmaus2/lz/ZlibInterface.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct Inflate
		{
			static unsigned int const input_buffer_size = 64*1024;

			private:
			libmaus2::lz::ZlibInterface::unique_ptr_type strm;

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
				if ( (ret=strm->z_inflateReset()) != Z_OK )
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
				strm->eraseContext();
				strm->setZAlloc(Z_NULL);
				strm->setZFree(Z_NULL);
				strm->setOpaque(Z_NULL);
				strm->setAvailIn(0);
				strm->setNextIn(Z_NULL);

				if ( windowSizeLog )
					ret = strm->z_inflateInit2(windowSizeLog);
				else
					ret = strm->z_inflateInit();

				if (ret != Z_OK)
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "inflate failed in inflateInit";
					se.finish();
					throw se;
				}
			}

			Inflate(std::istream & rin, int windowSizeLog = 0)
			: strm(libmaus2::lz::ZlibInterface::construct()), in(rin), inbuf(input_buffer_size,false), outbuf(16*1024,false), outbuffill(0), op(0)
			{
				init(windowSizeLog);
			}
			Inflate(std::string const & filename, int windowSizeLog = 0)
			:
			  strm(libmaus2::lz::ZlibInterface::construct()),
			  pin(new istr_file_type(filename)),
			  in(*pin), inbuf(input_buffer_size,false), outbuf(16*1024,false), outbuffill(0), op(0)
			{
				init(windowSizeLog);
			}
			~Inflate()
			{
				strm->z_inflateEnd();
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
						strm->setAvailIn(0);
						strm->setNextIn(0);
						return false;
					}
					else
					{
						// std::cerr << "Got " << read << " in fillBuffer()" << std::endl;
					}

					strm->setAvailIn(read);
					strm->setNextIn(reinterpret_cast<Bytef *>(inbuf.get()));

					while ( (strm->getAvailIn() != 0) && (ret == Z_OK) )
					{
						if ( outbuffill == outbuf.size() )
							outbuf.resize(outbuf.size() + 16*1024);

						uint64_t const avail_out = outbuf.size() - outbuffill;
						strm->setAvailOut(avail_out);
						strm->setNextOut(reinterpret_cast<Bytef *>(outbuf.get() + outbuffill));
						ret = strm->z_inflate(Z_NO_FLUSH);

						if ( ret == Z_OK || ret == Z_STREAM_END )
							outbuffill += (avail_out - strm->getAvailOut());
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
					strm->getNextIn(),strm->getNextIn()+strm->getAvailIn()
				);
			}

			void ungetRest()
			{
				if ( in.eof() )
					in.clear();

				for ( uint64_t i = 0; i < strm->getAvailIn(); ++i )
					in.putback(strm->getNextIn()[strm->getAvailIn()-i-1]);
			}
		};
	}
}
#endif
