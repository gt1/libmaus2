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
#if ! defined(LIBMAUS_LCS_METAEDITDISTANCE_HPP)
#define LIBMAUS_LCS_METAEDITDISTANCE_HPP

#include <libmaus/lcs/EditDistance.hpp>
#include <libmaus/lcs/BandedEditDistance.hpp>

namespace libmaus
{
	namespace lcs
	{		
		struct MetaEditDistance : public EditDistanceTraceContainer
		{
			typedef MetaEditDistance this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			::libmaus::lcs::EditDistance E;
			::libmaus::lcs::BandedEditDistance BE;
			
			MetaEditDistance()
			{
			}
			
			template<typename iterator_a, typename iterator_b>
			EditDistanceResult process(
				iterator_a aa,
				uint64_t const rn,
				iterator_b bb,
				uint64_t const rm,
				uint64_t const rk,
				similarity_type const gain_match = 1,
				similarity_type const penalty_subst = 1,
				similarity_type const penalty_ins = 1,
				similarity_type const penalty_del = 1
			)
			{
				if ( ::libmaus::lcs::BandedEditDistance::validParameters(rn,rm,rk) )
				{
					EditDistanceResult const R = BE.process(aa,rn,bb,rm,rk,gain_match,penalty_subst,penalty_ins,penalty_del);
					ta = BE.ta;
					te = BE.te;
					return R;
				}
				else
				{
					EditDistanceResult const R = E.process(aa,rn,bb,rm,rk,gain_match,penalty_subst,penalty_ins,penalty_del);
					ta = E.ta;
					te = E.te;
					return R;			
				}
			}
		};
	}
}
#endif
