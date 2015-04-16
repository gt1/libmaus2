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

#if ! defined(HUFFMANENCODERFILE_HPP)
#define HUFFMANENCODERFILE_HPP

#include <libmaus2/bitio/FastWriteBitWriter.hpp>
#include <libmaus2/aio/SynchronousGenericOutput.hpp>

namespace libmaus2
{
	namespace huffman
	{
		template<typename _output_type, typename _bitwriter_type>
		struct HuffmanEncoderFileTemplate :
			public _output_type,
			public _output_type::iterator_type,
			public _bitwriter_type
		{
			typedef _output_type output_type;
			typedef _bitwriter_type bitwriter_type;
			typedef HuffmanEncoderFileTemplate<output_type,bitwriter_type> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			
			typedef typename output_type::iterator_type iterator_type;
		
			HuffmanEncoderFileTemplate(std::string const & newmergedfilenamerl, uint64_t const bufsize = 64*1024)
			:
				output_type(newmergedfilenamerl,bufsize,true/*truncate*/,0/*offset*/,true/*metasync*/),
				output_type::iterator_type(static_cast< output_type & >(*this)),
				bitwriter_type(static_cast< typename output_type::iterator_type & > (*this) )
			{}
			~HuffmanEncoderFileTemplate()
			{
				flush();
			}
			void flush()
			{
				flushBitStream();
				output_type::flush();
			}
			void flushBitStream()
			{
				bitwriter_type::flush();			
			}
			
			void write(uint64_t const word, unsigned int const bits)
			{
				bitwriter_type::write(word,bits);
			}
			
			uint64_t getPos() const
			{
				return output_type::getWrittenBytes();
			}
			uint64_t tellp() const
			{
				return getPos();
			}

			iterator_type & getIterator()
			{
				return static_cast<iterator_type &>(*this);
			}
		};
		
		typedef HuffmanEncoderFileTemplate< ::libmaus2::aio::SynchronousGenericOutputPosix<uint8_t>, ::libmaus2::bitio::FastWriteBitWriterStream8Posix > HuffmanEncoderFile;
		typedef HuffmanEncoderFileTemplate< ::libmaus2::aio::SynchronousGenericOutput     <uint8_t>, ::libmaus2::bitio::FastWriteBitWriterStream8Std > HuffmanEncoderFileStd;
	}
}
#endif
