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
#if ! defined(LIBMAUS_BAMBAM_ADAPTEROFFSETSTRAND_HPP)
#define LIBMAUS_BAMBAM_ADAPTEROFFSETSTRAND_HPP

#include <libmaus2/types/types.hpp>
#include <limits>

namespace libmaus2
{
	namespace bambam
	{
		struct AdapterOffsetStrand
		{
			uint16_t adpid;
			int16_t adpoff;
			uint16_t adpstr;
			int16_t score;
			double frac;
			double pfrac;
			uint16_t maxstart;
			uint16_t maxend;
			
			AdapterOffsetStrand() : adpid(0), adpoff(0), adpstr(0), score(std::numeric_limits<int16_t>::min()), frac(0), pfrac(0) {}
			AdapterOffsetStrand(
				uint16_t const radpid,
				int16_t const radpoff,
				uint16_t const radpstr
			) : adpid(radpid), adpoff(radpoff), adpstr(radpstr), score(std::numeric_limits<int16_t>::min()), frac(0), pfrac(0) {}
			
			bool operator<(AdapterOffsetStrand const & o) const
			{
				if ( adpid != o.adpid )
					return adpid < o.adpid;
				else if ( adpoff != o.adpoff )
					return adpoff < o.adpoff;
				else
					return adpstr < o.adpstr;
			}
			
			bool operator==(AdapterOffsetStrand const & o) const
			{
				return
					adpid == o.adpid &&
					adpoff == o.adpoff &&
					adpstr == o.adpstr;
			}

			uint64_t getMatchStart() const
			{
				return (adpoff >= 0) ? 0 : -adpoff;
			}

			uint64_t getAdapterStart() const
			{
				return (adpoff >= 0) ? adpoff : 0;
			}
		};
		
		struct AdapterOffsetStrandMatchStartComparator
		{
			bool operator()(AdapterOffsetStrand const & A, AdapterOffsetStrand const & B) const
			{
				return A.getMatchStart() < B.getMatchStart();	
			}
		};
	}
}
#endif
