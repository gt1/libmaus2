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


#if ! defined(BITINPUTBUFFER_HPP)
#define BITINPUTBUFFER_HPP

#if defined(__linux__)
#include <byteswap.h>
#endif

#if defined(__FreeBSD__)
#include <sys/endian.h>
#endif

#include <libmaus2/huffman/FileStreamBaseType.hpp>
#include <libmaus2/math/MetaLog.hpp>
#include <libmaus2/math/lowbits.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>

#include <libmaus2/util/ReverseByteOrder.hpp>

namespace libmaus2
{
	namespace huffman
	{
		template<
			typename _buffer_data_type = uint8_t,
			typename _raw_input_ptr_type = FileStreamBaseType::unique_ptr_type
		>
		struct BitInputBufferTemplate
		{
			typedef _raw_input_ptr_type raw_input_ptr_type;
			typedef _buffer_data_type buffer_data_type;
			typedef BitInputBufferTemplate<buffer_data_type,raw_input_ptr_type> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			typedef typename raw_input_ptr_type::element_type raw_input_type;

			raw_input_ptr_type in;

			enum { buffer_data_type_bits = 8*sizeof(buffer_data_type) };
			enum { buffer_data_type_shift = ::libmaus2::math::MetaLog2< buffer_data_type_bits >::log };

			::libmaus2::autoarray::AutoArray<buffer_data_type> B;
			buffer_data_type * const pa;
			buffer_data_type * const pe;
			buffer_data_type * const pm;
			buffer_data_type * pc;

			uint64_t word;
			unsigned int wordfill;
			uint64_t bitsread;

			BitInputBufferTemplate(raw_input_ptr_type & rin, uint64_t const bufsize)
			:
			  in(UNIQUE_PTR_MOVE(rin)),
			  B(bufsize,false), pa(B.begin()), pe(B.end()), pm(pa+bufsize/2), pc(pe),
			  word(0), wordfill(0), bitsread(0)
			{
				shift();

			}

			void fillWord(unsigned int const thres)
			{
				while ( wordfill < thres )
				{
					word <<= buffer_data_type_bits;
					word |= *(pc++);
					if ( pc > pm )
						shift();
					wordfill += buffer_data_type_bits;
				}
			}

			void flush()
			{
				while ( bitsread & 7 )
					readBit();
				// std::cerr << "Fill after flush is " << wordfill << std::endl;
			}

			bool readBit()
			{
				fillWord(1);
				uint64_t const mask = 1ull << (wordfill-1);
				bool const bit = word & mask;
				word &= (~mask);
				wordfill--;
				bitsread++;
				return bit;
			}


			uint64_t read(unsigned int bits)
			{
				bitsread += bits;

				if ( bits <= wordfill || bits <= buffer_data_type_bits )
				{
					fillWord(bits);
					uint64_t const mask = ::libmaus2::math::lowbits(bits) << (wordfill-bits);
					uint64_t const val = (word & mask) >> (wordfill-bits);
					word &= ~mask;
					wordfill -= bits;
					return val;
				}
				else
				{
					uint64_t val = word << (bits-wordfill);
					bits -= wordfill;
					wordfill = 0;
					word = 0;
					fillWord(bits);
					uint64_t const mask = ::libmaus2::math::lowbits(bits) << (wordfill-bits);
					val |= (word & mask) >> (wordfill-bits);
					word &= ~mask;
					wordfill -= bits;
					return val;
				}
			}

			uint64_t peek(unsigned int const bits)
			{
				fillWord(bits);
				uint64_t const mask = ::libmaus2::math::lowbits(bits) << (wordfill-bits);
				uint64_t const val = (word & mask) >> (wordfill-bits);
				return val;
			}

			void erase(unsigned int const bits)
			{
				assert ( bits <= wordfill );
				uint64_t const mask = ::libmaus2::math::lowbits(bits) << (wordfill-bits);
				word &= ~mask;
				wordfill -= bits;
				bitsread += bits;
			}

			void shift()
			{
				uint64_t const shiftout = pc-pa;

				// shift data in the back to the front
				if ( shiftout )
				{
					buffer_data_type * s = B.begin() + shiftout;
					buffer_data_type * t = B.begin();

					while ( s != B.end() )
						*(t++) = *(s++);
				}

				// load new data at the back
				uint64_t const wordoffset = B.size() - shiftout;
				uint64_t const bytelen = shiftout * sizeof(buffer_data_type);

				in->read ( reinterpret_cast<char *>(B.begin() + wordoffset) , bytelen );

				#if defined(LIBMAUS2_BYTE_ORDER_LITTLE_ENDIAN)
				for ( uint64_t i =  wordoffset; i < B.size(); ++i )
					switch ( sizeof(buffer_data_type) )
					{
						case 8:
							#if defined(_WIN32)
							B [ i ] = _byteswap_uint64(B[i]);
							#elif defined(__FreeBSD__)
							B [ i ] = bswap64( B[i] );
							#elif defined(__linux__)
							B [ i ] = bswap_64( B[i] );
							#else
							B [ i ] = ::libmaus2::util::ReverseByteOrder::reverseByteOrder<uint64_t>(B[i]);
							#endif
							break;
						case 4:
							#if defined(_WIN32)
							B [ i ] =
								_byteswap_ushort( (B[i]>>16) & 0xFFFFUL )
								|
								(static_cast<uint32_t>(_byteswap_ushort( B[i] & 0xFFFFUL )) << 16)
								;
								// _byteswap_ulong(B[i]);
							#elif defined(__FreeBSD__)
							B [ i ] = bswap32( B[i] );
							#elif defined(__linux__)
							B [ i ] = bswap_32( B[i] );
							#else
							B [ i ] = ::libmaus2::util::ReverseByteOrder::reverseByteOrder<uint32_t>(B[i]);
							#endif
							break;
						case 2:
							#if defined(_WIN32)
							B [ i ] = _byteswap_ushort(B[i]);
							#elif defined(__FreeBSD__)
							B [ i ] = bswap16( B[i] );
							#elif defined(__linux__)
							B [ i ] = bswap_16( B[i] );
							#else
							B [ i ] = ::libmaus2::util::ReverseByteOrder::reverseByteOrder<uint16_t>(B[i]);
							#endif
							break;
						case 1:
							break;
					}
				#endif

				pc = pa;
			}
		};

		typedef BitInputBufferTemplate<uint32_t,FileStreamBaseType::unique_ptr_type> BitInputBuffer4;
	}
}
#endif
