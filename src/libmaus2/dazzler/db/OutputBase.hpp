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
#if ! defined(LIBMAUS2_DAZZLER_DB_OUTPUTBASE_HPP)
#define LIBMAUS2_DAZZLER_DB_OUTPUTBASE_HPP

#include <libmaus2/exception/LibMausException.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace db
		{
			struct OutputBase
			{
				template<typename stream_type>
				static void align(stream_type & out, size_t const s, uint64_t & offset)
				{
					while ( offset % s )
					{
						out.put(0);
						if ( !out )
						{
							libmaus2::exception::LibMausException lme;
							lme.getStream() << "OutputBase::align: read failure/eof" << std::endl;
							lme.finish();
							throw lme;
						}
						offset += 1;
					}
				}

				template<typename stream_type>
				static void putLittleEndianInteger4(stream_type & out, int32_t const v, uint64_t & offset)
				{
					align(out, sizeof(int32_t), offset);

					for ( size_t i = 0; i < sizeof(int32_t); ++i )
					{
						out.put((v >> (8*i)) & 0xFF);
							
						if ( !out )
						{
							libmaus2::exception::LibMausException lme;
							lme.getStream() << "OutputBase::putLittleEndianInteger4: write error" << std::endl;
							lme.finish();
							throw lme;
						}
						offset += 1;
					}
				}

				template<typename stream_type>
				static void putLittleEndianInteger2(stream_type & out, int16_t const v, uint64_t & offset)
				{
					align(out, sizeof(int16_t), offset);

					for ( size_t i = 0; i < sizeof(int16_t); ++i )
					{
						out.put((v >> (8*i)) & 0xFF);
							
						if ( !out )
						{
							libmaus2::exception::LibMausException lme;
							lme.getStream() << "OutputBase::putLittleEndianInteger2: write error" << std::endl;
							lme.finish();
							throw lme;
						}
						offset += 1;

					}
				}

				template<typename stream_type>
				static void putUnsignedLittleEndianInteger2(stream_type & out, uint16_t const v, uint64_t & offset)
				{
					align(out, sizeof(uint16_t), offset);

					for ( size_t i = 0; i < sizeof(uint16_t); ++i )
					{
						out.put((v >> (8*i)) & 0xFF);
							
						if ( !out )
						{
							libmaus2::exception::LibMausException lme;
							lme.getStream() << "OutputBase::putUnsignedLittleEndianInteger2: write error" << std::endl;
							lme.finish();
							throw lme;
						}
						offset += 1;
					}
				}

				template<typename stream_type>
				static void putUnsignedLittleEndianInteger4(stream_type & out, uint32_t const v, uint64_t & offset)
				{
					align(out, sizeof(uint32_t), offset);

					for ( size_t i = 0; i < sizeof(uint32_t); ++i )
					{
						out.put((v >> (8*i)) & 0xFF);
							
						if ( !out )
						{
							libmaus2::exception::LibMausException lme;
							lme.getStream() << "OutputBase::putUnsignedLittleEndianInteger4: write error" << std::endl;
							lme.finish();
							throw lme;
						}
						offset += 1;
					}
				}

				template<typename stream_type>
				static void putLittleEndianInteger8(stream_type & out, int64_t const v, uint64_t & offset)
				{
					align(out, sizeof(int64_t), offset);

					for ( size_t i = 0; i < sizeof(int64_t); ++i )
					{
						out.put((v >> (8*i)) & 0xFF);
							
						if ( !out )
						{
							libmaus2::exception::LibMausException lme;
							lme.getStream() << "OutputBase::putUnsignedLittleEndianInteger4: write error" << std::endl;
							lme.finish();
							throw lme;
						}
						offset += 1;
					}
				}
				
				template<typename stream_type>
				static void putFloat(stream_type & out, float const v, uint64_t & offset)
				{
					align(out, sizeof(int32_t), offset);
					
					union numberpun
					{
						float fvalue;
						uint32_t uvalue;
					};
					numberpun np;
					np.fvalue = v;
					
					putUnsignedLittleEndianInteger4(out,np.uvalue,offset);
				}
			};
		}
	}
}
#endif
