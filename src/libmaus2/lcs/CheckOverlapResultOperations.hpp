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
#if ! defined(LIBMAUS_LCS_CHECKOVERLAPRESULTOPERATIONS_HPP)
#define LIBMAUS_LCS_CHECKOVERLAPRESULTOPERATIONS_HPP

#include <libmaus2/lcs/CheckOverlapResult.hpp>
#include <libmaus2/lcs/CheckOverlapScoreComparatorGreater.hpp>

namespace libmaus2
{
	namespace lcs
	{
		struct CheckOverlapResultOperations
		{
			static void splitLeftRightOther(
				std::vector< ::libmaus2::lcs::CheckOverlapResult::shared_ptr_type> const & all,
				std::vector< ::libmaus2::lcs::CheckOverlapResult::shared_ptr_type> & left,
				std::vector< ::libmaus2::lcs::CheckOverlapResult::shared_ptr_type> & right,
				std::vector< ::libmaus2::lcs::CheckOverlapResult::shared_ptr_type> & a_covers_b,
				std::vector< ::libmaus2::lcs::CheckOverlapResult::shared_ptr_type> & b_covers_a,
				std::vector< ::libmaus2::lcs::CheckOverlapResult::shared_ptr_type> & other,
				uint64_t minused = 0
				)
			{
				left.clear();
				right.clear();
				a_covers_b.clear();
				b_covers_a.clear();
				other.clear();

				for ( uint64_t i = 0; i < all.size(); ++i )
				{
					::libmaus2::lcs::CheckOverlapResult::shared_ptr_type COR = all[i];
					::libmaus2::lcs::OverlapOrientation::overlap_orientation const orientation = COR->orientation;

					if ( ::libmaus2::lcs::OverlapOrientation::isRightEdge(orientation) )
					{
						if ( COR->getUsedA() >= minused && COR->getUsedB() >= minused )
							right.push_back(all[i]);
					}
					else if ( ::libmaus2::lcs::OverlapOrientation::isLeftEdge(orientation) )
					{
						if ( COR->getUsedA() >= minused && COR->getUsedB() >= minused )
							left.push_back(all[i]);
					}
					else
					{
						switch ( orientation )
						{
							case ::libmaus2::lcs::OverlapOrientation::overlap_a_covers_b:
							case ::libmaus2::lcs::OverlapOrientation::overlap_ar_covers_b:
								a_covers_b.push_back(all[i]);
								break;
							case ::libmaus2::lcs::OverlapOrientation::overlap_b_covers_a:
							case ::libmaus2::lcs::OverlapOrientation::overlap_b_covers_ar:
								b_covers_a.push_back(all[i]);
								break;
							default: 
								other.push_back(all[i]);
								break;
						}
					}
				}
				
				::libmaus2::lcs::CheckOverlapScoreComparatorGreater scorecmp;
				std::sort(left.begin(),left.end(),scorecmp);
				std::sort(right.begin(),right.end(),scorecmp);
				std::sort(a_covers_b.begin(),a_covers_b.end(),scorecmp);
				std::sort(b_covers_a.begin(),b_covers_a.end(),scorecmp);
				std::sort(other.begin(),other.end(),scorecmp);
			}
		};
	}
}
#endif
