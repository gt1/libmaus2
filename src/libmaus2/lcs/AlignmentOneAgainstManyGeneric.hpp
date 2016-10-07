/*
    libmaus2
    Copyright (C) 2016 German Tischler

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
#if ! defined(LIBMAUS2_LCS_ALIGNMENTONEAGAINSTMANYGENERIC_HPP)
#define LIBMAUS2_LCS_ALIGNMENTONEAGAINSTMANYGENERIC_HPP

#include <libmaus2/lcs/AlignmentOneAgainstManyInterface.hpp>
#include <libmaus2/lcs/NP.hpp>

namespace libmaus2
{
	namespace lcs
	{
		struct AlignmentOneAgainstManyGeneric : public libmaus2::lcs::AlignmentOneAgainstManyInterface
		{
			libmaus2::lcs::NP np;

			void process(
				uint8_t const * qa, uint8_t const * qe,
				std::pair<uint8_t const *,uint64_t> const * MA,uint64_t const MAo,
				libmaus2::autoarray::AutoArray<uint64_t> & E
			)
			{
				for ( uint64_t i = 0; i < MAo; ++i )
				{
					np.align(qa,qe-qa,MA[i].first,MA[i].second);
					E [ i ] = np.getTraceContainer().getAlignmentStatistics().getEditDistance();
				}
			}
		};
	}
}
#endif
