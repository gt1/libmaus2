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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_TRACEBLOCKVECTORCACHE_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_TRACEBLOCKVECTORCACHE_HPP

#include <libmaus2/dazzler/align/TraceBlock.hpp>
#include <libmaus2/dazzler/align/Overlap.hpp>
#include <libmaus2/util/shared_ptr.hpp>
#include <vector>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct TraceBlockVectorCache
			{
				typedef std::vector<TraceBlock> vector_type;
				typedef libmaus2::util::shared_ptr<vector_type>::type vector_ptr_type;

				std::map < uint64_t, vector_ptr_type> M;
				std::vector<libmaus2::dazzler::align::Overlap> const & V;
				int64_t const tspace;

				TraceBlockVectorCache(
					std::vector<libmaus2::dazzler::align::Overlap> const & rV,
					int64_t const rtspace
				) : M(), V(rV), tspace(rtspace)
				{

				}

				vector_type const & operator[](uint64_t const i)
				{
					if ( M.find(i) == M.end() )
					{
						vector_type T = V[i].getTraceBlocks(tspace);
						vector_ptr_type P(new vector_type(T));
						M[i] = P;
					}
					assert ( M.find(i) != M.end() );
					return *(M.find(i)->second);
				}
			};
		}
	}
}
#endif
