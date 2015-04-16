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
#if ! defined(LIBMAUS_GRAPH_GRAPHEDGEBLOCKBUFFER_HPP)
#define LIBMAUS_GRAPH_GRAPHEDGEBLOCKBUFFER_HPP

#include <libmaus2/graph/GraphEdge.hpp>

namespace libmaus2
{
	namespace graph
	{
		struct GraphEdgeBlockBuffer
		{
			libmaus2::aio::CheckedOutputStream COS;
			libmaus2::autoarray::AutoArray<GraphEdge> B;
			GraphEdge * const pa;
			GraphEdge * pc;
			GraphEdge * const pe;
			std::vector<uint64_t> blocksizes;
			
			GraphEdgeBlockBuffer(std::string const & filename, uint64_t const bufsize)
			: 
				COS(filename),
				B(bufsize,false),
				pa(B.begin()),
				pc(pa),
				pe(B.end())
			{
			
			}
			
			void internalflush()
			{
				if ( pc != pa )
				{
					std::sort(pa,pc);
					COS.write(
						reinterpret_cast<char const *>(pa),
						(pc-pa)*sizeof(GraphEdge)
					);
					blocksizes.push_back(pc-pa);
					pc = pa;
				}
			}
			
			void put(GraphEdge const & ge)
			{
				*(pc++) = ge;
				
				if ( pc == pe )
					internalflush();
			}
			
			void flush()
			{
				internalflush();
				COS.flush();
			}
		};
	}
}
#endif
