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
#if ! defined(LIBMAUS2_LCS_ENVELOPEFRAGMENT_HPP)
#define LIBMAUS2_LCS_ENVELOPEFRAGMENT_HPP

#include <libmaus2/types/types.hpp>
#include <algorithm>
#include <ostream>

namespace libmaus2
{
	namespace lcs
	{
		struct EnvelopeFragment
		{
			typedef int32_t coordinate_type;

			coordinate_type x;
			coordinate_type y;
			coordinate_type k;
			coordinate_type id;

			EnvelopeFragment(coordinate_type rx = 0, coordinate_type ry = 0, coordinate_type rk = 0, coordinate_type rid = 0) : x(rx), y(ry), k(rk), id(rid)
			{

			}

			EnvelopeFragment getBackFragment() const
			{
				return EnvelopeFragment(x+k,y+k,k,id);
			}

			EnvelopeFragment getFrontFragmentFromBack() const
			{
				return EnvelopeFragment(x-k,y-k,k,id);
			}

			coordinate_type getDiagonal() const
			{
				return x-y;
			}

			coordinate_type getEnd() const
			{
				return y + k;
			}

			bool operator<(EnvelopeFragment const & o) const
			{
				if ( y != o.y )
					return y < o.y;
				else if ( x != o.x )
					return x < o.x;
				else
					return k < o.k;
			}

			struct EnvelopeFragmentDiagComparator
			{
				bool operator()(EnvelopeFragment const & A, EnvelopeFragment const & B) const
				{
					if ( A.getDiagonal() != B.getDiagonal() )
						return A.getDiagonal() < B.getDiagonal();
					else
						return A.y < B.y;
				}
			};

			template<typename const_iterator>
			struct EnvelopeFragmentBackComparator
			{
				bool operator()(const_iterator const & i, const_iterator const & j) const
				{
					return i->getEnd() < j->getEnd();
				}
			};

			// caveat: this is not guaranteed to work for fragments with differing k values
			template<typename iterator>
			static iterator mergeOverlappingAndSort(iterator rlow, iterator rtop)
			{
				// sort range by diagonal and then y
				std::sort(rlow,rtop,EnvelopeFragmentDiagComparator());

				iterator ilow = rlow;
				iterator itop = rtop;
				iterator iout = rlow;
				while ( ilow != itop )
				{
					// find end of same diagonal
					iterator ihigh = ilow+1;
					while ( ihigh != itop && ilow->getDiagonal() == ihigh->getDiagonal() )
						++ihigh;

					iterator jlow = ilow;
					while ( jlow != ihigh )
					{
						int64_t prevend = jlow->getEnd();
						iterator jhigh = jlow+1;
						while ( jhigh < ihigh && prevend >= jhigh->y )
						{
							prevend = jhigh->getEnd();
							++jhigh;
						}

						int64_t const start = jlow->y;
						int64_t const end = prevend;
						int64_t const len = end-start;

						*(iout++) = EnvelopeFragment(jlow->x,jlow->y,len);

						jlow = jhigh;
					}

					ilow = ihigh;
				}

				// sort by y coordinate
				std::sort(rlow,iout);

				// return end of range
				return iout;
			}

		};

		struct EnvelopeFragmentDiagonalComparator
		{
			bool operator()(EnvelopeFragment const & A, EnvelopeFragment const & B) const
			{
				if ( A.getDiagonal() != B.getDiagonal() )
					return A.getDiagonal() < B.getDiagonal();
				else
					return A.y < B.y;
			}
		};

		std::ostream & operator<<(std::ostream & out, EnvelopeFragment const & fragment);
	}
}
#endif
