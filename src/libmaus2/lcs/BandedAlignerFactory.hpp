/*
    libmaus2
    Copyright (C) 2015 German Tischler

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
#if ! defined(LIBMAUS2_LCS_BANDEDALIGNERFACTORY_HPP)
#define LIBMAUS2_LCS_BANDEDALIGNERFACTORY_HPP

#include <libmaus2/lcs/BandedAligner.hpp>
#include <set>

namespace libmaus2
{
	namespace lcs
	{
		struct BandedAlignerFactory
		{
			enum aligner_type {
				libmaus2_lcs_BandedAlignerFactory_BandedEditDistance,
				libmaus2_lcs_BandedAlignerFactory_x128_8,
				libmaus2_lcs_BandedAlignerFactory_x128_16,
				libmaus2_lcs_BandedAlignerFactory_y256_8,
				libmaus2_lcs_BandedAlignerFactory_y256_16
			};
			
			static libmaus2::lcs::BandedAligner::unique_ptr_type construct(aligner_type const type);
			static std::set<aligner_type> getSupportedAligners();
		};
		
		std::ostream & operator<<(std::ostream & out, BandedAlignerFactory::aligner_type const & A);
	}
}
#endif
