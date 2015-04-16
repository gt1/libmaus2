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

#if ! defined(BUFFERITERATOR_HPP)
#define BUFFERITERATOR_HPP

#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/bitio/OutputBuffer.hpp>
#include <libmaus2/bitio/FastWriteBitWriter.hpp>

namespace libmaus2
{
	namespace bitio
	{
		template<typename _data_type>
		struct BufferIterator
		{
			typedef _data_type data_type;
			
			typedef BufferIterator<data_type> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			
			::libmaus2::bitio::OutputFile<data_type> outputfile;
			::libmaus2::bitio::OutputBufferIterator<data_type> outputiterator;
			::libmaus2::bitio::FastWriteBitWriterBuffer64 writer;
			uint64_t bits;
			
			BufferIterator(std::string const & filename, uint64_t const bufsize)
			: outputfile(bufsize,filename), outputiterator(outputfile), writer(outputiterator), bits(0)
			{
			
			}
			
			void writeBit(uint64_t const bit)
			{
				writer.writeBit(bit);
				bits++;
			}
			
			void flush()
			{
				writer.flush();
				outputfile.flush();
			}
		};
		typedef BufferIterator<uint64_t> BufferIterator8;
	}
}
#endif
