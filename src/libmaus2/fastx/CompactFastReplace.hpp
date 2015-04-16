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


#if ! defined(LIBMAUS_FASTX_COMPACTFASTREPLACE_HPP)
#define LIBMAUS_FASTX_COMPACTFASTREPLACE_HPP

#include <libmaus2/util/GetObject.hpp>
#include <libmaus2/util/PutObject.hpp>
#include <libmaus2/util/utf8.hpp>
#include <libmaus2/fastx/CompactFastEncoder.hpp>

namespace libmaus2
{
	namespace fastx
	{			

		struct CompactFastReplace : public ::libmaus2::util::UTF8, public CompactFastTerminator
		{
			static void replacePattern(uint8_t * text, std::string const & pattern)
			{
				::libmaus2::util::GetObject<uint8_t *> G(text);

				// decode pattern length
				uint32_t const patlen = decodeUTF8(G);
				// decode flags
				uint32_t const flags = decodeUTF8(G);
				// does stored pattern have indeterminate bases?
				bool const storedHasIndeterminate = (flags & 1) != 0;
				// does mapped pattern have indeterminate bases?
				bool const patternHasIndeterminate = hasIndeterminate(pattern);
				
				if ( patlen == getTerminator() )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "CompactFastDecoderBase::replacePattern() cannot replace terminator." << std::endl;
					se.finish();
					throw se;
				}
				if ( patlen != pattern.size() )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "CompactFastDecoderBase::replacePattern() cannot change length of stored pattern." << std::endl;
					se.finish();
					throw se;					
				}
				if ( patternHasIndeterminate && !storedHasIndeterminate )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "CompactFastDecoderBase::replacePattern() cannot replace determinate bases by indeterminate ones." << std::endl;
					se.finish();
					throw se;
				}
				
				// replace pattern
				::libmaus2::util::PutObject<uint8_t *> P(text);
				::libmaus2::fastx::CompactFastEncoder::encode(pattern.c_str(),patlen,flags,P);
			}
		};
	}
}
#endif
