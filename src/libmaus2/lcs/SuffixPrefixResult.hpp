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

#if ! defined(SUFFIXPREFIXRESULT_HPP)
#define SUFFIXPREFIXRESULT_HPP

#include <libmaus2/types/types.hpp>
#include <ostream>

namespace libmaus2
{
	namespace lcs
	{
		struct SuffixPrefixResult
		{
			uint32_t aclip_left;
			uint32_t bclip_right;
			uint64_t numins;
			uint64_t numdel;
			uint64_t nummat;
			uint64_t nummis;

			uint32_t infix_aclip_left;
			uint32_t infix_aclip_right;
			uint64_t infix_numins;
			uint64_t infix_numdel;
			uint64_t infix_nummat;
			uint64_t infix_nummis;
			int64_t  infix_score;

			SuffixPrefixResult()
			: aclip_left(0), bclip_right(0), numins(0), numdel(0), nummat(0), nummis(0),
			  infix_aclip_left(0),
			  infix_aclip_right(0),
			  infix_numins(0),
			  infix_numdel(0),
			  infix_nummat(0),
			  infix_nummis(0),
			  infix_score(-std::numeric_limits<int64_t>::min())
			{}

			SuffixPrefixResult(uint32_t const raclip_left, uint32_t const rbclip_right,
			uint64_t rnumins, uint64_t rnumdel, uint64_t rnummat, uint64_t rnummis)
			: aclip_left(raclip_left), bclip_right(rbclip_right),
			  numins(rnumins), numdel(rnumdel),
			  nummat(rnummat), nummis(rnummis),
			  infix_aclip_left(0),
			  infix_aclip_right(0),
			  infix_numins(0),
			  infix_numdel(0),
			  infix_nummat(0),
			  infix_nummis(0),
			  infix_score(-std::numeric_limits<int64_t>::min())
			{}

			SuffixPrefixResult(
				uint32_t const raclip_left, uint32_t const rbclip_right,
				uint64_t rnumins, uint64_t rnumdel, uint64_t rnummat, uint64_t rnummis,
				uint32_t rinfix_aclip_left,
				uint32_t rinfix_aclip_right,
				uint64_t rinfix_numins,
				uint64_t rinfix_numdel,
				uint64_t rinfix_nummat,
				uint64_t rinfix_nummis,
				int64_t  rinfix_score
			)
			: aclip_left(raclip_left), bclip_right(rbclip_right),
			  numins(rnumins), numdel(rnumdel),
			  nummat(rnummat), nummis(rnummis),
			  infix_aclip_left(rinfix_aclip_left),
			  infix_aclip_right(rinfix_aclip_right),
			  infix_numins(rinfix_numins),
			  infix_numdel(rinfix_numdel),
			  infix_nummat(rinfix_nummat),
			  infix_nummis(rinfix_nummis),
			  infix_score(rinfix_score)
			{}
		};

		inline std::ostream & operator<<(std::ostream & out, SuffixPrefixResult const & SPR)
		{
		        out << "SuffixPrefixResult("
		                << "aclip_left=" << SPR.aclip_left << ","
		                << "bclip_right=" << SPR.bclip_right << ","
		                << "numins=" << SPR.numins << ","
		                << "numdel=" << SPR.numdel << ","
		                << "nummat=" << SPR.nummat << ","
		                << "nummis=" << SPR.nummis << ","
		                << "infix_aclip_left=" << SPR.infix_aclip_left << ","
		                << "infix_aclip_right=" << SPR.infix_aclip_right << ","
		                << "infix_numins=" << SPR.infix_numins << ","
		                << "infix_numdel=" << SPR.infix_numdel << ","
		                << "infix_nummat=" << SPR.infix_nummat << ","
		                << "infix_nummis=" << SPR.infix_nummis << ","
		                << "infix_score=" << SPR.infix_score // << ","
		                << ")";
                        return out;
		}
	}
}
#endif
