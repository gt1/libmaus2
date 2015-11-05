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
#if ! defined(LIBMAUS2_DAZZLER_DB_INPUTBASE_HPP)
#define LIBMAUS2_DAZZLER_DB_INPUTBASE_HPP

#include <libmaus2/exception/LibMausException.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace db
		{
			struct InputBase
			{
				template<typename stream_type>
				static void align(stream_type & in, size_t const s, uint64_t & offset)
				{
					while ( offset % s )
					{
						int c = in.get();
						if ( c < 0 )
						{
							libmaus2::exception::LibMausException lme;
							lme.getStream() << "InputBase::align: read failure/eof" << std::endl;
							lme.finish();
							throw lme;
						}
						offset += 1;
					}
				}

				template<typename stream_type>
				static int32_t getLittleEndianInteger4(stream_type & in, uint64_t & offset)
				{
					align(in, sizeof(int32_t), offset);

					int32_t v = 0;
					for ( size_t i = 0; i < sizeof(int32_t); ++i )
					{
						int c = in.get();
						if ( c < 0 )
						{
							libmaus2::exception::LibMausException lme;
							lme.getStream() << "InputBase::getLittleEndianInteger4: read failure/eof while expecting number" << std::endl;
							lme.finish();
							throw lme;
						}
						offset += 1;
						v |= (static_cast<int32_t>(static_cast<uint8_t>(c)) << (8*i));
					}
					return v;
				}

				template<typename stream_type>
				static int16_t getLittleEndianInteger2(stream_type & in, uint64_t & offset)
				{
					align(in, sizeof(int16_t), offset);

					int16_t v = 0;
					for ( size_t i = 0; i < sizeof(int16_t); ++i )
					{
						int c = in.get();
						if ( c < 0 )
						{
							libmaus2::exception::LibMausException lme;
							lme.getStream() << "InputBase::getLittleEndianInteger2: read failure/eof while expecting number" << std::endl;
							lme.finish();
							throw lme;
						}
						offset += 1;
						v |= (static_cast<int16_t>(static_cast<uint8_t>(c)) << (8*i));
					}
					return v;
				}

				template<typename stream_type>
				static uint16_t getUnsignedLittleEndianInteger2(stream_type & in, uint64_t & offset)
				{
					align(in, sizeof(uint16_t), offset);

					uint16_t v = 0;
					for ( size_t i = 0; i < sizeof(uint16_t); ++i )
					{
						int c = in.get();
						if ( c < 0 )
						{
							libmaus2::exception::LibMausException lme;
							lme.getStream() << "InputBase::getLittleEndianInteger2: read failure/eof while expecting number" << std::endl;
							lme.finish();
							throw lme;
						}
						offset += 1;
						v |= (static_cast<uint16_t>(static_cast<uint8_t>(c)) << (8*i));
					}
					return v;
				}

				template<typename stream_type>
				static uint32_t getUnsignedLittleEndianInteger4(stream_type & in, uint64_t & offset)
				{
					align(in, sizeof(uint32_t), offset);

					uint32_t v = 0;
					for ( size_t i = 0; i < sizeof(uint32_t); ++i )
					{
						int c = in.get();
						if ( c < 0 )
						{
							libmaus2::exception::LibMausException lme;
							lme.getStream() << "InputBase::getLittleEndianInteger4: read failure/eof while expecting number" << std::endl;
							lme.finish();
							throw lme;
						}
						offset += 1;
						v |= (static_cast<uint32_t>(static_cast<uint8_t>(c)) << (8*i));
					}
					return v;
				}

				template<typename stream_type>
				static int64_t getLittleEndianInteger8(stream_type & in, uint64_t & offset)
				{
					align(in, sizeof(int64_t), offset);

					int64_t v = 0;
					for ( size_t i = 0; i < sizeof(int64_t); ++i )
					{
						int c = in.get();
						if ( c < 0 )
						{
							libmaus2::exception::LibMausException lme;
							lme.getStream() << "InputBase::getLittleEndianInteger8: read failure/eof while expecting number" << std::endl;
							lme.finish();
							throw lme;
						}
						offset += 1;
						v |= (static_cast<int64_t>(static_cast<uint8_t>(c)) << (8*i));
					}
					return v;
				}

				template<typename stream_type>
				static float getFloat(stream_type & in, uint64_t & offset)
				{
					align(in, sizeof(int32_t), offset);

					uint32_t const u = getUnsignedLittleEndianInteger4(in, offset);
					union numberpun
					{
						float fvalue;
						uint32_t uvalue;
					};
					numberpun np;
					np.uvalue = u;
					return np.fvalue;
				}
			};
		}
	}
}
#endif
