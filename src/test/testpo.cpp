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
#include <libmaus2/graph/POGraph.hpp>
#include <libmaus2/util/ArgParser.hpp>

int main(int argc, char * argv[])
{
	try
	{
		libmaus2::util::ArgParser const arg(argc,argv);

		libmaus2::graph::POHashGraph G;

		#if 0
		G.insertEdge(0,1);
		G.insertEdge(0,2);
		G.insertEdge(1,2);
		G.insertEdge(2,3);
		G.insertEdge(3,4);
		std::cerr << G;
		assert ( G.haveForwardEdge(0,1) );
		assert ( G.haveForwardEdge(0,2) );
		assert ( G.haveForwardEdge(1,2) );
		assert ( G.haveForwardEdge(2,3) );
		assert ( G.haveForwardEdge(3,4) );
		assert ( !G.haveForwardEdge(2,4) );
		std::cerr << G.getMaxTo() << std::endl;
		std::cerr << G.getMaxFrom() << std::endl;

		// G.toDot(std::cout);

		G.clear();
		G.insertEdge(1,1);
		G.insertEdge(1,2);
		G.insertEdge(2,2);
		G.insertEdge(3,3);
		G.insertEdge(4,4);
		std::cerr << G;
		assert ( G.haveForwardEdge(1,1) );
		assert ( G.haveForwardEdge(1,2) );
		assert ( G.haveForwardEdge(2,2) );
		assert ( G.haveForwardEdge(3,3) );
		assert ( G.haveForwardEdge(4,4) );
		assert ( !G.haveForwardEdge(2,4) );
		std::cerr << G.getMaxTo() << std::endl;
		std::cerr << G.getMaxFrom() << std::endl;
		#endif

		#if 0
		std::string const text = "PKMIVRPQKNETV";
		G.setupSingle(text.begin(),text.size());
		#if 0
		G.toDot(std::cout);
		#endif

		std::string const query = "THKMLVRNETIM";
		G.align(query.begin(),query.size());

		std::cerr << "text = " << text << std::endl;
		std::cerr << "query= " << query << std::endl;

		G.toDot(std::cout);
		#endif

		std::string q0 = "PKMIVRPQKNETG";
		std::string q1 = "ALVRPQKNTRM";
		std::string q2 = "THKMLVRNETAM";

		G.setupSingle(q0.begin(),q0.size());
		G.align(q1.begin(),q1.size());
		G.align(q2.begin(),q2.size());

		// G.toDot(std::cout);

		libmaus2::graph::POHashGraph PA;
		PA.setupSingle(q0.begin(),q0.size());
		libmaus2::graph::POHashGraph PB;
		PB.setupSingle(q1.begin(),q1.size());
		libmaus2::graph::POHashGraph PC;
		PC.setupSingle(q2.begin(),q2.size());

		libmaus2::graph::POHashGraph C;
		libmaus2::graph::POHashGraph D;

		libmaus2::graph::POHashGraph::align(C,PA,PB);
		libmaus2::graph::POHashGraph::align(D,C,PC);

		D.toDot(std::cout);
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
