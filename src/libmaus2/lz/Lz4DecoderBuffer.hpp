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
#if !defined(LIBMAUS2_LZ_LZ4DECODERBUFFER_HPP)
#define LIBMAUS2_LZ_LZ4DECODERBUFFER_HPP

#include <streambuf>
#include <istream>
#include <libmaus2/aio/InputStreamInstance.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/lz/Lz4Index.hpp>
#include <libmaus2/lz/Lz4Base.hpp>
#include <libmaus2/util/utf8.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct Lz4DecoderBuffer : public ::std::streambuf
		{
			private:
			static uint64_t const headersize = 1*sizeof(uint64_t);

			::libmaus2::aio::InputStreamInstance::unique_ptr_type Pfilestream;
			std::istream & stream;
			Lz4Index index;

			::libmaus2::autoarray::AutoArray<char> buffer;
			::libmaus2::autoarray::AutoArray<char> cbuffer;

			uint64_t symsread;

			Lz4DecoderBuffer(Lz4DecoderBuffer const &);
			Lz4DecoderBuffer & operator=(Lz4DecoderBuffer &);

			void init()
			{
				setg(buffer.end(), buffer.end(), buffer.end());

				// seek to start of first block
				if ( index.payloadbytes )
				{
					uint64_t const blockpos = index[0];
					stream.clear();
					stream.seekg(blockpos,std::ios::beg);
				}
			}

			static int decompressBlock(
				char const * input, char * output,
				uint64_t const inputsize,
				uint64_t const maxoutputsize
                        );

			public:
			Lz4DecoderBuffer(std::string const & filename)
			: Pfilestream(new ::libmaus2::aio::InputStreamInstance(filename)),
			  stream(*Pfilestream),
			  index(stream),
			  buffer(index.blocksize,false),
			  cbuffer(libmaus2::lz::Lz4Base::getCompressBound(index.blocksize),false),
			  symsread(0)
			{
				init();
			}

			Lz4DecoderBuffer(std::istream & rstream)
			: Pfilestream(),
			  stream(rstream),
			  index(stream),
			  buffer(index.blocksize,false),
			  cbuffer(libmaus2::lz::Lz4Base::getCompressBound(index.blocksize),false),
			  symsread(0)
			{
				init();
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
					uint64_t tsymsread = (sp / index.blocksize)*index.blocksize;

					// block position
					uint64_t const blockpos = index[tsymsread/index.blocksize];

					symsread = tsymsread;
					stream.clear();
					// seek to block
					stream.seekg(blockpos);
					setg(buffer.end(),buffer.end(),buffer.end());
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
						abstarget = static_cast<int64_t>(index.payloadbytes) + off;

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
				uint64_t const symsleft = (index.payloadbytes-symsread);

				if ( symsleft == 0 )
					return traits_type::eof();

				uint64_t compressedsize = libmaus2::util::UTF8::decodeUTF8(stream);
				uint64_t uncompressedsize = libmaus2::util::UTF8::decodeUTF8(stream);

				assert ( compressedsize <= cbuffer.size() );

				// load packed data into memory
				stream.read(cbuffer.begin(),compressedsize);
				if ( (!stream) || (stream.gcount() != static_cast<int64_t>(compressedsize)) )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "Lz4DecoderBuffer::underflow() failed to read block of " << compressedsize << " compressed bytes." << std::endl;
					se.finish();
					throw se;
				}

				int const decompsize = decompressBlock(cbuffer.begin(),buffer.begin(),compressedsize,buffer.size());

				assert ( decompsize == static_cast<int>(uncompressedsize) );

				setg(buffer.begin(),buffer.begin(),buffer.begin()+uncompressedsize);

				symsread += uncompressedsize;

				return static_cast<int_type>(*uptr());
			}
		};
	}
}
#endif
