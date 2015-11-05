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

#if ! defined(LCSBASECONSTANTS_HPP)
#define LCSBASECONSTANTS_HPP

#include <ostream>

namespace libmaus2
{
	namespace lcs
	{
		struct BaseConstants
		{
			enum step_type { STEP_MATCH, STEP_MISMATCH, STEP_INS, STEP_DEL, STEP_RESET };
			typedef step_type * step_type_ptr;
			typedef step_type const * step_type_const_ptr;

			virtual ~BaseConstants() {}
		};

		inline std::ostream & operator<<(std::ostream & out, BaseConstants::step_type const s)
		{
			switch ( s )
			{
				case BaseConstants::STEP_MATCH: out << "+"; break;
				case BaseConstants::STEP_MISMATCH: out << "-"; break;
				case BaseConstants::STEP_INS: out << "I"; break;
				case BaseConstants::STEP_DEL: out << "D"; break;
				case BaseConstants::STEP_RESET: out << "R"; break;
			}
			return out;
		}
	}
}
#endif
