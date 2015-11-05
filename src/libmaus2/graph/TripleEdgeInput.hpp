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

#if ! defined(TRIPLEEDGEINPUT_HPP)
#define TRIPLEEDGEINPUT_HPP

#include <libmaus2/graph/TripleEdge.hpp>
#include <fstream>
#include <cassert>
#include <libmaus2/util/unique_ptr.hpp>

namespace libmaus2
{
	namespace graph
	{
		struct TripleEdgeInput
		{
			typedef ::libmaus2::graph::TripleEdgeInput this_type;
			typedef ::libmaus2::util::unique_ptr < this_type > :: type unique_ptr_type;

			uint64_t const bufsize;
			::libmaus2::autoarray::AutoArray< TripleEdge > B;
			libmaus2::aio::InputStreamInstance istr;

			uint64_t curbufleft;
			TripleEdge const * curtrip;

			static uint64_t getFileLength(std::string const & filename)
			{
				libmaus2::aio::InputStreamInstance istr(filename);
				istr.seekg(0,std::ios::end);
				uint64_t const l = istr.tellg();
				return l;
			}

			static uint64_t getNumberOfTriples(std::string const & filename)
			{
				uint64_t const l = getFileLength(filename);
				assert ( l % sizeof(TripleEdge) == 0 );
				return l / sizeof(TripleEdge);
			}

			TripleEdgeInput(std::string const & filename, uint64_t const rbufsize)
			:
				bufsize(rbufsize),
				B(bufsize),
				istr(filename),
				curbufleft(0),
				curtrip(0)
			{

			}
			~TripleEdgeInput()
			{
			}
			bool getNextTriple(TripleEdge & triple)
			{
				if ( ! curbufleft )
				{
					istr.read ( reinterpret_cast<char *>(B.get()), B.getN() * sizeof(TripleEdge) );

					if ( istr.gcount() == 0 )
						return false;

					assert ( istr.gcount() % sizeof(TripleEdge) == 0 );

					curbufleft = istr.gcount() / sizeof(TripleEdge);
					assert ( curbufleft );
					curtrip = B.get();
				}

				triple = *(curtrip++);
				curbufleft -= 1;

				return true;
			}
		};
	}
}
#endif
