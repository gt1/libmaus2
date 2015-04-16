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
#if ! defined(PLAINORGZIPSTREAMBUFFERWRAPPER_HPP)
#define PLAINORGZIPSTREAMBUFFERWRAPPER_HPP

#include <libmaus/aio/PosixFdInputStream.hpp>
#include <libmaus/lz/BufferedGzipStreamBuffer.hpp>
#include <libmaus/lz/GzipStream.hpp>
#include <libmaus/lz/StreamWrapperBuffer.hpp>
#include <libmaus/lz/GzipHeader.hpp>

namespace libmaus
{
	namespace lz
	{
		struct PlainOrGzipStreamBufferWrapper
		{
			typedef PlainOrGzipStreamBufferWrapper this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			private:
			libmaus::aio::PosixFdInputStream::unique_ptr_type PFIS;

			libmaus::lz::GzipStream::unique_ptr_type PGZ;
			::libmaus::lz::StreamWrapperBuffer< ::libmaus::lz::GzipStream >::unique_ptr_type PSWB;
			
			std::streambuf * strbuf;
			
			void init(std::istream & istr, uint64_t const bufsize, uint64_t const pushbacksize)
			{
				if ( libmaus::lz::GzipHeaderConstantsBase::checkGzipMagic(istr) )
				{
					libmaus::lz::GzipStream::unique_ptr_type TGZ(new libmaus::lz::GzipStream(istr));
					PGZ = UNIQUE_PTR_MOVE(TGZ);
					
					::libmaus::lz::StreamWrapperBuffer< ::libmaus::lz::GzipStream >::unique_ptr_type TSWB(
						new ::libmaus::lz::StreamWrapperBuffer< ::libmaus::lz::GzipStream >(*PGZ,bufsize,pushbacksize)
					);
					PSWB = UNIQUE_PTR_MOVE(TSWB);
					
					strbuf = PSWB.get();
				}
				else
				{
					strbuf = istr.rdbuf();
				}			
			}
			
			public:
			PlainOrGzipStreamBufferWrapper(int const rfd, uint64_t const bufsize = 64*1024, uint64_t const pushbacksize = 64*1024)
			{
				libmaus::aio::PosixFdInputStream::unique_ptr_type TPFIS(new libmaus::aio::PosixFdInputStream(rfd,bufsize,pushbacksize));
				PFIS = UNIQUE_PTR_MOVE(TPFIS);	
				init(*PFIS,bufsize,pushbacksize);
			}
			PlainOrGzipStreamBufferWrapper(std::istream & istr, uint64_t const bufsize = 64*1024, uint64_t const pushbacksize = 64*1024)
			{
				init(istr,bufsize,pushbacksize);
			}
			std::streambuf * getStreamBuffer()
			{
				return strbuf;
			}	
		};
	}
}
#endif
