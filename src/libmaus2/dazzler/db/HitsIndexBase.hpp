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
#if ! defined(LIBMAUS2_DAZZLER_DB_HITSINDEXBASE_HPP)
#define LIBMAUS2_DAZZLER_DB_HITSINDEXBASE_HPP

#include <libmaus2/types/types.hpp>
#include <libmaus2/exception/LibMausException.hpp>
#include <istream>

namespace libmaus2
{
	namespace dazzler
	{
		namespace db
		{
			struct HitsIndexBase
			{
				// untrimmed number of reads
				int32_t ureads;
				// trimmed number of reads
				int32_t treads;
				// cut off
				int32_t cutoff;
				// multiple reads from each well?
				bool all;
				// frequency of bases A,C,G and T
				float freq[4];
				// maximum read length
				int32_t maxlen;
				// total number of bases in database
				int64_t totlen;
				// number of reads
				int32_t nreads;
				// is database trimmed?
				bool trimmed;
				// part this record refers to (0 for all of database, >0 for partial)
				int part;
				// first untrimmed record index
				int32_t ufirst;
				// first trimmed record index
				int32_t tfirst;
				// path to db
				std::string path;
				// are records loaded into memory
				bool loaded;
				
				static void align(std::istream & in, size_t const s, uint64_t & offset)
				{
					while ( offset % s )
					{
						int c = in.get();
						if ( c < 0 )
						{
							libmaus2::exception::LibMausException lme;
							lme.getStream() << "HitsIndexBase::align: read failure/eof" << std::endl;
							lme.finish();
							throw lme;
						}
						offset += 1;
					}
				}

				static int32_t getLittleEndianInteger4(std::istream & in, uint64_t & offset)
				{
					align(in, sizeof(int32_t), offset);

					int32_t v = 0;
					for ( size_t i = 0; i < sizeof(int32_t); ++i )
					{
						int c = in.get();
						if ( c < 0 )
						{
							libmaus2::exception::LibMausException lme;
							lme.getStream() << "HitsIndexBase::getLittleEndianInteger4: read failure/eof while expecting number" << std::endl;
							lme.finish();
							throw lme;
						}
						offset += 1;
						v |= (static_cast<int32_t>(static_cast<uint8_t>(c)) << (8*i));
					}
					return v;
				}

				static uint32_t getUnsignedLittleEndianInteger4(std::istream & in, uint64_t & offset)
				{
					align(in, sizeof(uint32_t), offset);

					uint32_t v = 0;
					for ( size_t i = 0; i < sizeof(uint32_t); ++i )
					{
						int c = in.get();
						if ( c < 0 )
						{
							libmaus2::exception::LibMausException lme;
							lme.getStream() << "HitsIndexBase::getLittleEndianInteger4: read failure/eof while expecting number" << std::endl;
							lme.finish();
							throw lme;
						}
						offset += 1;
						v |= (static_cast<uint32_t>(static_cast<uint8_t>(c)) << (8*i));
					}
					return v;
				}

				static int64_t getLittleEndianInteger8(std::istream & in, uint64_t & offset)
				{
					align(in, sizeof(int64_t), offset);

					int64_t v = 0;
					for ( size_t i = 0; i < sizeof(int64_t); ++i )
					{
						int c = in.get();
						if ( c < 0 )
						{
							libmaus2::exception::LibMausException lme;
							lme.getStream() << "HitsIndexBase::getLittleEndianInteger8: read failure/eof while expecting number" << std::endl;
							lme.finish();
							throw lme;
						}
						offset += 1;
						v |= (static_cast<int64_t>(static_cast<uint8_t>(c)) << (8*i));
					}
					return v;
				}
				
				static float getFloat(std::istream & in, uint64_t & offset)
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

				
				HitsIndexBase()
				{
				
				}
				
				HitsIndexBase(std::istream & in)
				{
					deserialise(in);
				}
				
				void deserialise(std::istream & in)
				{
					uint64_t offset = 0;
				
					ureads  = getLittleEndianInteger4(in,offset);
					treads  = getLittleEndianInteger4(in,offset);
					cutoff  = getLittleEndianInteger4(in,offset);
					all     = getLittleEndianInteger4(in,offset);

					for ( size_t i = 0; i < sizeof(freq)/sizeof(freq[0]); ++i )
						freq[i] = getFloat(in,offset);

					maxlen  = getLittleEndianInteger4(in,offset);
					totlen  = getLittleEndianInteger8(in,offset);
					nreads  = getLittleEndianInteger4(in,offset);
					trimmed = getLittleEndianInteger4(in,offset);
					
					/* part = */   getLittleEndianInteger4(in,offset);
					/* ufirst = */ getLittleEndianInteger4(in,offset);
					/* tfirst = */ getLittleEndianInteger4(in,offset);
					
					/* path =   */ getLittleEndianInteger8(in,offset); // 8 byte pointer
					/* loaded = */ getLittleEndianInteger4(in,offset);
					/* bases =  */ getLittleEndianInteger8(in,offset); // 8 byte pointer
					
					/* reads =  */ getLittleEndianInteger8(in,offset); // 8 byte pointer
					/* tracks = */ getLittleEndianInteger8(in,offset); // 8 byte pointer				
					
					nreads = ureads;
				}
			};

			std::ostream & operator<<(std::ostream & out, HitsIndexBase const & H);
		}
	}
}
#endif
