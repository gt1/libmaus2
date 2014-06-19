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
		template<
			libmaus::lcs::edit_distance_priority_type _edit_distance_priority = ::libmaus::lcs::del_ins_diag
		>
		struct MetaEditDistance : public EditDistanceTraceContainer
		{
			static ::libmaus::lcs::edit_distance_priority_type const edit_distance_priority = _edit_distance_priority;
			typedef MetaEditDistance<edit_distance_priority> this_type;
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			::libmaus::lcs::EditDistance<edit_distance_priority> E;
			::libmaus::lcs::BandedEditDistance<edit_distance_priority> BE;
			
			typedef typename ::libmaus::lcs::EditDistance<edit_distance_priority>::result_type result_type;
			
			MetaEditDistance()
			{
			}
			
			template<typename iterator_a, typename iterator_b>
			result_type process(
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
				if ( ::libmaus::lcs::BandedEditDistance<edit_distance_priority>::validParameters(rn,rm,rk) )
				{
					result_type const R = BE.process(aa,rn,bb,rm,rk,gain_match,penalty_subst,penalty_ins,penalty_del);
					ta = BE.ta;
					te = BE.te;
					return R;
				}
				else
				{
					result_type const R = E.process(aa,rn,bb,rm,rk,gain_match,penalty_subst,penalty_ins,penalty_del);
					ta = E.ta;
					te = E.te;
					return R;			
				}
			}
		};
	}
}
#endif
