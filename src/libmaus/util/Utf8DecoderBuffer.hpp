/**
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
**/

#if ! defined(LIBMAUS_UTIL_UTF8DECODERBUFFER_HPP)
#define LIBMAUS_UTIL_UTF8DECODERBUFFER_HPP

#include <istream>
#include <fstream>
#include <ios>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/util/GetFileSize.hpp>
#include <libmaus/aio/CheckedInputStream.hpp>
#include <libmaus/util/NumberSerialisation.hpp>
#include <libmaus/bitio/Ctz.hpp>
#include <libmaus/bitio/CompactArray.hpp>
#include <libmaus/bitio/ArrayDecode.hpp>
#include <libmaus/util/Utf8BlockIndex.hpp>

namespace libmaus
{
	namespace util
	{
		struct Utf8DecoderBuffer : public ::std::wstreambuf
		{
			protected:
			typedef ::std::wstreambuf::char_type char_type;
			
			static uint64_t const headersize = 4*sizeof(uint64_t);
			
			::libmaus::util::Utf8BlockIndexDecoder indexdecoder;
			uint64_t const blocksize;
                        uint64_t const lastblocksize;
                        uint64_t const maxblockbytes;
                        uint64_t const numblocks;
                        
			::libmaus::aio::CheckedInputStream stream;
			uint64_t const n;
			uint64_t const buffersize;

			::libmaus::autoarray::AutoArray<uint8_t> inbuffer;
			::libmaus::autoarray::AutoArray<wchar_t> buffer;
			
			uint64_t symsread;

			Utf8DecoderBuffer(Utf8DecoderBuffer const &);
			Utf8DecoderBuffer & operator=(Utf8DecoderBuffer&);
			
			public:
			Utf8DecoderBuffer(
				std::string const & filename, 
				::std::size_t rbuffersize
			)
			: 
			  indexdecoder(filename+".idx"),
			  blocksize(indexdecoder.blocksize),
			  lastblocksize(indexdecoder.lastblocksize),
			  maxblockbytes(indexdecoder.maxblockbytes),
			  numblocks(indexdecoder.numblocks),
			  stream(filename),
			  n( (numblocks-1)*blocksize + lastblocksize  ),
			  // make buffersize multiple of blocksize
			  buffersize(((rbuffersize + blocksize-1)/blocksize)*blocksize),
			  inbuffer(),
			  // inbuffer(maxblockbytes,false),
			  buffer(buffersize,false),
			  symsread(0)
			{
				setg(buffer.end(), buffer.end(), buffer.end());	
			}

			private:
			::std::streampos seekpos(::std::streampos sp, ::std::ios_base::openmode which = ::std::ios_base::in | ::std::ios_base::out)
			{
				if ( which & ::std::ios_base::in )
				{
					int64_t const cur = symsread-(egptr()-gptr());
					int64_t const curlow = cur - static_cast<int64_t>(gptr()-eback());
					int64_t const curhigh = cur + static_cast<int64_t>(egptr()-gptr());
					
					// call relative seek, if target is in range
					if ( sp >= curlow && sp <= curhigh )
						return seekoff(sp - cur, ::std::ios_base::cur, which);

					// target is out of range, we really need to seek
					uint64_t tsymsread = (sp / buffersize)*buffersize;
					
					symsread = tsymsread;
					stream.clear();
					assert ( tsymsread % blocksize == 0 );
					stream.seekg( indexdecoder[tsymsread / blocksize] );
					setg(buffer.end(),buffer.end(),buffer.end());
					underflow();
					setg(eback(),gptr() + (sp-static_cast<int64_t>(tsymsread)), egptr());
				
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
						abstarget = n + off;
						
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
					return static_cast<int_type>(*gptr());

				assert ( gptr() == egptr() );
				
				// we should be on a block size boundary or at the end
				assert ( symsread == n || symsread % blocksize == 0 );

				uint64_t const symsleft = (n-symsread);
				
				if ( symsleft == 0 )
					return traits_type::eof();
				
				// number of symbols we want to read
				uint64_t const symstoread = std::min(symsleft,buffersize);
				
				// block start in symbols
				uint64_t const insyms = symsread;
				uint64_t outsyms = symsread + symstoread;

				// round up to next block
				if ( outsyms % blocksize )
					outsyms += (blocksize - (outsyms % blocksize));
					
				assert ( outsyms % blocksize == 0 );
				
				// number of bytes we want to read from coded stream
				uint64_t const bytestoread = indexdecoder[outsyms/blocksize] - indexdecoder[insyms/blocksize];
				
				// load packed data into memory
				if ( inbuffer.size() < bytestoread )
					inbuffer = ::libmaus::autoarray::AutoArray<uint8_t>(bytestoread);
				stream.read ( reinterpret_cast<char *>(inbuffer.begin()) , bytestoread );
				if ( stream.gcount() != static_cast<int64_t>(bytestoread) )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Utf8DecoderBuffer::underflow() failed to read " << bytestoread << " bytes." << std::endl;
					se.finish();
					throw se;
				}
				
				// decode utf-8 coded data into byte buffer
				::libmaus::util::GetObject<uint8_t const *> G(inbuffer.begin());
				for ( uint64_t i = 0; i < symstoread; ++i )
					buffer[i] = ::libmaus::util::UTF8::decodeUTF8(G);

				setg(buffer.begin(),buffer.begin(),buffer.begin()+symstoread);

				symsread += symstoread;
				
				return static_cast<int_type>(*gptr());
			}
		};


		struct Utf8DecoderWrapper : public Utf8DecoderBuffer, public ::std::wistream
		{
			typedef Utf8DecoderWrapper this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			typedef Utf8DecoderBuffer::char_type char_type;
		
			Utf8DecoderWrapper(std::string const & filename, uint64_t const buffersize = 64*1024)
			: Utf8DecoderBuffer(filename,buffersize), ::std::wistream(this)
			{
				
			}

			static int getSymbolAtPosition(std::string const & filename, uint64_t const offset)
			{
				this_type I(filename);
				I.seekg(offset);
				return I.get();
			}

			static uint64_t getFileSize(std::string const & filename)
			{
				this_type I(filename);
				I.seekg(0,std::ios::end);
				return I.tellg();
			}
		};
	}
}
#endif
