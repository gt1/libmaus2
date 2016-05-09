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
#if ! defined(LIBMAUS2_GAMMA_FLAGGEDINTERVAL_HPP)
#define LIBMAUS2_GAMMA_FLAGGEDINTERVAL_HPP

#include <libmaus2/types/types.hpp>
#include <ostream>

namespace libmaus2
{
	namespace gamma
	{
		struct FlaggedInterval
		{
			enum interval_type {
				interval_type_complete = 0,
				interval_type_start = 1,
				interval_type_middle = 2,
				interval_type_end = 3
			};

			static interval_type getType(
				uint64_t const low,
				uint64_t const high,
				uint64_t const from,
				uint64_t const to
			)
			{
				bool const nooverlap = from >= high || low >= to;

				assert ( ! nooverlap );

				if ( from >= low && to <= high )
					return interval_type_complete;
				else if ( from < low && to > high )
					return interval_type_middle;
				else if ( from < low )
				{
					assert ( to <= high );
					return interval_type_end;
				}
				else
				{
					assert ( from >= low );
					assert ( to > high );
					return interval_type_start;
				}
			}

			static bool mergeable(interval_type left, interval_type right)
			{
				if ( left == interval_type_start )
					return right == interval_type_middle || right == interval_type_end;
				else if ( left == interval_type_middle )
					return right == interval_type_middle || right == interval_type_end;
				else
					return false;
			}

			static interval_type merge(interval_type left, interval_type right)
			{
				assert ( mergeable(left,right) );
				if ( left == interval_type_start )
					// start,middle
					if ( right == interval_type_middle )
						return interval_type_start;
					// start,end
					else
					{
						assert ( right == interval_type_end );
						return interval_type_complete;
					}
				else // if ( left == interval_type_middle )
				{
					assert ( left == interval_type_middle );
					// middle,middle
					if ( right == interval_type_middle )
						return interval_type_middle;
					// middle,end
					else
					{
						assert ( right == interval_type_end );
						return interval_type_end;
					}
				}
			}

			uint64_t from;
			uint64_t to;
			interval_type type;
			bool active;

			FlaggedInterval()
			{

			}

			FlaggedInterval(uint64_t const rfrom, uint64_t const rto, interval_type const rtype, bool const ractive)
			: from(rfrom), to(rto), type(rtype), active(ractive)
			{

			}

			bool operator==(FlaggedInterval const & P) const
			{
				return
					from == P.from && to == P.to && type == P.type && active == P.active;
			}
		};

		inline std::ostream & operator<<(std::ostream & out, FlaggedInterval::interval_type const I)
		{
			switch ( I )
			{
				case FlaggedInterval::interval_type_complete:
					out << "interval_type_complete";
					break;
				case FlaggedInterval::interval_type_start:
					out << "interval_type_start";
					break;
				case FlaggedInterval::interval_type_end:
					out << "interval_type_end";
					break;
				case FlaggedInterval::interval_type_middle:
					out << "interval_type_middle";
					break;
				default:
					out << "interval_type_unknown";
					break;
			}

			return out;
		}

		inline std::ostream & operator<<(std::ostream & out, FlaggedInterval const & I)
		{
			out << "FlaggedInterval(from=" << I.from << ",to=" << I.to << ",type=" << I.type << ",active=" << I.active << ")";
			return out;
		}
	}
}
#endif
