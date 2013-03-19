/**
    libmaus
    Copyright (C) 2002-2013 German Tischler
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

#if ! defined(UTF8_HPP)
#define UTF8_HPP

#include <libmaus/types/types.hpp>
#include <libmaus/exception/LibMausException.hpp>
#include <libmaus/util/Demangle.hpp>
#include <libmaus/math/lowbits.hpp>

namespace libmaus
{
	namespace util
	{
		struct UTF8
		{
			template<typename in_type>
			static uint32_t decodeUTF8(in_type & istr)
			{
				int len = 0;
				unsigned char mask = 0x80u;

				// compute length of utf8 repr.
				int const str0 = istr.get();

				if ( str0 < 0 )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "EOF in decodeUTF8(" << ::libmaus::util::Demangle::demangle<in_type>() <<" &)";
					se.finish();
					throw se;
				}
				
				while ( str0 & mask )
				{
					len++;
					mask >>= 1;
				}

				// get useable bits from first byte
				unsigned int bitsinfirstbyte = 8-len-1;
				uint32_t number = str0 & ::libmaus::math::lowbits(bitsinfirstbyte);

				// every additional byte provides 6 bits
				// of information
				while ( --len > 0 )
				{
					number <<= 6;
					int const strn = istr.get();

					if ( strn < 0 )
					{
						::libmaus::exception::LibMausException se;
						se.getStream() << "EOF in decodeUTF8(" << ::libmaus::util::Demangle::demangle<in_type>() <<" &)";
						se.finish();
						throw se;
					}

					number |= (strn) & 0x3f;
				}

				return number;
			}

			template<typename in_type>
			static uint32_t decodeUTF8(in_type & istr, uint64_t & codelen)
			{
				try
				{
					int len = 0;
					unsigned char mask = 0x80;

					// compute length of utf8 repr.
					int const str0 = istr.get();
					codelen += 1;

					if ( str0 < 0 )
					{
						::libmaus::exception::LibMausException se;
						se.getStream() << "EOF in decodeUTF8(" << ::libmaus::util::Demangle::demangle<in_type>() <<" &)";
						se.finish();
						throw se;
					}
					
					while ( str0 & mask )
					{
						len++;
						mask >>= 1;
					}
					
					// codelen += (1+len);

					// get useable bits from first byte
					unsigned int bitsinfirstbyte = 8-len-1;
					uint32_t number = str0 & ::libmaus::math::lowbits(bitsinfirstbyte);

					// every additional byte provides 6 bits
					// of information
					while ( --len > 0 )
					{
						number <<= 6;
						int const strn = istr.get();
						codelen += 1;

						if ( strn < 0 )
						{
							::libmaus::exception::LibMausException se;
							se.getStream() << "EOF in decodeUTF8(" << ::libmaus::util::Demangle::demangle<in_type>() <<" &)";
							se.finish();
							throw se;
						}

						number |= (strn) & 0x3f;
					}

					return number;
				}
				catch(std::exception const & ex)
				{
					// std::cerr << "exception in decodeUTF8(): " << ex.what() << std::endl;
					throw;
				}
			}

			template<typename out_type>
			static void encodeUTF8(uint32_t num, out_type & out)
			{
				if ( num <= 0x7F )
				{
					out.put(static_cast<uint8_t>(num));
				}
				else if ( num <= 0x7FF )
				{
					out.put(static_cast<uint8_t>(128 | 64 | (((1<<5)-1) & (num >> 6))));
					out.put(static_cast<uint8_t>(128 | (((1<<6)-1) & (num))));
				}
				else if ( num <= 0x0000FFFF )
				{
					out.put(static_cast<uint8_t>(128 | 64 | 32 | (((1<<4)-1) & (num >> 12))));
					out.put(static_cast<uint8_t>(128 | (((1<<6)-1) & (num >> 6))));
					out.put(static_cast<uint8_t>(128 | (((1<<6)-1) & (num))));
				}
				else if ( num <= 0x001FFFFF )
				{
					out.put(static_cast<uint8_t>(128 | 64 | 32 | 16 | (((1<<3)-1) & (num >> 18))));
					out.put(static_cast<uint8_t>(128 | (((1<<6)-1) & (num >> 12))));
					out.put(static_cast<uint8_t>(128 | (((1<<6)-1) & (num >> 6))));
					out.put(static_cast<uint8_t>(128 | (((1<<6)-1) & (num))));
				}
				else if ( num <= 0x03FFFFFF )
				{
					out.put(static_cast<uint8_t>(128 | 64 | 32 | 16 | 8 | (((1<<2)-1) & (num >> 24))));
					out.put(static_cast<uint8_t>(128 | (((1<<6)-1) & (num >> 18))));
					out.put(static_cast<uint8_t>(128 | (((1<<6)-1) & (num >> 12))));
					out.put(static_cast<uint8_t>(128 | (((1<<6)-1) & (num >> 6))));
					out.put(static_cast<uint8_t>(128 | (((1<<6)-1) & (num))));
				}
				else if ( num <= 0x7FFFFFFF )
				{
					out.put(static_cast<uint8_t>(128 | 64 | 32 | 16 | 8 | 4 | (((1<<1)-1) & (num >> 30))));
					out.put(static_cast<uint8_t>(128 | (((1<<6)-1) & (num >> 24))));
					out.put(static_cast<uint8_t>(128 | (((1<<6)-1) & (num >> 18))));
					out.put(static_cast<uint8_t>(128 | (((1<<6)-1) & (num >> 12))));
					out.put(static_cast<uint8_t>(128 | (((1<<6)-1) & (num >> 6))));
					out.put(static_cast<uint8_t>(128 | (((1<<6)-1) & (num))));
				}
				else
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Value " << num << " passed to encodeUTF8 is out of range for code.";
					se.finish();
					throw se;
				}
			}

			static void encodeUTF8Pointer(uint32_t num, uint8_t * & out)
			{
				if ( num <= 0x7F )
				{
					*(out++) = (static_cast<uint8_t>(num));
				}
				else if ( num <= 0x7FF )
				{
					*(out++) = (static_cast<uint8_t>(128 | 64 | (((1<<5)-1) & (num >> 6))));
					*(out++) = (static_cast<uint8_t>(128 | (((1<<6)-1) & (num))));
				}
				else if ( num <= 0x0000FFFF )
				{
					*(out++) = (static_cast<uint8_t>(128 | 64 | 32 | (((1<<4)-1) & (num >> 12))));
					*(out++) = (static_cast<uint8_t>(128 | (((1<<6)-1) & (num >> 6))));
					*(out++) = (static_cast<uint8_t>(128 | (((1<<6)-1) & (num))));
				}
				else if ( num <= 0x001FFFFF )
				{
					*(out++) = (static_cast<uint8_t>(128 | 64 | 32 | 16 | (((1<<3)-1) & (num >> 18))));
					*(out++) = (static_cast<uint8_t>(128 | (((1<<6)-1) & (num >> 12))));
					*(out++) = (static_cast<uint8_t>(128 | (((1<<6)-1) & (num >> 6))));
					*(out++) = (static_cast<uint8_t>(128 | (((1<<6)-1) & (num))));
				}
				else if ( num <= 0x03FFFFFF )
				{
					*(out++) = (static_cast<uint8_t>(128 | 64 | 32 | 16 | 8 | (((1<<2)-1) & (num >> 24))));
					*(out++) = (static_cast<uint8_t>(128 | (((1<<6)-1) & (num >> 18))));
					*(out++) = (static_cast<uint8_t>(128 | (((1<<6)-1) & (num >> 12))));
					*(out++) = (static_cast<uint8_t>(128 | (((1<<6)-1) & (num >> 6))));
					*(out++) = (static_cast<uint8_t>(128 | (((1<<6)-1) & (num))));
				}
				else if ( num <= 0x7FFFFFFF )
				{
					*(out++) = (static_cast<uint8_t>(128 | 64 | 32 | 16 | 8 | 4 | (((1<<1)-1) & (num >> 30))));
					*(out++) = (static_cast<uint8_t>(128 | (((1<<6)-1) & (num >> 24))));
					*(out++) = (static_cast<uint8_t>(128 | (((1<<6)-1) & (num >> 18))));
					*(out++) = (static_cast<uint8_t>(128 | (((1<<6)-1) & (num >> 12))));
					*(out++) = (static_cast<uint8_t>(128 | (((1<<6)-1) & (num >> 6))));
					*(out++) = (static_cast<uint8_t>(128 | (((1<<6)-1) & (num))));
				}
				else
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Value " << num << " passed to encodeUTF8 is out of range for code.";
					se.finish();
					throw se;
				}
			}
		};
	}
}
#endif
