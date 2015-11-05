/*
    libmaus2
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#if ! defined(LIBMAUS2_LZ_RAZFDECODERBUFFER_HPP)
#define LIBMAUS2_LZ_RAZFDECODERBUFFER_HPP

#include <libmaus2/lz/RAZFIndex.hpp>
#include <streambuf>
#include <istream>

namespace libmaus2
{
	namespace lz
	{
		struct RAZFDecoderBuffer : public ::std::streambuf, public ::libmaus2::lz::RAZFConstants
		{
			private:
			::libmaus2::aio::InputStreamInstance::unique_ptr_type Pfilestream;
			std::istream & stream;

			libmaus2::lz::RAZFIndex const index;

			z_stream zstream;

			::libmaus2::autoarray::AutoArray<char> inbuffer;
			::libmaus2::autoarray::AutoArray<char> outbuffer;

			uint64_t symsread;

			RAZFDecoderBuffer(RAZFDecoderBuffer const &);
			RAZFDecoderBuffer & operator=(RAZFDecoderBuffer &);

			void init()
			{
				zstream.zalloc = Z_NULL;
				zstream.zfree = Z_NULL;
				zstream.opaque = Z_NULL;
				zstream.avail_in = 0;
				zstream.next_in = Z_NULL;
				zstream.avail_out = 0;
				zstream.next_out = Z_NULL;

				int const ok = inflateInit2(&zstream,-static_cast<int>(razf_window_bits));

				if ( ok != Z_OK )
				{
					libmaus2::exception::LibMausException se;
					se.getStream() << "RAZFDecoderBuffer::init(): inflateInit2() failed" << std::endl;
					se.finish();
					throw se;
				}
			}

			void setup()
			{
				setg(outbuffer.end(), outbuffer.end(), outbuffer.end());
				stream.clear();
				stream.seekg(index[symsread / razf_block_size],std::ios::beg);

				zstream.avail_in = 0;
				zstream.next_in = Z_NULL;
				zstream.avail_out = 0;
				zstream.next_out = Z_NULL;

				if ( inflateReset(&zstream) != Z_OK )
				{
					libmaus2::exception::LibMausException se;
					se.getStream() << "RAZFDecoderBuffer::setup(): inflateReset() failed" << std::endl;
					se.finish();
					throw se;
				}
			}

			public:
			RAZFDecoderBuffer(std::string const & filename)
			: Pfilestream(new ::libmaus2::aio::InputStreamInstance(filename)),
			  stream(*Pfilestream),
			  index(stream),
			  inbuffer(razf_block_size,false),
			  outbuffer(razf_block_size,false),
			  symsread(0)
			{
				init();
				setup();
			}

			RAZFDecoderBuffer(std::istream & rstream)
			: Pfilestream(),
			  stream(rstream),
			  index(stream),
			  inbuffer(razf_block_size,false),
			  outbuffer(razf_block_size,false),
			  symsread(0)
			{
				init();
				setup();
			}

			~RAZFDecoderBuffer()
			{
				inflateEnd(&zstream);
			}

			private:
			// gptr as unsigned pointer
			uint8_t const * uptr() const
			{
				return reinterpret_cast<uint8_t const *>(gptr());
			}

			::std::streampos seekpos(::std::streampos sp, ::std::ios_base::openmode which = ::std::ios_base::in | ::std::ios_base::out)
			{
				if ( which & ::std::ios_base::in )
				{
					// current position
					int64_t const cur = symsread-(egptr()-gptr());
					// current start of buffer (relative)
					int64_t const curlow = cur - static_cast<int64_t>(gptr()-eback());
					// current end of buffer (relative)
					int64_t const curhigh = cur + static_cast<int64_t>(egptr()-gptr());

					// call relative seek, if target is in range
					if ( sp >= curlow && sp <= curhigh )
						return seekoff(static_cast<int64_t>(sp) - cur, ::std::ios_base::cur, which);

					// target is out of range, we really need to seek
					symsread = (sp / razf_block_size)*razf_block_size;
					//
					uint64_t const tsymsread = symsread;
					// run setup
					setup();
					// read next block
					underflow();
					// skip bytes in block to get to final position
					setg(eback(),gptr() + (static_cast<int64_t>(sp)-static_cast<int64_t>(tsymsread)), egptr());

					return sp;
				}

				return -1;
			}

			::std::streampos seekoff(::std::streamoff off, ::std::ios_base::seekdir way, ::std::ios_base::openmode which = ::std::ios_base::in | ::std::ios_base::out)
			{
				if ( which & ::std::ios_base::in )
				{
					int64_t abstarget = 0;
					int64_t const cur = symsread - (egptr()-gptr());

					if ( way == ::std::ios_base::cur )
						abstarget = cur + off;
					else if ( way == ::std::ios_base::beg )
						abstarget = off;
					else // if ( way == ::std::ios_base::end )
						abstarget = static_cast<int64_t>(index.uncompressed) + off;

					if ( abstarget - cur == 0 )
					{
						return abstarget;
					}
					else if ( (abstarget - cur) > 0 && (abstarget - cur) <= (egptr()-gptr()) )
					{
						setg(eback(),gptr()+(abstarget-cur),egptr());
						return abstarget;
					}
					else if ( (abstarget - cur) < 0 && (cur-abstarget) <= (gptr()-eback()) )
					{
						setg(eback(),gptr()-(cur-abstarget),egptr());
						return abstarget;
					}
					else
					{
						return seekpos(abstarget,which);
					}
				}

				return -1;
			}

			int_type underflow()
			{
				// if there is still data, then return it
				if ( gptr() < egptr() )
					return static_cast<int_type>(*uptr());

				assert ( gptr() == egptr() );

				// number of bytes left
				uint64_t const symsleft = (index.uncompressed-symsread);

				if ( symsleft == 0 )
					return traits_type::eof();

				//
				uint64_t const toread = std::min(symsleft,razf_block_size);

				zstream.next_out = reinterpret_cast<Bytef *>(outbuffer.begin());
				zstream.avail_out = toread;

				while ( zstream.avail_out )
				{
					if ( ! zstream.avail_in )
					{
						zstream.next_in = reinterpret_cast<Bytef *>(inbuffer.begin());

						stream.read(inbuffer.begin(),inbuffer.size());

						if ( ! stream.gcount() )
						{
							::libmaus2::exception::LibMausException se;
							se.getStream() << "RAZFDecoderBuffer::underflow() input failure" << std::endl;
							se.finish();
							throw se;
						}

						zstream.avail_in = stream.gcount();
					}

					int const r = inflate(&zstream,Z_NO_FLUSH);

					switch ( r )
					{
						case Z_STREAM_ERROR:
						case Z_NEED_DICT:
						case Z_DATA_ERROR:
						case Z_MEM_ERROR:
						{
							::libmaus2::exception::LibMausException se;
							se.getStream() << "RAZFDecoderBuffer::underflow() inflate failure" << std::endl;
							se.finish();
							throw se;
							break;
						}
					}
				}

				setg(outbuffer.begin(),outbuffer.begin(),outbuffer.begin()+toread);

				symsread += toread;

				return static_cast<int_type>(*uptr());
			}
		};
	}
}
#endif
