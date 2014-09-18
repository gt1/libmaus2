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

#if ! defined(LOCALEDITDISTANCERESULT_HPP)
#define LOCALEDITDISTANCERESULT_HPP

#include <libmaus/types/types.hpp>
#include <ostream>

namespace libmaus
{
	namespace lcs
	{
		struct LocalEditDistanceResult
		{
			uint64_t numins;
			uint64_t numdel;
			uint64_t nummat;
			uint64_t nummis;
			
			uint64_t a_clip_left;
			uint64_t a_clip_right;
			uint64_t b_clip_left;
			uint64_t b_clip_right;
			
			
			LocalEditDistanceResult()
			: numins(0), numdel(0), nummat(0), nummis(0)
			{}
			
			LocalEditDistanceResult(
				uint64_t rnumins, uint64_t rnumdel, uint64_t rnummat, uint64_t rnummis,
				uint64_t ra_clip_left,
				uint64_t ra_clip_right,
				uint64_t rb_clip_left,
				uint64_t rb_clip_right
			)
			: numins(rnumins), numdel(rnumdel), nummat(rnummat), nummis(rnummis),
			  a_clip_left(ra_clip_left),
			  a_clip_right(ra_clip_right),
			  b_clip_left(rb_clip_left),
			  b_clip_right(rb_clip_right)
			{}
			
			bool operator==(LocalEditDistanceResult const & o) const
			{
				return
					numins == o.numins &&
					numdel == o.numdel &&
					nummat == o.nummat &&
					nummis == o.nummis;
			}
			
			bool operator!=(LocalEditDistanceResult const & o) const
			{
				return !operator==(o);
			}

			double getErrorRate() const
			{
				return
					static_cast<double>(numins + numdel + nummis) / static_cast<double>(numins+numdel+nummat+nummis);
			}
		};
		
		std::ostream & operator<<(std::ostream & out, LocalEditDistanceResult const &);
	}
}
#endif
