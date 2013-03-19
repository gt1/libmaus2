/**
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
**/

#include <libmaus/rank/ERankBase.hpp>
#include <libmaus/rank/RankTable.hpp>
#include <libmaus/rank/popcnt.hpp>
#include <cassert>

namespace libmaus
{
	namespace rank
	{
		RankTable::RankTable() : table(generateTable()) {}
		RankTable::~RankTable() { delete [] table; }	
		uint8_t * RankTable::generateTable()
		{
			uint8_t * table = 0;
			
			try
			{
				table = new uint8_t [ (1<<19) ];
				
				unsigned int q = 0;
				for ( unsigned int m = 0; m < (1u<<16); ++m )
				{
					uint8_t c = 0;
					
					for ( unsigned int p = 0; p < 16; ++p )
					{
						if ( m & (1u<<(15-p)) )
							c += 1;
							
						c &= 0xF;

						if ( !(p & 1) )
						{
							table[q] = c;
							assert ( (table[ (m << 3) + (p >> 1) ] & 0xF) == c );
						}
						else
						{
							table[q++] |= (c<<4);
							assert ( (table[ (m << 3) + (p >> 1) ] >> 4) == c );
						}
					}
				}

				return table;
			}
			catch(...)
			{
				delete [] table;
				table = 0;
				throw;
			}
		}
		SimpleRankTable::SimpleRankTable() : table(generateTable()) {}
		SimpleRankTable::~SimpleRankTable() { delete [] table; }	
		uint8_t * SimpleRankTable::generateTable()
		{
			uint8_t * table = 0;
			
			try
			{
				table = new uint8_t [ (1<<16) ];
				
				for ( uint32_t m = 0; m < (1u<<16); ++m )
					table[m] = ::libmaus::rank::PopCnt4<sizeof(int)>::popcnt4(m);

				return table;
			}
			catch(...)
			{
				delete [] table;
				table = 0;
				throw;
			}
		}
	}
}