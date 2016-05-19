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
#if !defined(LIBMAUS2_RANK_DNARANKBIDIRRANGE_HPP)
#define LIBMAUS2_RANK_DNARANKBIDIRRANGE_HPP

#include <libmaus2/types/types.hpp>
#include <ostream>

namespace libmaus2
{
	namespace rank
	{
		struct DNARankBiDirRange
		{
			uint64_t forw;
			uint64_t reco;
			uint64_t size;

			void set(uint64_t const rforw, uint64_t rreco, uint64_t const rsize)
			{
				forw = rforw;
				reco = rreco;
				size = rsize;
			}

			bool operator==(DNARankBiDirRange const & O) const
			{
				return forw == O.forw && reco == O.reco && size == O.size;
			}

			bool operator<(DNARankBiDirRange const & O) const
			{
				if ( forw != O.forw )
					return forw < O.forw;
				else if ( reco != O.reco )
					return reco < O.reco;
				else
					return size < O.size;
			}
		};

		std::ostream & operator<<(std::ostream & out, DNARankBiDirRange const & B);
	}
}
#endif
