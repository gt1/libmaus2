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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_TRACEPOINT_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_TRACEPOINT_HPP

#include <libmaus2/types/types.hpp>
#include <ostream>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct TracePoint
			{
				int64_t apos;
				int64_t bpos;
				int64_t id;
				
				TracePoint() {}
				TracePoint(
					int64_t const rapos,
					int64_t const rbpos,
					int64_t const rid	
				) : apos(rapos), bpos(rbpos),id(rid)
				{
				
				}

				bool operator<(TracePoint const & T) const
				{
					if ( apos != T.apos )
						return apos < T.apos;
					else if ( bpos != T.bpos )
						return bpos < T.bpos;
					else
						return id < T.id;
				}
			};

			std::ostream & operator<<(std::ostream & out, TracePoint const & TP);
		}
	}
}
#endif
