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

#if ! defined(EDITDISTANCERESULT_HPP)
#define EDITDISTANCERESULT_HPP

namespace libmaus
{
	namespace lcs
	{
		struct EditDistanceResult
		{
			uint64_t numins;
			uint64_t numdel;
			uint64_t nummat;
			uint64_t nummis;
			
			EditDistanceResult()
			: numins(0), numdel(0), nummat(0), nummis(0)
			{}
			
			EditDistanceResult(uint64_t rnumins, uint64_t rnumdel, uint64_t rnummat, uint64_t rnummis)
			: numins(rnumins), numdel(rnumdel), nummat(rnummat), nummis(rnummis)
			{}
		};
	}
}
#endif
