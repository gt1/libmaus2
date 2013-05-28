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


#if ! defined(LIBMAUS_LCS_OVERLAPORIENTATION_HPP)
#define LIBMAUS_LCS_OVERLAPORIENTATION_HPP

#include <libmaus/exception/LibMausException.hpp>

namespace libmaus
{
	namespace lcs
	{
		struct OverlapOrientation
		{
			enum overlap_orientation 
			{
				overlap_cover_complete = 0,
				overlap_a_back_dovetail_b_front   = 1,
				overlap_a_front_dovetail_b_back   = 2,
				overlap_a_front_dovetail_b_front  = 3,
				overlap_a_back_dovetail_b_back   = 4,
				overlap_a_covers_b     = 5,
				overlap_b_covers_a     = 6,
				overlap_ar_covers_b    = 7,
				overlap_b_covers_ar    = 8
			};
			
			static overlap_orientation getInverse(overlap_orientation const o)
			{
				switch ( o )
				{
					case overlap_cover_complete:
						return overlap_cover_complete;

					//
					case overlap_a_back_dovetail_b_front:
						return overlap_a_front_dovetail_b_back;
					//
					case overlap_a_front_dovetail_b_back:
						return overlap_a_back_dovetail_b_front;
						
					//
					case overlap_a_front_dovetail_b_front:
						return overlap_a_front_dovetail_b_front;
					//
					case overlap_a_back_dovetail_b_back:
						return overlap_a_back_dovetail_b_back;
					//
					case overlap_a_covers_b:
						return overlap_b_covers_a;
					case overlap_b_covers_a:
						return overlap_a_covers_b;
					case overlap_ar_covers_b:
						return overlap_b_covers_ar;
					case overlap_b_covers_ar:
						return overlap_ar_covers_b;
					default:
					{
						::libmaus::exception::LibMausException se;
						se.getStream() << "Orientation " << o << " has no inverse." << std::endl;
						se.finish();
						throw se;
					}
				}
			}
			
			static bool isRightEdge(overlap_orientation const & o)
			{
				switch ( o )
				{
					case overlap_a_back_dovetail_b_front:
					case overlap_a_back_dovetail_b_back:
						return true;
					default:
						return false;
				}
			}
			static bool isLeftEdge(overlap_orientation const & o)
			{
				switch ( o )
				{
					case overlap_a_front_dovetail_b_back:
					case overlap_a_front_dovetail_b_front:
						return true;
					default:
						return false;
				}
			}
			static int leftRightOther(overlap_orientation const & o)
			{
				if ( isRightEdge(o) )
					return 0;
				else if ( isLeftEdge(o) )
					return 1;
				else
					return 2;
			}
		};
		
		struct OverlapOrientationOrder
		{
			bool operator()(OverlapOrientation::overlap_orientation const a, OverlapOrientation::overlap_orientation const b) const
			{
				int const ia = OverlapOrientation::leftRightOther(a);
				int const ib = OverlapOrientation::leftRightOther(b);
				
				if ( ia != ib )
					return ia < ib;
					
				return static_cast<int>(a) < static_cast<int>(b);
			}
		};

		inline std::ostream & operator<<(std::ostream & out, OverlapOrientation::overlap_orientation const & o)
		{
			switch ( o )
			{
				case OverlapOrientation::overlap_cover_complete: out << "overlap_cover_complete"; break;
				case OverlapOrientation::overlap_a_back_dovetail_b_front: out << "overlap_a_back_dovetail_b_front"; break;
				case OverlapOrientation::overlap_a_front_dovetail_b_back: out << "overlap_a_front_dovetail_b_back"; break;
				case OverlapOrientation::overlap_a_front_dovetail_b_front: out << "overlap_a_front_dovetail_b_front"; break;
				case OverlapOrientation::overlap_a_back_dovetail_b_back: out << "overlap_a_back_dovetail_b_back"; break;
				case OverlapOrientation::overlap_a_covers_b: out << "overlap_a_covers_b"; break;
				case OverlapOrientation::overlap_b_covers_a: out << "overlap_b_covers_a"; break;
				case OverlapOrientation::overlap_ar_covers_b: out << "overlap_ar_covers_b"; break;
				case OverlapOrientation::overlap_b_covers_ar: out << "overlap_b_covers_ar"; break;
			}
			return out;
		}

	}
}
#endif
