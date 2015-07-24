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
#if ! defined(LIBMAUS2_DAZZLER_DB_TRACKANNO_HPP)
#define LIBMAUS2_DAZZLER_DB_TRACKANNO_HPP

#include <libmaus2/dazzler/db/TrackAnnoInterface.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>

namespace libmaus2
{
	namespace dazzler
	{	
		template<typename _element_type>
		struct TrackAnno : public TrackAnnoInterface
		{
			typedef _element_type element_type;
			typedef TrackAnno<element_type> this_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
			
			libmaus2::autoarray::AutoArray<element_type> A;
			
			TrackAnno()
			{
			
			}
			
			TrackAnno(uint64_t const n) : A(n) {}
			
			uint64_t operator[](uint64_t const i) const
			{
				return A[i];
			}
			
			void shift(uint64_t const s)
			{
				for ( uint64_t i = 0; i < A.size(); ++i )
					A[i] -= s;
			}
		};
	}
}
#endif
