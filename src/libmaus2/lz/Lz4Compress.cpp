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
#include <libmaus/lz/Lz4Compress.hpp>
#include <libmaus/lz/Lz4Base.hpp>
#include <libmaus/util/utf8.hpp>
#include <libmaus/lz/lz4.h>

libmaus::lz::Lz4Compress::Lz4Compress(std::ostream & rout, uint64_t const rinputblocksize, std::ostream * rindexstream)
: inputblocksize(rinputblocksize), outputblocksize(libmaus::lz::Lz4Base::getCompressBound(inputblocksize)), outputblock(outputblocksize,false), out(rout), outputbyteswritten(0), payloadbyteswritten(0), indexstream(rindexstream)
{

}

libmaus::lz::Lz4Compress::~Lz4Compress() {}

void libmaus::lz::Lz4Compress::writeUncompressed(char const * input, int const inputsize)
{
	out.write(input,inputsize);

	if ( ! out )
	{
		libmaus::exception::LibMausException se;
		se.getStream() << "libmaus::lz::Lz4Compress::write(): failed to write to output stream"  << std::endl;
		se.finish();
		throw se;
	}
	
	outputbyteswritten += inputsize;
}

void libmaus::lz::Lz4Compress::write(char const * input, int const inputsize)
{
	if ( indexstream )
	{
		if ( (payloadbyteswritten % inputblocksize) != 0 )
		{
			libmaus::exception::LibMausException se;
			se.getStream() << "libmaus::lz::Lz4Compress::write(): block index out of sync"  << std::endl;
			se.finish();
			throw se;		
		}
	
		libmaus::util::NumberSerialisation::serialiseNumber(*indexstream,outputbyteswritten);

		if ( ! (*indexstream) )
		{
			libmaus::exception::LibMausException se;
			se.getStream() << "libmaus::lz::Lz4Compress::write(): failed to write to index stream"  << std::endl;
			se.finish();
			throw se;
		}
	}

	int const compressedSize = LZ4_compress(input,outputblock.begin(),inputsize);

	std::ostringstream ostr;
	libmaus::util::UTF8::encodeUTF8(compressedSize,ostr);
	libmaus::util::UTF8::encodeUTF8(inputsize,ostr);

	writeUncompressed(ostr.str().c_str(),ostr.str().size());
	writeUncompressed(outputblock.begin(),compressedSize);	
	
	payloadbyteswritten += inputsize;
}

void libmaus::lz::Lz4Compress::flush()
{
	out.flush();

	if ( ! out )
	{
		libmaus::exception::LibMausException se;
		se.getStream() << "libmaus::lz::Lz4Compress::flush(): failed to write to output stream"  << std::endl;
		se.finish();
		throw se;
	}

	if ( indexstream )
	{
		indexstream->flush();

		if ( ! (*indexstream) )
		{
			libmaus::exception::LibMausException se;
			se.getStream() << "libmaus::lz::Lz4Compress::write(): failed to write to index stream"  << std::endl;
			se.finish();
			throw se;
		}
	}
}

uint64_t libmaus::lz::Lz4Compress::align(uint64_t const mod)
{
	char zbuf[] = { 0,0,0,0, 0,0,0,0 };
	
	while ( outputbyteswritten % mod )
	{
		uint64_t const towrite = std::min ( static_cast<uint64_t>(sizeof(zbuf)/sizeof(zbuf[0])), mod - (outputbyteswritten % mod));
		writeUncompressed(&zbuf[0],towrite);
	}
	
	return outputbyteswritten;
}

uint64_t libmaus::lz::Lz4Compress::getPayloadBytesWritten() const
{
	return payloadbyteswritten;
}
