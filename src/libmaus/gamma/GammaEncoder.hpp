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
#if ! defined(LIBMAUS_GAMMA_GAMMAENCODER_HPP)
#define LIBMAUS_GAMMA_GAMMAENCODER_HPP

#include <libmaus/bitio/Clz.hpp>
#include <libmaus/util/unique_ptr.hpp>
#include <libmaus/math/lowbits.hpp>

namespace libmaus
{
	namespace gamma
	{
		template<typename _stream_type>
		struct GammaEncoder : public libmaus::bitio::Clz
		{
			typedef _stream_type stream_type;
			typedef GammaEncoder<stream_type> this_type;
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			stream_type & stream;
			uint64_t v;
			unsigned int bav;
			
			GammaEncoder(stream_type & rstream)
			: stream(rstream), v(0), bav(64)
			{
			
			}
			
			static inline unsigned int getCodeLen(uint64_t const code)
			{
				unsigned int const nd = 63-clz(code);
				return 1 + (nd<<1);
			}
			
			inline void encodeWord(uint64_t const code, unsigned int const codelen)
			{
				if ( bav >= codelen )
				{
					v <<= codelen;
					v |= code;
					bav -= codelen;
				}
				else
				{
					unsigned int const overflow = (codelen-bav);
					stream.put((v << bav) | (code >> overflow));
					v = code & libmaus::math::lowbits(overflow); 
						// ((1ull << overflow)-1);
					bav = 64-overflow;
				}			
			}

			void encode(uint64_t const q)
			{
				uint64_t const code = q+1;	
				unsigned int codelen = getCodeLen(code);
				encodeWord(code,codelen);
			}
			
			void flush()
			{
				if ( bav != 64 )
				{
					v <<= bav;
					stream.put(v);
					v = 0;
					bav = 64;
				}
			}
			
			uint64_t getOffset() const
			{
				return 64 * stream.getWrittenWords() + (64-bav);
			}
		};
	}
}
#endif
