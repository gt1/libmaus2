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

#include <libmaus/util/VarBitList.hpp>

libmaus::util::VarBitList::VarBitList()
{
}

uint64_t libmaus::util::VarBitList::select1(uint64_t rank) const
{
	std::list<bool>::const_iterator I = B.begin();

	uint64_t pos = 0;
	while ( ! (*I) )
	{
		I++;
		pos++;
	}
		
	while ( rank )
	{
		I++;
		pos++;
		
		
		while ( ! (*I) )
		{
			I++;
			pos++;
		}
		
		rank--;
	}	
	
	return pos;
}
uint64_t libmaus::util::VarBitList::select0(uint64_t rank) const
{
	std::list<bool>::const_iterator I = B.begin();

	uint64_t pos = 0;
	while ( (*I) )
	{
		I++;
		pos++;
	}
		
	while ( rank )
	{
		I++;
		pos++;
		
		
		while ( (*I) )
		{
			I++;
			pos++;
		}
		
		rank--;
	}	
	
	return pos;
}


uint64_t libmaus::util::VarBitList::rank1(uint64_t pos)
{
	std::list<bool>::iterator I = B.begin();
	
	uint64_t pc = 0;
	for ( uint64_t i = 0; i <= pos; ++i )
	{
		if ( *I )
			pc++;
		I++;
	}
	
	return pc;
}

void libmaus::util::VarBitList::insertBit(uint64_t pos, bool b)
{
	assert ( pos <= B.size() );

	std::list<bool>::iterator I = B.begin();
	
	for ( uint64_t i = 0; i < pos; ++i )
		I++;
		
	B.insert(I,b);
}
void libmaus::util::VarBitList::deleteBit(uint64_t pos)
{
	assert ( pos < B.size() );

	std::list<bool>::iterator I = B.begin();
	
	for ( uint64_t i = 0; i < pos; ++i )
		I++;

	B.erase(I);	
}
void libmaus::util::VarBitList::setBit(uint64_t pos, bool b)
{
	assert ( pos < B.size() );

	std::list<bool>::iterator I = B.begin();
	
	for ( uint64_t i = 0; i < pos; ++i )
		I++;

	*I = b;	
}

std::ostream & libmaus::util::operator<<(std::ostream & out, libmaus::util::VarBitList const & B)
{
	for ( std::list< bool >::const_iterator I = B.B.begin(); I != B.B.end(); ++I )
		out << ((*I)?"1":"0");
	return out;
}
