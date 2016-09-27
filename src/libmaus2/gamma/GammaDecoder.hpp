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
#if ! defined(LIBMAUS2_GAMMA_GAMMADECODER_HPP)
#define LIBMAUS2_GAMMA_GAMMADECODER_HPP

#include <climits>
#include <libmaus2/gamma/GammaDecoderBase.hpp>
#include <libmaus2/util/unique_ptr.hpp>

namespace libmaus2
{
	namespace gamma
	{
		template<typename _stream_type>
		struct GammaDecoder : public GammaDecoderBase<typename _stream_type::data_type>
		{
			typedef _stream_type stream_type;
			typedef typename stream_type::data_type stream_data_type;
			typedef GammaDecoder<stream_type> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef GammaDecoderBase<typename _stream_type::data_type> base_type;

			stream_type & stream;
			stream_data_type v;
			unsigned int bav;

			GammaDecoder(stream_type & rstream) : stream(rstream), v(0), bav(0) {}

			void flush()
			{
				bav = 0;
				v = 0;
			}

			stream_data_type decodeWord(unsigned int const bits)
			{
				if ( bits <= bav )
				{
					// extract bits
					stream_data_type const code = v >> ((CHAR_BIT*sizeof(stream_data_type))-bits);

					// remove bits from stream
					v <<= bits;
					bav -= bits;

					return code;
				}
				else
				{
					unsigned int const restbits = bits-bav;
					stream_data_type code = (v >> ((CHAR_BIT*sizeof(stream_data_type))-bav)) << restbits;

					#if ! defined(NDEBUG)
					bool const ok =
					#endif
						stream.getNext(v);
					#if ! defined(NDEBUG)
					assert ( ok );
					#endif
					// v = stream.get();
					bav = (CHAR_BIT*sizeof(stream_data_type));

					code |= v >> ((CHAR_BIT*sizeof(stream_data_type))-restbits);
					v <<= restbits;
					bav -= restbits;

					return code;
				}
			}

			static stream_data_type leftShift(stream_data_type const a, unsigned int const b)
			{
				if ( expect_false( b == (CHAR_BIT*sizeof(stream_data_type)) ) )
					return stream_data_type();
				else
					return a << b;
			}

			static stream_data_type rightShift(stream_data_type const a, unsigned int const b)
			{
				if ( expect_false( b == (CHAR_BIT*sizeof(stream_data_type)) ) )
					return stream_data_type();
				else
					return a >> b;
			}

			stream_data_type decode()
			{
				unsigned int cl;

				// decode code length
				if ( base_type::isNonNull(v) )
				{
					cl = base_type::clz(v);
					v <<= cl;
					bav -= cl;
				}
				else
				{
					cl = bav;

					while ( true )
					{
						// read next word
						#if ! defined(NDEBUG)
						bool const ok =
						#endif
							stream.getNext(v);
						#if ! defined(NDEBUG)
						assert ( ok );
						#endif
						bav = (CHAR_BIT*sizeof(stream_data_type));

						if ( expect_true ( base_type::isNonNull(v) ) )
						{
							unsigned int const llz = base_type::clz(v);
							assert ( llz != (CHAR_BIT*sizeof(stream_data_type)) );
							cl += llz;
							v <<= llz;
							bav -= llz;
							break;
						}
						else
						{
							cl += CHAR_BIT*sizeof(stream_data_type);
							v = 0;
							bav = 0;
						}
					}
				}

				stream_data_type code;
				unsigned int cl1 = cl+1;

				// is code completely in this word?
				if ( bav >= cl+1 )
				{
					unsigned int const shift = ((CHAR_BIT*sizeof(stream_data_type))-(cl1));
					code = rightShift(v,shift);
					bav -= cl1;
					v = leftShift(v,cl1);
				}
				// code is not completely in this word, include the next one
				else
				{
					// take rest of current word
					unsigned int const shift0 = ((CHAR_BIT*sizeof(stream_data_type))-(bav));
					code = rightShift(v,shift0);
					cl1 -= bav;

					// read next word
					#if ! defined(NDEBUG)
					bool const ok =
					#endif
						stream.getNext(v);
					#if ! defined(NDEBUG)
					assert ( ok );
					#endif

					bav = (CHAR_BIT*sizeof(stream_data_type));

					code = leftShift(code,cl1);
					unsigned int const shift1 = ((CHAR_BIT*sizeof(stream_data_type))-(cl1));
					code = code | rightShift(v,shift1);
					v = leftShift(v,cl1);
					bav -= cl1;
				}

				return code-1;
			}
		};
	}
}
#endif
