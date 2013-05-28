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
#include <libmaus/types/types.hpp>
#include <libmaus/lz/SnappyCompress.hpp>

#include <libmaus/lz/IstreamSource.hpp>
#include <libmaus/lz/StdOstreamSink.hpp>
#include <libmaus/aio/IStreamWrapper.hpp>
#if ! defined(LIBMAUS_HAVE_SNAPPY)
#include <libmaus/util/GetFileSize.hpp>
#endif

#if defined(LIBMAUS_HAVE_SNAPPY)
uint64_t libmaus::lz::SnappyCompress::compress(::libmaus::lz::IstreamSource< ::libmaus::aio::IStreamWrapper> & in, std::ostream & out)
{
	::libmaus::lz::StdOstreamSink<std::ostream> sostr(out);
	::snappy::Compress(&in,&sostr);
	return sostr.written;
}
uint64_t libmaus::lz::SnappyCompress::compress(std::istream & in, uint64_t const n, std::ostream & out)
{
	::libmaus::aio::IStreamWrapper ISW(in);
	::libmaus::lz::IstreamSource< ::libmaus::aio::IStreamWrapper> sistr(ISW,n);
	::libmaus::lz::StdOstreamSink<std::ostream> sostr(out);
	::snappy::Compress(&sistr,&sostr);
	return sostr.written;
}
uint64_t libmaus::lz::SnappyCompress::compress(char const * in, uint64_t const n, std::ostream & out)
{
	::snappy::ByteArraySource BAS(in,n);
	::libmaus::lz::StdOstreamSink<std::ostream> sostr(out);
	::snappy::Compress(&BAS,&sostr);
	return sostr.written;
}
void libmaus::lz::SnappyCompress::uncompress(char const * in, uint64_t const insize, std::string & out)
{
	bool const ok = ::snappy::Uncompress(in, insize, &out);
	
	if ( ! ok )
	{
		::libmaus::exception::LibMausException se;
		se.getStream() << "Failed to decompress snappy data in ::libmaus::lz::SnappyCompress::uncompress(char *, uint64_t, std::string &)" << std::endl;
		se.finish();
		throw se;
	}
}
void libmaus::lz::SnappyCompress::uncompress(std::istream & in, uint64_t const insize, char * out)
{
	::libmaus::aio::IStreamWrapper ISW(in);
	::libmaus::lz::IstreamSource< ::libmaus::aio::IStreamWrapper> sistr(ISW,insize);

	bool const ok = ::snappy::RawUncompress(&sistr,out);
	
	if ( ! ok )
	{
		::libmaus::exception::LibMausException se;
		se.getStream() << "Failed to decompress snappy data in ::libmaus::lz::SnappyCompress::uncompress(std::istream &, uint64_t, char *)" << std::endl;
		se.finish();
		throw se;	
	}
}
void libmaus::lz::SnappyCompress::uncompress(
	::libmaus::lz::IstreamSource< ::libmaus::aio::IStreamWrapper> & in, 
	char * out,
	int64_t const /* length */
)
{
	bool const ok = ::snappy::RawUncompress(&in,out);
	
	if ( ! ok )
	{
		::libmaus::exception::LibMausException se;
		se.getStream() << "Failed to decompress snappy data in ::libmaus::lz::SnappyCompress::uncompress(::libmaus::lz::IstreamSource< ::libmaus::aio::IStreamWrapper> & in, char *, int64_t)" << std::endl;
		se.finish();
		throw se;	
	}
}
#else
uint64_t libmaus::lz::SnappyCompress::compress(::libmaus::lz::IstreamSource< ::libmaus::aio::IStreamWrapper> & in, std::ostream & out)
{
	uint64_t const n = in.Available();
	uint64_t r = n;
	
	while ( r )
	{
		size_t av = 0;
		char const * B = in.Peek(&av);
		uint64_t const tocopy = std::min(static_cast<uint64_t>(av),r);
		
		out.write(B,tocopy);
		in.Skip(tocopy);
		
		r -= tocopy;
	}
	
	return n;
}
uint64_t libmaus::lz::SnappyCompress::compress(std::istream & in, uint64_t const n, std::ostream & out)
{
	::libmaus::util::GetFileSize::copy(in,out,n);
	return n;
}
uint64_t libmaus::lz::SnappyCompress::compress(char const * in, uint64_t const n, std::ostream & out)
{
	out.write(in,n);
	return n;
}
void ::libmaus::lz::SnappyCompress::uncompress(char const * in, uint64_t const insize, std::string & out)
{
	out = std::string(in,in+insize);
}
void ::libmaus::lz::SnappyCompress::uncompress(std::istream & in, uint64_t const insize, char * out)
{
	in.read(out,insize);

	if ( ! in )
	{
		::libmaus::exception::LibMausException se;
		se.getStream() << "Failed to read data." << std::endl;
		se.finish();
		throw se;	
	}
}
void ::libmaus::lz::SnappyCompress::uncompress(
	::libmaus::lz::IstreamSource< ::libmaus::aio::IStreamWrapper> & in, 
	char * out,
	int64_t const length
)
{
	assert ( length >= 0 );
	uint64_t r = length;
	
	while ( r )
	{
		size_t av = 0;
		char const * B = in.Peek(&av);
		uint64_t const tocopy = std::min(static_cast<uint64_t>(av),r);
		
		std::copy(B,B+tocopy,out);
		in.Skip(tocopy);

		out += tocopy;		
		r -= tocopy;
	}	
}
#endif
