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

#include <libmaus2/bitio/Clz.hpp>
#include <libmaus2/util/unique_ptr.hpp>

namespace libmaus2
{
	namespace gamma
	{
		template<typename stream_data_type>
		struct GammaDecoderBase
		{
		};

		template<>
		struct GammaDecoderBase<uint64_t>
		{
			typedef uint64_t stream_data_type;

			static unsigned int clz(stream_data_type const v)
			{
				return libmaus2::bitio::Clz::clz(v);
			}
		};

		#if defined(LIBMAUS2_HAVE_UNSIGNED_INT128)
		template<>
		struct GammaDecoderBase<libmaus2::uint128_t>
		{
			typedef libmaus2::uint128_t stream_data_type;

			static unsigned int clz(stream_data_type const v)
			{
				if ( v >> 64 )
					return libmaus2::bitio::Clz::clz(static_cast<uint64_t>(v >> 64));
				else
					return 64 + libmaus2::bitio::Clz::clz(static_cast<uint64_t>(v));
			}
		};
		#endif

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

					bool const ok = stream.getNext(v);
					assert ( ok );
					// v = stream.get();
					bav = (CHAR_BIT*sizeof(stream_data_type));

					code |= v >> ((CHAR_BIT*sizeof(stream_data_type))-restbits);
					v <<= restbits;
					bav -= restbits;

					return code;
				}
			}

			stream_data_type decode()
			{
				unsigned int cl;

				// decode code length
				if ( v )
				{
					cl = base_type::clz(v);
					v <<= cl;
					bav -= cl;
				}
				else
				{
					cl = bav;

					// read next word
					bool const ok = stream.getNext(v);
					assert ( ok );
					bav = (CHAR_BIT*sizeof(stream_data_type));

					unsigned int const llz = base_type::clz(v);
					cl += llz;
					v <<= llz;
					bav -= llz;
				}

				stream_data_type code;
				unsigned int cl1 = cl+1;

				// is code completely in this word?
				if ( bav >= cl+1 )
				{
					code = (v >> ((CHAR_BIT*sizeof(stream_data_type))-(cl1)));
					bav -= cl1;
					v <<= cl1;
				}
				// code is not completely in this word, include the next one
				else
				{
					// take rest of current word
					code = (v >> ((CHAR_BIT*sizeof(stream_data_type))-(bav)));
					cl1 -= bav;

					// read next word
					bool const ok = stream.getNext(v);
					assert ( ok );
					bav = (CHAR_BIT*sizeof(stream_data_type));

					code <<= cl1;
					code |= (v >> ((CHAR_BIT*sizeof(stream_data_type))-(cl1)));
					v <<= cl1;
					bav -= cl1;
				}

				return code-1;
			}
		};
	}
}
#endif
