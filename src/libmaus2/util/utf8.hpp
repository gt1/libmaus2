/**
    libmaus2
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

#include <libmaus2/types/types.hpp>
#include <libmaus2/exception/LibMausException.hpp>
#include <libmaus2/util/Demangle.hpp>
#include <libmaus2/math/lowbits.hpp>

namespace libmaus2
{
	namespace util
	{
		template<typename stream_type, unsigned int k>
		struct Utf8Loop
		{
			static uint32_t utf8loop(stream_type & stream, uint32_t code)
			{
				code <<= 6;
				code  |= static_cast<uint32_t>(stream.get()) & 0x3Ful;
				return Utf8Loop<stream_type,k-1>::utf8loop(stream,code);
			}
		};

		template<typename stream_type>
		struct Utf8Loop<stream_type,1>
		{
			static uint32_t utf8loop(stream_type &, uint32_t code)
			{
				return code;
			}
		};

		struct UTF8
		{
			template<typename stream_type>
			static uint32_t decodeUTF8Unchecked(stream_type & stream)
			{
				uint32_t code = stream.get();

				static unsigned int const tcl[] = { 1,0,2,3,4,5,6 };
				unsigned int const cl = tcl[__builtin_clz((~static_cast<unsigned int>(code)) << (8*(sizeof(unsigned int)-1)))];

				switch ( cl )
				{
					case 1:          code &= 0x7F;                                                  break;
					case 2:          code = Utf8Loop<stream_type,2>::utf8loop(stream,code & 0x1Ful); break;
					case 3:          code = Utf8Loop<stream_type,3>::utf8loop(stream,code & 0x0Ful); break;
					case 4:          code = Utf8Loop<stream_type,4>::utf8loop(stream,code & 0x07ul); break;
					case 5:          code = Utf8Loop<stream_type,5>::utf8loop(stream,code & 0x03ul); break;
					case 6: default: code = Utf8Loop<stream_type,6>::utf8loop(stream,code & 0x01ul); break;
				}

				return code;
			}

			template<typename stream_type>
			static uint32_t decodeUTF8UncheckedEOF(stream_type & stream)
			{
				int const c = stream.get();

				if ( c < 0 )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "EOF in decodeUTF8UncheckedEOF(" << ::libmaus2::util::Demangle::demangle<stream_type>() <<" &)";
					se.finish();
					throw se;
				}

				uint32_t code = c;

				static unsigned int const tcl[] = { 1,0,2,3,4,5,6 };
				unsigned int const cl = tcl[__builtin_clz((~static_cast<unsigned int>(code)) << (8*(sizeof(unsigned int)-1)))];

				switch ( cl )
				{
					case 1:          code &= 0x7F;                                                  break;
					case 2:          code = Utf8Loop<stream_type,2>::utf8loop(stream,code & 0x1Ful); break;
					case 3:          code = Utf8Loop<stream_type,3>::utf8loop(stream,code & 0x0Ful); break;
					case 4:          code = Utf8Loop<stream_type,4>::utf8loop(stream,code & 0x07ul); break;
					case 5:          code = Utf8Loop<stream_type,5>::utf8loop(stream,code & 0x03ul); break;
					case 6: default: code = Utf8Loop<stream_type,6>::utf8loop(stream,code & 0x01ul); break;
				}

				return code;
			}

			template<typename in_type>
			static uint32_t decodeUTF8(in_type & istr)
			{
				int len = 0;
				unsigned char mask = 0x80u;

				// compute length of utf8 repr.
				int const str0 = istr.get();

				if ( str0 < 0 )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "EOF in decodeUTF8(" << ::libmaus2::util::Demangle::demangle<in_type>() <<" &)";
					se.finish();
					throw se;
				}

				if ( (str0 & 0xc0) == 0x80 )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "Defect code in decodeUTF8(" << ::libmaus2::util::Demangle::demangle<in_type>() <<" &)";
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
				uint32_t number = str0 & ::libmaus2::math::lowbits(bitsinfirstbyte);

				// every additional byte provides 6 bits
				// of information
				while ( --len > 0 )
				{
					number <<= 6;
					int const strn = istr.get();

					if ( strn < 0 )
					{
						::libmaus2::exception::LibMausException se;
						se.getStream() << "EOF in decodeUTF8(" << ::libmaus2::util::Demangle::demangle<in_type>() <<" &)";
						se.finish();
						throw se;
					}
					if ( (strn & 0xc0) != 0x80 )
					{
						::libmaus2::exception::LibMausException se;
						se.getStream() << "Defect code in decodeUTF8(" << ::libmaus2::util::Demangle::demangle<in_type>() <<" &)";
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
						::libmaus2::exception::LibMausException se;
						se.getStream() << "EOF in decodeUTF8(" << ::libmaus2::util::Demangle::demangle<in_type>() <<" &)";
						se.finish();
						throw se;
					}

					if ( (str0 & 0xc0) == 0x80 )
					{
						::libmaus2::exception::LibMausException se;
						se.getStream() << "Defect code in decodeUTF8(" << ::libmaus2::util::Demangle::demangle<in_type>() <<" &)";
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
					uint32_t number = str0 & ::libmaus2::math::lowbits(bitsinfirstbyte);

					// every additional byte provides 6 bits
					// of information
					while ( --len > 0 )
					{
						number <<= 6;
						int const strn = istr.get();
						codelen += 1;

						if ( strn < 0 )
						{
							::libmaus2::exception::LibMausException se;
							se.getStream() << "EOF in decodeUTF8(" << ::libmaus2::util::Demangle::demangle<in_type>() <<" &)";
							se.finish();
							throw se;
						}
						if ( (strn & 0xc0) != 0x80 )
						{
							::libmaus2::exception::LibMausException se;
							se.getStream() << "Defect code in decodeUTF8(" << ::libmaus2::util::Demangle::demangle<in_type>() <<" &)";
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
			static unsigned int encodeUTF8(uint32_t num, out_type & out)
			{
				if ( num <= 0x7F )
				{
					out.put(static_cast<uint8_t>(num));
					return 1;
				}
				else if ( num <= 0x7FF )
				{
					out.put(static_cast<uint8_t>(128 | 64 | (((1<<5)-1) & (num >> 6))));
					out.put(static_cast<uint8_t>(128 | (((1<<6)-1) & (num))));
					return 2;
				}
				else if ( num <= 0x0000FFFF )
				{
					out.put(static_cast<uint8_t>(128 | 64 | 32 | (((1<<4)-1) & (num >> 12))));
					out.put(static_cast<uint8_t>(128 | (((1<<6)-1) & (num >> 6))));
					out.put(static_cast<uint8_t>(128 | (((1<<6)-1) & (num))));
					return 3;
				}
				else if ( num <= 0x001FFFFF )
				{
					out.put(static_cast<uint8_t>(128 | 64 | 32 | 16 | (((1<<3)-1) & (num >> 18))));
					out.put(static_cast<uint8_t>(128 | (((1<<6)-1) & (num >> 12))));
					out.put(static_cast<uint8_t>(128 | (((1<<6)-1) & (num >> 6))));
					out.put(static_cast<uint8_t>(128 | (((1<<6)-1) & (num))));
					return 4;
				}
				else if ( num <= 0x03FFFFFF )
				{
					out.put(static_cast<uint8_t>(128 | 64 | 32 | 16 | 8 | (((1<<2)-1) & (num >> 24))));
					out.put(static_cast<uint8_t>(128 | (((1<<6)-1) & (num >> 18))));
					out.put(static_cast<uint8_t>(128 | (((1<<6)-1) & (num >> 12))));
					out.put(static_cast<uint8_t>(128 | (((1<<6)-1) & (num >> 6))));
					out.put(static_cast<uint8_t>(128 | (((1<<6)-1) & (num))));
					return 5;
				}
				else if ( num <= 0x7FFFFFFF )
				{
					out.put(static_cast<uint8_t>(128 | 64 | 32 | 16 | 8 | 4 | (((1<<1)-1) & (num >> 30))));
					out.put(static_cast<uint8_t>(128 | (((1<<6)-1) & (num >> 24))));
					out.put(static_cast<uint8_t>(128 | (((1<<6)-1) & (num >> 18))));
					out.put(static_cast<uint8_t>(128 | (((1<<6)-1) & (num >> 12))));
					out.put(static_cast<uint8_t>(128 | (((1<<6)-1) & (num >> 6))));
					out.put(static_cast<uint8_t>(128 | (((1<<6)-1) & (num))));
					return 6;
				}
				else
				{
					::libmaus2::exception::LibMausException se;
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
					::libmaus2::exception::LibMausException se;
					se.getStream() << "Value " << num << " passed to encodeUTF8 is out of range for code.";
					se.finish();
					throw se;
				}
			}
		};
	}
}
#endif
