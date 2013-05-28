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
#if ! defined(LIBMAUS_BAMBAM_DECODERBASE_HPP)
#define LIBMAUS_BAMBAM_DECODERBASE_HPP

#include <libmaus/types/types.hpp>
#include <libmaus/exception/LibMausException.hpp>

namespace libmaus
{
	namespace bambam
	{
		struct DecoderBase
		{
			template<typename stream_type>	
			static uint8_t getByte(stream_type & in)
			{
				int const c = in.get();
				
				if ( c < 0 )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Unexpected EOF in ::libmaus::bambam::DecoderBase::getByte()" << std::endl;
					se.finish();
					throw se;
				}
				
				return c;
			}
			
			template<typename stream_type>	
			static uint64_t getByteAsWord(stream_type & in)
			{
				return getByte(in);
			}

			template<typename stream_type>	
			static uint64_t getLEInteger(stream_type & in, unsigned int const l)
			{
				uint64_t v = 0;
				for ( unsigned int i = 0; i < l; ++i )
					v |= getByteAsWord(in) << (8*i);
				return v;
			}

			static uint64_t getLEInteger(uint8_t const * D, unsigned int const l)
			{
				uint64_t v = 0;
				for ( unsigned int i = 0; i < l; ++i )
					v |= static_cast<uint64_t>(D[i]) << (8*i);
				return v;
			}
		};
	}
}
#endif
