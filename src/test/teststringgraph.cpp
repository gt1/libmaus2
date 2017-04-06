/*
    libmaus2
    Copyright (C) 2017 German Tischler

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
#include <libmaus2/dazzler/stringgraph/StringGraph.hpp>
#include <libmaus2/util/ArgParser.hpp>
#include <libmaus2/fastx/FastAReader.hpp>

int main(int argc, char * argv[])
{
	try
	{
		libmaus2::util::ArgParser const arg(argc,argv);

		std::string const dbfn = arg[0];
		std::string const sgfn = arg[1];

		#if defined(STRING_GRAPH_DEBUG)
		std::string const textfn = arg[2];

		libmaus2::fastx::FastAReader FA(textfn);
		std::vector<std::string> Vtext;
		libmaus2::fastx::FastAReader::pattern_type pattern;
		libmaus2::util::ToUpperTable touppertable;
		while ( FA.getNextPatternUnlocked(pattern) )
		{
			pattern.toupper(touppertable);
			Vtext.push_back(pattern.spattern);
		}
		#endif

		libmaus2::dazzler::db::DatabaseFile DB(dbfn);
		DB.computeTrimVector();
		libmaus2::dazzler::stringgraph::StringGraph::unique_ptr_type SG(libmaus2::dazzler::stringgraph::StringGraph::load(sgfn));

		for ( uint64_t i = 0; i < SG->size(); ++i )
			std::cout << ">edge_" << i << "\n" << SG->traverse(i,DB
				#if defined(STRING_GRAPH_DEBUG)
				,Vtext
				#endif
			) << "\n";
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
