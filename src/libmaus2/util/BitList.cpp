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

#include <libmaus2/util/BitList.hpp>

libmaus2::util::BitList::BitList(uint64_t words)
{
	for ( uint64_t i = 0; i < 64*words; ++i )
		B.push_front(false);
}

uint64_t libmaus2::util::BitList::select1(uint64_t rank) const
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
uint64_t libmaus2::util::BitList::select0(uint64_t rank) const
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

uint64_t libmaus2::util::BitList::rank1(uint64_t pos) const
{
	std::list<bool>::const_iterator I = B.begin();

	uint64_t pc = 0;
	for ( uint64_t i = 0; i <= pos; ++i )
	{
		if ( *I )
			pc++;
		I++;
	}

	return pc;
}

void libmaus2::util::BitList::insertBit(uint64_t pos, bool b)
{
	assert ( pos < B.size() );

	std::list<bool>::iterator I = B.begin();

	for ( uint64_t i = 0; i < pos; ++i )
		I++;

	B.insert(I,b);
	B.pop_back();
}
void libmaus2::util::BitList::deleteBit(uint64_t pos)
{
	assert ( pos < B.size() );

	std::list<bool>::iterator I = B.begin();

	for ( uint64_t i = 0; i < pos; ++i )
		I++;

	B.erase(I);
	B.push_back(false);
}
void libmaus2::util::BitList::setBit(uint64_t pos, bool b)
{
	assert ( pos < B.size() );

	std::list<bool>::iterator I = B.begin();

	for ( uint64_t i = 0; i < pos; ++i )
		I++;

	*I = b;
}

std::ostream & operator<<(std::ostream & out, libmaus2::util::BitList const & B)
{
	for ( std::list< bool >::const_iterator I = B.B.begin(); I != B.B.end(); ++I )
		out << ((*I)?"1":"0");
	return out;
}
