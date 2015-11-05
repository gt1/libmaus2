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
#if ! defined(LIBMAUS2_FASTX_COMPACTFASTQDECODERBASE_HPP)
#define LIBMAUS2_FASTX_COMPACTFASTQDECODERBASE_HPP

#include <libmaus2/bitio/ArrayDecode.hpp>
#include <libmaus2/fastx/CompactFastDecoder.hpp>
#include <libmaus2/fastx/CompactFastQContext.hpp>
#include <libmaus2/fastx/CompactFastQHeader.hpp>
#include <libmaus2/fastx/FastQElement.hpp>

namespace libmaus2
{
	namespace fastx
	{
		struct CompactFastQDecoderBase
		{
			typedef ::libmaus2::fastx::FASTQEntry pattern_type;
			typedef ::libmaus2::fastx::FastQElement element_type;

			template<typename stream_type>
			static bool decodePattern(
				stream_type & stream,
				::libmaus2::fastx::CompactFastQHeader const & header,
				::libmaus2::fastx::CompactFastQContext & context,
				pattern_type & pattern
			)
			{
				bool const ok = ::libmaus2::fastx::CompactFastDecoderBase::decode(pattern,stream,context.nextid);

				std::ostringstream sidstr;
				sidstr << "read_" << pattern.patid;
				pattern.sid = sidstr.str();

				if ( ! ok )
					return false;

				uint64_t const l = pattern.getPatternLength();
				if ( l > context.qbuf.size() )
					context.qbuf = ::libmaus2::autoarray::AutoArray<uint8_t>(l);

				// decode weight array
				::libmaus2::util::PutObject<uint8_t *> P(context.qbuf.begin());
				::libmaus2::bitio::ArrayDecode::decodeArray(stream,P,l,header.qbits);

				// dequantise
				for ( uint64_t i = 0; i < l; ++i )
					context.qbuf[i] = header.quant.phredForQuant[context.qbuf[i]]+33;

				pattern.quality = std::string(
					reinterpret_cast<char const *>(context.qbuf.begin()),
					reinterpret_cast<char const *>(context.qbuf.begin()+l));

				return true;
			}

			template<typename stream_type>
			static bool decodeElement(
				stream_type & stream,
				::libmaus2::fastx::CompactFastQHeader const & header,
				::libmaus2::fastx::CompactFastQContext & context,
				element_type & element
			)
			{
				pattern_type pattern;

				bool const ok = decodePattern(stream,header,context,pattern);

				if ( ! ok )
					return false;

				element.name = pattern.sid;
				element.query = pattern.pattern;
				element.plus = pattern.plus;
				element.quality = pattern.quality;

				return true;
			}
		};
	}
}
#endif
