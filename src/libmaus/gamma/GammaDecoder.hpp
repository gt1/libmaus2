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
#if ! defined(LIBMAUS_GAMMA_GAMMADECODER_HPP)
#define LIBMAUS_GAMMA_GAMMADECODER_HPP

#include <libmaus/bitio/Clz.hpp>
#include <libmaus/util/unique_ptr.hpp>

namespace libmaus
{
	namespace gamma
	{
		template<typename _stream_type>
		struct GammaDecoder : public libmaus::bitio::Clz
		{
			typedef _stream_type stream_type;
			typedef GammaDecoder<stream_type> this_type;
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			stream_type & stream;
			uint64_t v;
			uint64_t bav;
			
			GammaDecoder(stream_type & rstream) : stream(rstream), v(0), bav(0) {}
			
			void flush()
			{
				bav = 0;
				v = 0;
			}
			
			uint64_t decodeWord(unsigned int const bits)
			{
				if ( bits <= bav )
				{
					// extract bits
					uint64_t const code = v >> (64-bits);

					// remove bits from stream
					v <<= bits;
					bav -= bits;

					return code;
				}
				else
				{
					unsigned int const restbits = bits-bav;
					uint64_t code = (v >> (64-bav)) << restbits;
					
					v = stream.get();
					bav = 64;
					
					code |= v >> (64-restbits);
					v <<= restbits;
					bav -= restbits;
					
					return code;
				}
			}
			
			uint64_t decode()
			{
				unsigned int cl;
				
				// decode code length
				if ( v )
				{
					cl = clz(v);
					v <<= cl;
					bav -= cl;
				}
				else
				{
					cl = bav;
					
					// read next word
					v = stream.get();
					bav = 64;

					unsigned int const llz = clz(v);
					cl += llz;
					v <<= llz;
					bav -= llz;
				}
				
				uint64_t code;
				unsigned int cl1 = cl+1;

				// is code completely in this word?
				if ( bav >= cl+1 )
				{
					code = (v >> (64-(cl1)));
					bav -= cl1;
					v <<= cl1;
				}
				// code is not completely in this word, include the next one
				else
				{
					// take rest of current word
					code = (v >> (64-(bav)));
					cl1 -= bav;
					
					// read next word
					v = stream.get();
					bav = 64;

					code <<= cl1;
					code |= (v >> (64-(cl1)));
					v <<= cl1;
					bav -= cl1;
				}
				
				return code-1;
			}
		};
	}
}
#endif
