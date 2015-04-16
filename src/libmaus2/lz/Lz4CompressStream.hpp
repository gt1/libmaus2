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
#if !defined(LIBMAUS_LZ_LZ4COMPRESSSTREAM_HPP)
#define LIBMAUS_LZ_LZ4COMPRESSSTREAM_HPP

#include <libmaus/lz/Lz4CompressStreamBuffer.hpp>
#include <ostream>
#include <sstream>
#include <libmaus/util/unique_ptr.hpp>
#include <libmaus/util/shared_ptr.hpp>
#include <libmaus/util/utf8.hpp>
#include <libmaus/util/NumberSerialisation.hpp>

namespace libmaus
{
	namespace lz
	{
		struct Lz4CompressStream : public Lz4CompressStreamBuffer, public std::ostream
		{	
			typedef Lz4CompressStream this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			libmaus::util::unique_ptr<std::ostringstream>::type memoryindexstream;

			std::ostream * indexstream;
		
			Lz4CompressStream(std::ostream & out, uint64_t const buffersize)
			: Lz4CompressStreamBuffer(out,buffersize), std::ostream(this), memoryindexstream(new std::ostringstream), indexstream(memoryindexstream.get())
			{
				Lz4CompressStreamBuffer::wrapped.setIndexStream(indexstream);
			
				std::ostringstream ostr;
				// write buffer size
				libmaus::util::UTF8::encodeUTF8(buffersize,ostr);

				Lz4CompressStreamBuffer::Lz4CompressWrapper::wrapped.writeUncompressed(ostr.str().c_str(),ostr.str().size());				
			}
			
			void writeIndex()
			{
				flush();
				
				if ( memoryindexstream )
				{
					uint64_t const numoffsets = memoryindexstream->str().size() / sizeof(uint64_t);
					
					// start of index byte offset in output stream
					uint64_t const indexoffset = Lz4CompressStreamBuffer::Lz4CompressWrapper::wrapped.align(8);
					
					// write number of payload bytes and number of entries in block pointer index
					std::ostringstream numoffsetsostr;
					libmaus::util::NumberSerialisation::serialiseNumber(numoffsetsostr,Lz4CompressStreamBuffer::Lz4CompressWrapper::wrapped.getPayloadBytesWritten());
					libmaus::util::NumberSerialisation::serialiseNumber(numoffsetsostr,numoffsets);
					Lz4CompressStreamBuffer::Lz4CompressWrapper::wrapped.writeUncompressed(numoffsetsostr.str().c_str(),numoffsetsostr.str().size());
					
					// write block pointers
					Lz4CompressStreamBuffer::Lz4CompressWrapper::wrapped.writeUncompressed(memoryindexstream->str().c_str(),memoryindexstream->str().size());
					
					// write pointer to start of index
					std::ostringstream blockindexptrstr;
					libmaus::util::NumberSerialisation::serialiseNumber(blockindexptrstr,indexoffset);
					Lz4CompressStreamBuffer::Lz4CompressWrapper::wrapped.writeUncompressed(blockindexptrstr.str().c_str(),blockindexptrstr.str().size());
				}
			}
		};
	}
}
#endif
