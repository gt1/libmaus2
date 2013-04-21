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
#include <libmaus/util/DigitTable.hpp>

libmaus::util::DigitTable::DigitTable()
{
	memset(&A[0],0,sizeof(A));
	A['0'] = 1;
	A['1'] = 1;
	A['2'] = 1;
	A['3'] = 1;
	A['4'] = 1;
	A['5'] = 1;
	A['6'] = 1;
	A['7'] = 1;
	A['8'] = 1;
	A['9'] = 1;
}
