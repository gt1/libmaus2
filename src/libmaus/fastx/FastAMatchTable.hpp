/*
    libmaus
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#if ! defined(LIBMAUS_FASTX_FASTAMATCHTABLE_HPP)
#define LIBMAUS_FASTX_FASTAMATCHTABLE_HPP

#include <map>
#include <vector>
#include <ostream>
#include <libmaus/types/types.hpp>

namespace libmaus
{
	namespace fastx
	{
		struct FastAMatchTable
		{
			std::map< uint8_t, std::vector<uint8_t> > M;

			FastAMatchTable()
			{
				// A
				M['A'].push_back('A');
				// T/U
				M['T'].push_back('T');
				M['T'].push_back('U');
				M['U'].push_back('T');
				M['U'].push_back('U');
				// G
				M['G'].push_back('G');
				// C
				M['C'].push_back('C');
				// K
				M['K'].push_back('G');
				M['K'].push_back('T');
				// M
				M['M'].push_back('A');
				M['M'].push_back('C');
				// R
				M['R'].push_back('A');
				M['R'].push_back('G');
				// Y
				M['Y'].push_back('C');
				M['Y'].push_back('T');
				// S
				M['S'].push_back('C');
				M['S'].push_back('G');
				// W
				M['W'].push_back('A');
				M['W'].push_back('T');
				// B
				M['B'].push_back('C');
				M['B'].push_back('G');
				M['B'].push_back('T');
				// V
				M['V'].push_back('A');
				M['V'].push_back('C');
				M['V'].push_back('G');
				// H
				M['H'].push_back('A');
				M['H'].push_back('C');
				M['H'].push_back('T');
				// D
				M['D'].push_back('A');
				M['D'].push_back('G');
				M['D'].push_back('T');
				// N
				M['N'].push_back('A');
				M['N'].push_back('C');
				M['N'].push_back('G');
				M['N'].push_back('T');
			}
		};
		
		std::ostream & operator<<(std::ostream & out, FastAMatchTable const & F);
	}
}
#endif
