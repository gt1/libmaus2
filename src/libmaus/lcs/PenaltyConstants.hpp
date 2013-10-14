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

#if ! defined(PENALTYCONSTANTS_HPP)
#define PENALTYCONSTANTS_HPP

#include <libmaus/lcs/BaseConstants.hpp>

namespace libmaus
{
	namespace lcs
	{
		struct PenaltyConstants : public BaseConstants
		{
			typedef int32_t similarity_type;
			static similarity_type const penalty_ins = 3;
			static similarity_type const penalty_del = 3;
			static similarity_type const penalty_subst = 1;
			static similarity_type const gain_match = 1;		
			
			virtual ~PenaltyConstants() {}
		};
	}
}
#endif
