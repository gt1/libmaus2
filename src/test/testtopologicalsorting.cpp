
/*
    libmaus
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#include <libmaus/graph/TopologicalSorting.hpp>
#include <libmaus/graph/IdentityTargetProjector.hpp>

#include <iostream>

void testTopologicalSorting(std::map< uint64_t,std::vector<uint64_t> > const & edges)
{
	std::cout << "digraph {\n";
	
	for ( std::map< uint64_t,std::vector<uint64_t> >::const_iterator ita = edges.begin(); ita != edges.end(); ++ita )
	{
		std::vector<uint64_t> const & V = ita->second;
		for ( uint64_t i = 0; i < V.size(); ++i )
			std::cout << ita->first << " -> " << V[i] << "\n";
	}
	
	std::cout << "}\n";

	std::pair<bool,std::map<uint64_t,uint64_t> > TS = libmaus::graph::TopologicalSorting::topologicalSorting<uint64_t,libmaus::graph::IdentityTargetProjector>(edges,0);
	
	if ( TS.first )
	{
		std::cerr << "Sorting:" << std::endl;
		
		for ( std::map<uint64_t,uint64_t>::const_iterator ita = TS.second.begin(); ita != TS.second.end(); ++ita )
		{
			std::cerr << ita->first << "\t" << ita->second << std::endl;
		}
	}
	else
	{
		std::cerr << "Graph has no topological sorting" << std::endl;
	}
}

void testTopologicalSortingCyclic()
{
	std::map< uint64_t,std::vector<uint64_t> > edges;

	edges[0].push_back(1);
	edges[1].push_back(2);
	edges[2].push_back(3);
	edges[3].push_back(0);
	edges[2].push_back(4);
	edges[4].push_back(2);
	edges[3].push_back(4);
	
	edges[4].push_back(5);

	edges[5].push_back(6);
	edges[6].push_back(5);
	edges[5].push_back(9);
	edges[9].push_back(5);
	
	edges[6].push_back(7);
	edges[7].push_back(8);
	edges[8].push_back(9);
	edges[9].push_back(6);
	edges[9].push_back(10);
	edges[10].push_back(9);

	testTopologicalSorting(edges);	
}

void testTopologicalSortingAcyclic()
{
	std::map< uint64_t,std::vector<uint64_t> > edges;

	edges[0].push_back(1);
	edges[1].push_back(2);
	edges[0].push_back(3);
	edges[1].push_back(4);

	testTopologicalSorting(edges);	
}

int main()
{
	try
	{
		testTopologicalSortingCyclic();
		testTopologicalSortingAcyclic();
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
