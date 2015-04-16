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
#include <libmaus2/types/types.hpp>
#include <libmaus2/lz/SnappyCompress.hpp>

#include <libmaus2/lz/IstreamSource.hpp>
#include <libmaus2/lz/StdOstreamSink.hpp>
#include <libmaus2/aio/IStreamWrapper.hpp>
#if ! defined(LIBMAUS2_HAVE_SNAPPY)
#include <libmaus2/util/GetFileSize.hpp>
#endif

#if defined(LIBMAUS2_HAVE_SNAPPY)
uint64_t libmaus2::lz::SnappyCompress::compress(::libmaus2::lz::IstreamSource< ::libmaus2::aio::IStreamWrapper> & in, std::ostream & out)
{
	::libmaus2::lz::StdOstreamSink<std::ostream> sostr(out);
	::snappy::Compress(&in,&sostr);
	return sostr.written;
}
uint64_t libmaus2::lz::SnappyCompress::compress(std::istream & in, uint64_t const n, std::ostream & out)
{
	::libmaus2::aio::IStreamWrapper ISW(in);
	::libmaus2::lz::IstreamSource< ::libmaus2::aio::IStreamWrapper> sistr(ISW,n);
	::libmaus2::lz::StdOstreamSink<std::ostream> sostr(out);
	::snappy::Compress(&sistr,&sostr);
	return sostr.written;
}
uint64_t libmaus2::lz::SnappyCompress::compress(char const * in, uint64_t const n, std::ostream & out)
{
	::snappy::ByteArraySource BAS(in,n);
	::libmaus2::lz::StdOstreamSink<std::ostream> sostr(out);
	::snappy::Compress(&BAS,&sostr);
	return sostr.written;
}
void libmaus2::lz::SnappyCompress::uncompress(char const * in, uint64_t const insize, std::string & out)
{
	bool const ok = ::snappy::Uncompress(in, insize, &out);
	
	if ( ! ok )
	{
		::libmaus2::exception::LibMausException se;
		se.getStream() << "Failed to decompress snappy data in ::libmaus2::lz::SnappyCompress::uncompress(char *, uint64_t, std::string &)" << std::endl;
		se.finish();
		throw se;
	}
}
void libmaus2::lz::SnappyCompress::uncompress(std::istream & in, uint64_t const insize, char * out)
{
	::libmaus2::aio::IStreamWrapper ISW(in);
	::libmaus2::lz::IstreamSource< ::libmaus2::aio::IStreamWrapper> sistr(ISW,insize);

	bool const ok = ::snappy::RawUncompress(&sistr,out);
	
	if ( ! ok )
	{
		::libmaus2::exception::LibMausException se;
		se.getStream() << "Failed to decompress snappy data in ::libmaus2::lz::SnappyCompress::uncompress(std::istream &, uint64_t, char *)" << std::endl;
		se.finish();
		throw se;	
	}
}
void libmaus2::lz::SnappyCompress::uncompress(
	::libmaus2::lz::IstreamSource< ::libmaus2::aio::IStreamWrapper> & in, 
	char * out,
	int64_t const /* length */
)
{
	bool const ok = ::snappy::RawUncompress(&in,out);
	
	if ( ! ok )
	{
		::libmaus2::exception::LibMausException se;
		se.getStream() << "Failed to decompress snappy data in ::libmaus2::lz::SnappyCompress::uncompress(::libmaus2::lz::IstreamSource< ::libmaus2::aio::IStreamWrapper> & in, char *, int64_t)" << std::endl;
		se.finish();
		throw se;	
	}
}
bool ::libmaus2::lz::SnappyCompress::rawuncompress(char const * compressed, uint64_t compressed_length, char * uncompressed)
{
	return snappy::RawUncompress(compressed,compressed_length,uncompressed);
}
uint64_t libmaus2::lz::SnappyCompress::compressBound(uint64_t length)
{
	return snappy::MaxCompressedLength(length);
}
uint64_t libmaus2::lz::SnappyCompress::rawcompress(char const * uncompressed, uint64_t uncompressed_length, char * compressed)
{
	size_t compressed_length;
	::snappy::RawCompress(uncompressed,uncompressed_length,compressed,&compressed_length);
	return compressed_length;
}
#else
uint64_t libmaus2::lz::SnappyCompress::compress(::libmaus2::lz::IstreamSource< ::libmaus2::aio::IStreamWrapper> & in, std::ostream & out)
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
uint64_t libmaus2::lz::SnappyCompress::compress(std::istream & in, uint64_t const n, std::ostream & out)
{
	::libmaus2::util::GetFileSize::copy(in,out,n);
	return n;
}
uint64_t libmaus2::lz::SnappyCompress::compress(char const * in, uint64_t const n, std::ostream & out)
{
	out.write(in,n);
	return n;
}
void ::libmaus2::lz::SnappyCompress::uncompress(char const * in, uint64_t const insize, std::string & out)
{
	out = std::string(in,in+insize);
}
void ::libmaus2::lz::SnappyCompress::uncompress(std::istream & in, uint64_t const insize, char * out)
{
	in.read(out,insize);

	if ( ! in )
	{
		::libmaus2::exception::LibMausException se;
		se.getStream() << "Failed to read data." << std::endl;
		se.finish();
		throw se;	
	}
}
void ::libmaus2::lz::SnappyCompress::uncompress(
	::libmaus2::lz::IstreamSource< ::libmaus2::aio::IStreamWrapper> & in, 
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

bool ::libmaus2::lz::SnappyCompress::rawuncompress(char const * compressed, uint64_t compressed_length, char * uncompressed)
{
	std::copy(compressed,compressed+compressed_length,uncompressed);
	return true;
}
uint64_t libmaus2::lz::SnappyCompress::compressBound(uint64_t length)
{
	return length;
}
uint64_t libmaus2::lz::SnappyCompress::rawcompress(char const * uncompressed, uint64_t uncompressed_length, char * compressed)
{
	std::copy(uncompressed,uncompressed+uncompressed_length,compressed);
	return uncompressed_length;
}
#endif
