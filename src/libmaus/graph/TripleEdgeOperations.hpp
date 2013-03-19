/**
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
**/

#if ! defined(TRIPLEEDGEOPERATIONS_HPP)
#define TRIPLEEDGEOPERATIONS_HPP

#include <libmaus/types/types.hpp>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/graph/TripleEdge.hpp>
#include <libmaus/graph/TripleEdgeInput.hpp>
#include <libmaus/graph/TripleEdgeOutputMerge.hpp>
#include <libmaus/util/GetFileSize.hpp>
#include <string>
#include <vector>
#include <fstream>
#include <cassert>

namespace libmaus
{
	namespace graph
	{
		struct TripleEdgeOperations
		{
			static void sortFile(std::string const & filename)
			{
				uint64_t const len = ::libmaus::util::GetFileSize::getFileSize(filename);
				assert ( len % sizeof(::libmaus::graph::TripleEdge) == 0 );
				uint64_t const numtriples = len / sizeof(::libmaus::graph::TripleEdge);
				std::ifstream istr(filename.c_str(), std::ios::binary);
				::libmaus::autoarray::AutoArray< ::libmaus::graph::TripleEdge> T(numtriples);
				istr.read ( reinterpret_cast<char *>(T.get()), len);
				assert ( istr );
				assert ( istr.gcount() == static_cast<int64_t>(len) );
				istr.close(); 
				
				std::sort ( T.get(), T.get() + numtriples );

				std::ofstream ostr(filename.c_str(), std::ios::binary);
				ostr.write( reinterpret_cast<char const *>(T.get()), len );
				assert ( ostr );
				ostr.flush();
				assert ( ostr );
				ostr.close();
			}

			static void mergeFiles(
				std::string const & filenamea,
				std::string const & filenameb,
				std::string const & outputfilename
				)
			{
				::libmaus::graph::TripleEdgeInput inputa(filenamea,32*1024);
				::libmaus::graph::TripleEdgeInput inputb(filenameb,32*1024);
				::libmaus::graph::TripleEdgeOutputMerge output(outputfilename, 32*1024);

				::libmaus::graph::TripleEdge triplea;
				::libmaus::graph::TripleEdge tripleb;
				bool oka = inputa.getNextTriple(triplea);
				bool okb = inputb.getNextTriple(tripleb);

				while ( oka && okb )
				{
					if ( triplea < tripleb )
					{
						output.write(triplea);
						oka = inputa.getNextTriple(triplea);
					}
					else
					{
						output.write(tripleb);
						okb = inputb.getNextTriple(tripleb);
					}
				}

				while ( oka )
				{
					output.write(triplea);
					oka = inputa.getNextTriple(triplea);
				}

				while ( okb )
				{
					output.write(tripleb);
					okb = inputb.getNextTriple(tripleb);
				}
			}
			
			static inline bool anyTrue(::libmaus::autoarray::AutoArray < bool > const & ok)
			{
				bool t = false;
				for ( uint64_t i = 0; i < ok.getN(); ++i )
					t = t || ok[i];
				return t;
			}

			static inline uint64_t minOk(
				::libmaus::autoarray::AutoArray < bool > const & ok,
				::libmaus::autoarray::AutoArray < ::libmaus::graph::TripleEdge > const & triples
				)
			{
				assert ( anyTrue ( ok ) );
				
				bool foundok = false;
				uint64_t minidx = 0;
				
				while ( ! foundok )
					if ( ok[minidx] )
						foundok = true;
					else
						minidx++;
						
				assert ( ok[minidx] );
				::libmaus::graph::TripleEdge mintrip = triples[minidx];
				
				for ( uint64_t i = 0; i < triples.getN(); ++i )
					if ( ok[i] && triples[i] < mintrip )
					{
						mintrip = triples[i];
						minidx = i;
					}
					
				return minidx;
			}

			static void multiwayMergeFiles(
				std::vector < std::string > const & inputfilenames,
				std::string const & outputfilename
				)
			{
				typedef ::libmaus::graph::TripleEdgeInput input_type;
				typedef input_type::unique_ptr_type input_ptr_type;
				::libmaus::autoarray::AutoArray<input_ptr_type> inputs(inputfilenames.size());
				
				for ( uint64_t i = 0; i < inputfilenames.size(); ++i )
					inputs[i] = UNIQUE_PTR_MOVE(input_ptr_type (
						new input_type ( inputfilenames[i] , 32*1024 )
						));
						
				::libmaus::autoarray::AutoArray < ::libmaus::graph::TripleEdge > triples(inputfilenames.size());
				::libmaus::autoarray::AutoArray < bool > ok(inputfilenames.size());
			
				::libmaus::graph::TripleEdgeOutputMerge output(outputfilename, 32*1024);

				for ( uint64_t i = 0; i < inputfilenames.size(); ++i )
					ok [i] = inputs[i]->getNextTriple ( triples[i] );

				while ( anyTrue ( ok ) )
				{
					uint64_t const minidx = minOk(ok,triples);
					output.write ( triples[minidx] );
					ok[minidx] = inputs[minidx]->getNextTriple( triples[minidx] );
				}
			}
		};
	}
}
#endif
