/*
    libmaus2
    Copyright (C) 2015 German Tischler

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
#if ! defined(LIBMAUS2_LZ_XZDECODER_HPP)
#define LIBMAUS2_LZ_XZDECODER_HPP

#include <libmaus2/autoarray/AutoArray.hpp>

#if defined(LIBMAUS2_HAVE_LZMA)
#include <lzma.h>
#endif

namespace libmaus2
{
	namespace lz
	{
		struct XzDecoder
		{
			private:
			std::istream & in;
			libmaus2::autoarray::AutoArray<uint8_t> Ainbuf;
			libmaus2::autoarray::AutoArray<uint8_t> Aoutbuf;

			uint8_t const * pa;
			uint8_t const * pc;
			uint8_t const * pe;

			#if defined(LIBMAUS2_HAVE_LZMA)
			lzma_stream lstr;
			lzma_ret lastret;
			#endif

			void initDecoder();
			size_t fillInputBuffer();

			public:
			XzDecoder(std::istream & rin, size_t const inbufsize = 64*1024, size_t const outbufsize = 64*1024);
			~XzDecoder();

			size_t read(uint8_t * A, size_t s);
		};
	}
}
#endif
