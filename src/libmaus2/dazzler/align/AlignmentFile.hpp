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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_ALIGNMENTFILE_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_ALIGNMENTFILE_HPP

#include <libmaus2/dazzler/align/Overlap.hpp>
		
namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct AlignmentFile : public libmaus2::dazzler::db::InputBase
			{
				static uint8_t const TRACE_XOVR = 125;
			
				int64_t novl; // number of overlaps
				int32_t tspace;
				bool small;
				size_t tbytes;
				
				int64_t alre;

				void deserialise(std::istream & in)
				{
					uint64_t offset = 0;
					novl = getLittleEndianInteger8(in,offset);
					tspace = getLittleEndianInteger4(in,offset);
										
					if ( tspace <= TRACE_XOVR )
					{
						small = true;
						tbytes = sizeof(uint8_t);
					}
					else
					{
						small = false;
						tbytes = sizeof(uint16_t);
					}					
				}
				
				AlignmentFile()
				{
				}
				
				AlignmentFile(std::istream & in)
				{
					deserialise(in);
					alre = 0;
				}
				
				bool getNextOverlap(std::istream & in, Overlap & OVL)
				{
					if ( alre >= novl )
						return false;
				
					OVL.deserialise(in);
					OVL.path.path.resize(OVL.path.tlen/2);
						
					assert ( OVL.path.tlen % 2 == 0 );
						
					if ( small )
					{
						for ( int32_t i = 0; i < OVL.path.tlen/2; ++i )
						{
							int const a = in.get();
							int const b = in.get();
							
							if ( a < 0 || b < 0 )
							{
								libmaus2::exception::LibMausException lme;
								lme.getStream() << "AlignmentFile: failed to read trace path" << std::endl;
								lme.finish();
								throw lme;
							}
							
							OVL.path.path[i] = Path::tracepoint(a,b);
						}
					}
					else
					{
						for ( int32_t i = 0; i < OVL.path.tlen/2; ++i )
						{
							uint64_t offset = 0;
							int16_t const a = getLittleEndianInteger2(in,offset);
							int16_t const b = getLittleEndianInteger2(in,offset);
							OVL.path.path[i] = Path::tracepoint(a,b);
						}
					}
					
					alre += 1;
					
					return true;
				}				
			};
		}
	}
}
#endif
