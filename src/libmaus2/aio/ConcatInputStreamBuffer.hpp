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
#if !defined(LIBMAUS2_AIO_CONCATINPUTSTREAMBUFFER_HPP)
#define LIBMAUS2_AIO_CONCATINPUTSTREAMBUFFER_HPP

#include <streambuf>
#include <istream>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/aio/InputStreamInstance.hpp>

namespace libmaus2
{
	namespace aio
	{
		struct ConcatInputStreamBuffer : public ::std::streambuf
		{
			public:
			typedef ConcatInputStreamBuffer this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			private:
			std::vector<std::string> const filenames;
			std::vector<std::string>::const_iterator filenames_ita;
			uint64_t const blocksize;
			uint64_t const putbackspace;
			::libmaus2::autoarray::AutoArray<char> buffer;
			libmaus2::aio::InputStreamInstance::unique_ptr_type Pin;
			uint64_t symsread;
			int64_t filesize;
			bool filesizecomputed;
			libmaus2::autoarray::AutoArray<uint64_t> filesizes;

			ConcatInputStreamBuffer(ConcatInputStreamBuffer const &);
			ConcatInputStreamBuffer & operator=(ConcatInputStreamBuffer &);

			std::streamsize readblock(char * p, std::streamsize s)
			{
				std::streamsize r = 0;

				while ( s )
				{
					// no stream open?
					if ( ! Pin )
					{
						// open next file if we have any
						if ( filenames_ita != filenames.end() )
						{
							libmaus2::aio::InputStreamInstance::unique_ptr_type Tin(new libmaus2::aio::InputStreamInstance(*(filenames_ita++)));
							Pin = UNIQUE_PTR_MOVE(Tin);
						}
						// no more files, set s to zero
						else
						{
							s = 0;
						}
					}
					// if file is open
					if ( Pin )
					{
						Pin->read(p,s);
						std::streamsize const t = Pin->gcount();
						r += t;
						s -= t;
						p += t;
						if ( ! t )
						{
							Pin.reset();
						}
					}
				}

				return r;
			}

			public:
			ConcatInputStreamBuffer(
				std::vector<std::string> const & rfilenames,
				int64_t const rblocksize,
				uint64_t const rputbackspace = 0
			)
			:
			  filenames(rfilenames),
			  filenames_ita(filenames.begin()),
			  blocksize(rblocksize),
			  putbackspace(rputbackspace),
			  buffer(putbackspace + blocksize,false),
			  Pin(),
			  symsread(0),
			  filesize(-1),
			  filesizecomputed(false),
			  filesizes(filenames.size()+1)
			{
				setg(buffer.end(),buffer.end(),buffer.end());
			}

			private:
			// gptr as unsigned pointer
			uint8_t const * uptr() const
			{
				return reinterpret_cast<uint8_t const *>(gptr());
			}

			int64_t getFileSize()
			{
				if ( !filesizecomputed )
				{
					bool ok = true;

					uint64_t fs = 0;
					for ( uint64_t i = 0; i < filenames.size(); ++i )
					{
						InputStreamInstance I(filenames[i]);
						I.seekg(0,std::ios::end);
						std::streampos const p = I.tellg();

						if ( p >= 0 )
						{
							fs += p;
							filesizes[i] = p;
						}
						else
							ok = false;
					}

					filesizes[filenames.size()] = 0;

					if ( ok )
					{
						filesize = fs;
						filesizes.prefixSums();
					}

					filesizecomputed = true;
				}

				return filesize;
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
					uint64_t const tsymsread = (sp / blocksize)*blocksize;

					// set symsread
					symsread = tsymsread;

					// get total file size
					int64_t const fs = getFileSize();
					if ( fs < 0 )
						return -1;

					// seeking beyond end of file
					if ( static_cast<int64_t>(symsread) >= fs )
					{
						filenames_ita = filenames.end();
						Pin.reset();
						setg(buffer.end(),buffer.end(),buffer.end());
						symsread = fs;
					}
					else
					{
						assert ( fs >= 0 );
						assert ( static_cast<int64_t>(symsread) < fs );

						// figure out in which file offset symsread is
						uint64_t const * p = std::lower_bound(filesizes.begin(),filesizes.end(),symsread);

						assert ( p >= filesizes.begin() );
						assert ( p < filesizes.end() );

						if ( *p != symsread )
							--p;

						assert ( p >= filesizes.begin() );
						assert ( p < filesizes.end() );
						assert ( symsread >= *p );

						filenames_ita = filenames.begin() + (p-filesizes.begin());
						uint64_t const off = symsread - (*p);

						libmaus2::aio::InputStreamInstance::unique_ptr_type Tin(new libmaus2::aio::InputStreamInstance(*(filenames_ita++)));
						Pin = UNIQUE_PTR_MOVE(Tin);
						Pin->seekg(off);

						// reset buffer
						setg(buffer.end(),buffer.end(),buffer.end());

						// read next block
						underflow();

						// skip bytes in block to get to final position
						setg(
							eback(),
							gptr() + (static_cast<int64_t>(sp)-static_cast<int64_t>(tsymsread)),
							egptr()
						);

						if ( sp <= fs )
							assert ( sp == static_cast<std::streampos>(symsread - (egptr()-gptr())) );

						return sp;
					}
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
					{
						int64_t const fs = getFileSize();

						if ( fs < 0 )
							return -1;

						abstarget = fs + off;
					}

					// no movement
					if ( abstarget - cur == 0 )
					{
						return abstarget;
					}
					// move right but within buffer
					else if ( (abstarget - cur) > 0 && (abstarget - cur) <= (egptr()-gptr()) )
					{
						setg(
							eback(),
							gptr()+(abstarget-cur),
							egptr()
						);
						return abstarget;
					}
					// move left within buffer
					else if ( (abstarget - cur) < 0 && (cur-abstarget) <= (gptr()-eback()) )
					{
						setg(
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
				uint64_t const uncompressedsize = readblock(buffer.begin()+putbackspace,buffer.size()-putbackspace);

				// set buffer pointers
				setg(
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
