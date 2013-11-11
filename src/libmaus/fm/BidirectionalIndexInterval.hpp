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
#if ! defined(LIBMAUS_SUFFIXSORT_BIINTERVAL_HPP)
#define LIBMAUS_SUFFIXSORT_BIINTERVAL_HPP

#include <libmaus/types/types.hpp>
#include <ostream>

namespace libmaus
{
	namespace fm
	{
		struct BidirectionalIndexInterval
		{
			uint64_t spf;
			uint64_t spr;
			uint64_t siz;
			
			BidirectionalIndexInterval() {}
			BidirectionalIndexInterval(uint64_t const rspf, uint64_t const rspr, uint64_t const rsiz)
			: spf(rspf), spr(rspr), siz(rsiz) {}
			
			BidirectionalIndexInterval swap() const
			{
				return BidirectionalIndexInterval(spr,spf,siz);
			}
			void swapInPlace()
			{
				std::swap(spf,spr);
			}
			
			bool operator==(BidirectionalIndexInterval const & o) const
			{
				return 
					(siz == o.siz)
					&&
					(
						(!siz)
						||
						(spf==o.spf && spr == o.spr)
					);
			}
			bool operator!=(BidirectionalIndexInterval const & o) const
			{
				return ! (*this==o);
			}
			
			bool operator<(BidirectionalIndexInterval const & o) const
			{
				return spf < o.spf;
			}
		};

		std::ostream & operator<<(std::ostream & out, BidirectionalIndexInterval const & BI);
	}
}

#endif
