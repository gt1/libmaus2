/*
    libmaus
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2015 Genome Research Limited

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
#if ! defined(LIBMAUS_BAMBAM_STRCMPNUMRECODE_HPP)
#define LIBMAUS_BAMBAM_STRCMPNUMRECODE_HPP

#include <libmaus/bambam/StrCmpNum.hpp>
#include <libmaus/util/NotDigitOrTermTable.hpp>
#include <libmaus/autoarray/AutoArray.hpp>

namespace libmaus
{
	namespace bambam
	{						
		struct StrCmpNumRecode
		{
			static size_t recode(char const * ca, libmaus::autoarray::AutoArray<uint64_t> & A)
			{
				unsigned char const * a = reinterpret_cast<unsigned char const *>(ca);
				size_t j = 0;
								
				while ( *a )
				{
					uint64_t v;
					if ( *a < '0' )
					{
						/*
						 * symbol is below '0' = 0x2f
						 * we store up to 8 byte as long as they are not numbers and not the terminator
						 */
						v = 0;
						int i = 7;
						for ( ; libmaus::util::NotDigitOrTermTable::table[*a] && (i >= 0); --i, ++a )
							v |= (static_cast<uint64_t>(*a) << (i<<3));
						/*
						 * append next symbol or the terminator
						 */
						if ( i >= 0 )
						{
							if ( StrCmpNum::digit_table[*a] )
								v |= (static_cast<uint64_t>('0') << (i<<3));
							else						
								v |= (static_cast<uint64_t>(*a) << (i<<3));
						}
					}
					else if ( *a <= '9' )
					{
						assert ( *a >= '0' );
						v = *(a++) - '0';
						while ( StrCmpNum::digit_table[*a] )
						{
							v *= 10;
							v += *(a++) - '0';
						}

						/*
						 * set second most significant bit
						 */
						v |= 0x4000000000000000ull;
					}
					else
					{
						/*
						 * set most significant bit
						 */
						v = 0x8000000000000000ull;
						int i = 6;
						/*
						 * store up to seven symbols
						 */
						for ( ; libmaus::util::NotDigitOrTermTable::table[*a] && (i >= 0); --i, ++a )
							v |= (static_cast<uint64_t>(*a) << (i<<3));
						/*
						 * append next symbol
						 */
						if ( i >= 0 )
						{
							if ( StrCmpNum::digit_table[*a] )
								v |= (static_cast<uint64_t>('0') << (i<<3));
							else
								v |= (static_cast<uint64_t>(*a) << (i<<3));
						}
					}
					
					if ( !(j < A.size()) )
						A.resize(A.size()+1);

					assert ( j < A.size() );
					A[j++] = v;
				}
				
				if ( !(j < A.size()) )
					A.resize(A.size()+1);

				assert ( j < A.size() );
				A[j++] = 0;
				
				return j;
			}
		};
	}
}
#endif
