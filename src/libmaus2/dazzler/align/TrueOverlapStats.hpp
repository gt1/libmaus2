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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_TRUEOVERLAPSTATS_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_TRUEOVERLAPSTATS_HPP

#include <libmaus2/parallel/SynchronousCounter.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct TrueOverlapStats
			{
				typedef TrueOverlapStats this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				libmaus2::parallel::SynchronousCounter<uint64_t> nkept;
				libmaus2::parallel::SynchronousCounter<uint64_t> ndisc;
				libmaus2::parallel::SynchronousCounter<uint64_t> ncom;
				libmaus2::parallel::SynchronousCounter<uint64_t> nuncom;

				TrueOverlapStats()
				: nkept(0), ndisc(0), ncom(0), nuncom(0)
				{

				}
			};

			std::ostream & operator<<(std::ostream & out, TrueOverlapStats const & T);
		}
	}
}

inline std::ostream & libmaus2::dazzler::align::operator<<(std::ostream & out, libmaus2::dazzler::align::TrueOverlapStats const & T)
{
	return out << "TrueOverlapStats(kept=" << T.nkept << ",disc=" << T.ndisc << ",com=" << T.ncom << ",uncom=" << T.nuncom << ")";
}
#endif
