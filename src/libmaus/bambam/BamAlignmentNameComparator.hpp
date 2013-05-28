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
#if ! defined(LIBMAUS_BAMBAM_BAMALIGNMENTNAMECOMPARATOR_HPP)
#define LIBMAUS_BAMBAM_BAMALIGNMENTNAMECOMPARATOR_HPP

#include <libmaus/bambam/BamAlignment.hpp>

namespace libmaus
{
	namespace bambam
	{
		struct BamAlignmentNameComparator
		{
			uint8_t const * data;
			static uint8_t const digit_table[256];
			
			BamAlignmentNameComparator(uint8_t const * rdata)
			: data(rdata)
			{
			
			}
			
			static int strcmpnum(uint8_t const * a, uint8_t const * b)
			{
				// std::cerr << "Comparing\na=" << a << "\nb=" << b << "\n";
			
				while ( *a && *b )
				{
					uint8_t const ca = *a;
					uint8_t const cb = *b;
			
					if ( digit_table[ca] && digit_table[cb] )
					{
						uint64_t na = ca - '0';
						uint64_t nb = cb - '0';
						
						while ( (*(++a)) && digit_table[*a] )
						{
							na *= 10;
							na += *a - '0';
						}
						while ( (*(++b)) && digit_table[*b] )
						{
							nb *= 10;
							nb += *b - '0';
						}
						
						// std::cerr << "na=" << na << " nb=" << nb << std::endl;
						
						if ( na != nb )
						{
							if ( na < nb )
								return -1;
							else
								return 1;
						}
					}
					else if ( ca != cb )
					{
						if ( ca < cb )
							return -1;
						else
							return 1;
					}
					else
					{
						++a;
						++b;
					}
				}
				
				return 0;	
			}
			
			static int strcmpnum(char const * a, char const * b)
			{
				return strcmpnum(
					reinterpret_cast<uint8_t const *>(a),
					reinterpret_cast<uint8_t const *>(b)
				);	
			}
			
			static bool compare(uint8_t const * da, uint8_t const * db)
			{
				char const * namea = ::libmaus::bambam::BamAlignmentDecoderBase::getReadName(da);
				char const * nameb = ::libmaus::bambam::BamAlignmentDecoderBase::getReadName(db);

				int const r = strcmpnum(namea,nameb);
				bool res;
				
				if ( r < 0 )
				{
					res = true;
				}
				else if ( r == 0 )
				{
					// read 1 before read 2
					res = ::libmaus::bambam::BamAlignmentDecoderBase::getFlags(da) & ::libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FREAD1;
				}
				else
				{
					res = false;
				}
				
				return res;
			}

			static int compareInt(uint8_t const * da, uint8_t const * db)
			{
				char const * namea = ::libmaus::bambam::BamAlignmentDecoderBase::getReadName(da);
				char const * nameb = ::libmaus::bambam::BamAlignmentDecoderBase::getReadName(db);

				int const r = strcmpnum(namea,nameb);
				
				if ( r != 0 )
					return r;

				// read 1 before read 2
				int const r1 = ::libmaus::bambam::BamAlignmentDecoderBase::getFlags(da) & ::libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FREAD1;
				int const r2 = ::libmaus::bambam::BamAlignmentDecoderBase::getFlags(db) & ::libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FREAD1;
				
				if ( r1 == r2 )
					return 0;
				else if ( r1 )
					return -1;
				else
					return 1;
			}

			static int compareIntNameOnly(uint8_t const * da, uint8_t const * db)
			{
				char const * namea = ::libmaus::bambam::BamAlignmentDecoderBase::getReadName(da);
				char const * nameb = ::libmaus::bambam::BamAlignmentDecoderBase::getReadName(db);

				return strcmpnum(namea,nameb);
			}
			
			bool operator()(uint64_t const a, uint64_t const b) const
			{
				uint8_t const * da = data + a + sizeof(uint32_t);
				uint8_t const * db = data + b + sizeof(uint32_t);
				return compare(da,db);
			}

			int compareInt(uint64_t const a, uint64_t const b) const
			{
				uint8_t const * da = data + a + sizeof(uint32_t);
				uint8_t const * db = data + b + sizeof(uint32_t);
				return compareInt(da,db);
			}

			int compareIntNameOnly(uint64_t const a, uint64_t const b) const
			{
				uint8_t const * da = data + a + sizeof(uint32_t);
				uint8_t const * db = data + b + sizeof(uint32_t);
				return compareIntNameOnly(da,db);
			}
		};
	}
}
#endif
