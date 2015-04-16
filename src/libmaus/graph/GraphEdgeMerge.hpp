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
#if ! defined(LIBMAUS_GRAPH_GRAPHEDGEMERGE_HPP)
#define LIBMAUS_GRAPH_GRAPHEDGEMERGE_HPP

#include <libmaus/aio/SynchronousGenericInput.hpp>
#include <libmaus/graph/GraphEdgeHeapComparator.hpp>
#include <libmaus/graph/GraphEdgeBuffer.hpp>
#include <queue>

namespace libmaus
{
	namespace graph
	{
		struct GraphEdgeMerge
		{
			typedef libmaus::aio::SynchronousGenericInput<GraphEdge> input_type;
			typedef input_type::unique_ptr_type input_ptr_type;
			
			std::vector < uint64_t > blocksizes;
			libmaus::autoarray::AutoArray<input_ptr_type> inputstreams;
			std::priority_queue < 
				std::pair<uint64_t,GraphEdge>,
				std::vector< std::pair<uint64_t,GraphEdge> >,
				GraphEdgeHeapComparator
			> Q;
			
			GraphEdgeMerge(
				std::vector < uint64_t > const & BS,
				std::string const & edgefilename
			) : blocksizes(BS)
			{
				inputstreams = libmaus::autoarray::AutoArray<input_ptr_type>(blocksizes.size());
				uint64_t acc = 0;
				
				for ( uint64_t i = 0; i < blocksizes.size(); ++i )
				{
					inputstreams[i] = UNIQUE_PTR_MOVE(
						input_ptr_type(new input_type(edgefilename,64*1024,acc,blocksizes[i]))
					);

					acc += blocksizes[i];		
				}
				
				for ( uint64_t i = 0; i < blocksizes.size(); ++i )
				{
					GraphEdge ge;
					if ( inputstreams[i]->getNext(ge) )
						Q.push(std::pair<uint64_t,GraphEdge>(i,ge));
				}
			}
			
			bool getNext(GraphEdge & edge)
			{
				if ( Q.empty() )
					return false;
				
				edge = Q.top().second;	
				uint64_t const i = Q.top().first;
				Q.pop();
				
				// std::cerr << "i=" << i << std::endl;
				
				GraphEdge ge;
				if ( inputstreams[i]->getNext(ge) )
					Q.push(std::pair<uint64_t,GraphEdge>(i,ge));
					
				return true;                                                                        
			}
			
			bool peek(GraphEdge & edge)
			{
				if ( Q.empty() )
					return false;
				else
				{
					edge = Q.top().second;
					return true;
				}
			}
			
			bool nextIs(uint64_t const s) const
			{
				if ( Q.empty() )
					return false;
				else
					return Q.top().second.s == s;
			}

			static void merge(
				std::vector < uint64_t > const & BS,
				std::string const & edgefilename,
				std::string const & outputfilename
			)
			{
				GraphEdgeMerge GEM(BS,edgefilename);
				GraphEdgeBuffer GEB(outputfilename,64*1024);
				GraphEdge ge;
				while ( GEM.getNext(ge) )
					GEB.put(ge);
				GEB.flush();
			}
		};
	}
}
#endif
