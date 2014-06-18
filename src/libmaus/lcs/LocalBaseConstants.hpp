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

#if ! defined(LIBMAUS_LCS_LOCALBASECONSTANTS_HPP)
#define LIBMAUS_LCS_LOCALBASECONSTANTS_HPP

#include <ostream>

namespace libmaus
{
	namespace lcs
	{
		struct LocalBaseConstants
		{
			enum step_type { STEP_MATCH, STEP_MISMATCH, STEP_INS, STEP_DEL, STEP_RESET };
			
			virtual ~LocalBaseConstants() {}
		};
		
		std::ostream & operator<<(std::ostream & out, LocalBaseConstants::step_type const s);
	}
}
#endif
