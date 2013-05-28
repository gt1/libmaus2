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
#if ! defined(LIBMAUS_LCS_ORIENTATIONWEIGHTENCODING_HPP)
#define LIBMAUS_LCS_ORIENTATIONWEIGHTENCODING_HPP

#include <libmaus/lcs/OverlapOrientation.hpp>
#include <libmaus/math/MetaLog.hpp>
#include <libmaus/graph/TripleEdge.hpp>
#include <libmaus/math/lowbits.hpp>

namespace libmaus
{
	namespace lcs
	{
		struct OrientationWeightEncoding : public OverlapOrientation
		{
			typedef ::libmaus::graph::TripleEdge edge_type;

			static unsigned int const orientation_bits = 
				::libmaus::math::MetaNumBits<overlap_b_covers_ar>::bits;
			static unsigned int const orientation_mask =
				static_cast<uint64_t>((1ull << orientation_bits)-1ull);
			static unsigned int const orientation_shift = 8*sizeof(edge_type::link_weight_type)-orientation_bits;
			
			static edge_type::link_weight_type addOrientation(
				edge_type::link_weight_type const weight,
				overlap_orientation const orientation
			)
			{
				return 
					weight | (static_cast<edge_type::link_weight_type>(orientation) << orientation_shift);
			}
			static edge_type::link_weight_type removeOrientation(
				edge_type::link_weight_type const weight
			)
			{
				return weight & ::libmaus::math::lowbits(orientation_shift);
			}
			static overlap_orientation getOrientation(
				edge_type::link_weight_type const weight
				)
			{
				switch (weight >> orientation_shift)
				{
					case overlap_cover_complete: return overlap_cover_complete;
					case overlap_a_back_dovetail_b_front: return overlap_a_back_dovetail_b_front;
					case overlap_a_front_dovetail_b_back: return overlap_a_front_dovetail_b_back;
					case overlap_a_front_dovetail_b_front: return overlap_a_front_dovetail_b_front;
					case overlap_a_back_dovetail_b_back: return overlap_a_back_dovetail_b_back;
					// 
					case overlap_a_covers_b: return overlap_a_covers_b;
					case overlap_b_covers_a: return overlap_b_covers_a;
					case overlap_ar_covers_b: return overlap_ar_covers_b;
					case overlap_b_covers_ar: return overlap_b_covers_ar;
					// default: return overlap_a_dovetail_b;
				}
				assert ( false );
			}
		};
	}
}
#endif
