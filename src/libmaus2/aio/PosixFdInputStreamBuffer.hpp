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
#if !defined(LIBMAUS2_AIO_POSIXFDINPUTSTREAMBUFFER_HPP)
#define LIBMAUS2_AIO_POSIXFDINPUTSTREAMBUFFER_HPP

#include <streambuf>
#include <istream>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/aio/PosixFdInput.hpp>

namespace libmaus2
{
	namespace aio
	{
		struct PosixFdInputStreamBuffer : public ::std::streambuf
		{
			public:
			typedef PosixFdInputStreamBuffer this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			private:
			static int64_t getDefaultBlockSize()
			{
				return 64*1024;
			}

			::libmaus2::aio::PosixFdInput & stream;
			int64_t const optblocksize;

			int64_t const filesize;

			uint64_t const blocksize;
			uint64_t const putbackspace;

			::libmaus2::autoarray::AutoArray<char> buffer;

			uint64_t symsread;

			PosixFdInputStreamBuffer(PosixFdInputStreamBuffer const &);
			PosixFdInputStreamBuffer & operator=(PosixFdInputStreamBuffer &);

			inline void setgchecked(char * a, char * b, char * c)
			{
				bool const ok = (a <= b) && (b <= c);

				if ( ! ok )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "PosixFdInputStreamBuffer: invalid parameters for setg detected, a="
						<< (void *)a << " b=" << (void *)b << " c=" << (void *)c << std::endl;
					lme.finish();
					throw lme;
				}

				setg(a,b,c);
			}

			void init(bool const repos)
			{
				// set empty buffer
				setgchecked(buffer.end(), buffer.end(), buffer.end());
				// seek
				if ( repos )
					stream.lseek(symsread);
			}

			uint64_t getOptimalBlockSize()
			{
				int64_t const optblocksize = stream.getOptimalIOBlockSize();

				if ( optblocksize <= 0 )
					return getDefaultBlockSize();
				else
					return optblocksize;
			}

			public:
			PosixFdInputStreamBuffer(
				::libmaus2::aio::PosixFdInput & rstream,
				int64_t const rblocksize,
				uint64_t const rputbackspace = 0
			)
			:
			  stream(rstream),
			  optblocksize(getOptimalBlockSize()),
			  filesize(stream.sizeChecked()),
			  blocksize((rblocksize < 0) ? optblocksize : rblocksize),
			  putbackspace(rputbackspace),
			  buffer(putbackspace + blocksize,false),
			  symsread(0)
			{
				init(false);
			}

			private:
			// gptr as unsigned pointer
			uint8_t const * uptr() const
			{
				return reinterpret_cast<uint8_t const *>(gptr());
			}

			/**
			 * seek to absolute position
			 **/
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
					uint64_t tsymsread = (sp / blocksize)*blocksize;

					// set symsread
					symsread = tsymsread;

					// reinit
					init(true);

					// read next block
					underflow();

					// skip bytes in block to get to final position
					setgchecked(
						eback(),
						gptr() + (static_cast<int64_t>(sp)-static_cast<int64_t>(tsymsread)),
						egptr()
					);

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
						abstarget = static_cast<int64_t>(filesize) + off;

					// no movement
					if ( abstarget - cur == 0 )
					{
						return abstarget;
					}
					// move right but within buffer
					else if ( (abstarget - cur) > 0 && (abstarget - cur) <= (egptr()-gptr()) )
					{
						setgchecked(
							eback(),
							gptr()+(abstarget-cur),
							egptr()
						);
						return abstarget;
					}
					// move left within buffer
					else if ( (abstarget - cur) < 0 && (cur-abstarget) <= (gptr()-eback()) )
					{
						setgchecked(
							eback(),
							gptr()-(cur-abstarget),
							egptr()
						);
						return abstarget;
					}
					// move target is outside currently buffered region
					// -> use absolute move
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

				// number of bytes for putback buffer
				uint64_t const putbackcopy = std::min(
					static_cast<uint64_t>(gptr() - eback()),
					putbackspace
				);
				// copy bytes
				std::copy(
					gptr()-putbackcopy,
					gptr(),
					buffer.begin() + putbackspace - putbackcopy
				);

				// load data
				uint64_t const uncompressedsize = stream.read(
						buffer.begin()+putbackspace,
						buffer.size()-putbackspace
					);

				// set buffer pointers
				setgchecked(
					buffer.begin()+putbackspace-putbackcopy,
					buffer.begin()+putbackspace,
					buffer.begin()+putbackspace+uncompressedsize);

				symsread += uncompressedsize;

				if ( uncompressedsize )
					return static_cast<int_type>(*uptr());
				else
					return traits_type::eof();
			}
		};
	}
}
#endif
