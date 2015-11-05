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

#if ! defined(TRIPLEEDGEOUTPUTMERGE_HPP)
#define TRIPLEEDGEOUTPUTMERGE_HPP

#include <libmaus2/graph/TripleEdgeOutput.hpp>

namespace libmaus2
{
	namespace graph
	{
		struct TripleEdgeOutputMerge : public ::libmaus2::graph::TripleEdgeOutput
		{
			private:
			typedef ::libmaus2::graph::TripleEdgeOutput base_type;

			::libmaus2::graph::TripleEdge prevtrip;

			void flush()
			{
				if ( prevtrip.a != prevtrip.b )
				{
					base_type::write(prevtrip);
					prevtrip = ::libmaus2::graph::TripleEdge(0,0,0);
				}
				base_type::flush();
			}

			public:
			TripleEdgeOutputMerge(std::string const & rfilename, uint64_t const bufsize)
			: base_type(rfilename,bufsize)
			{

			}

			~TripleEdgeOutputMerge()
			{
				flush();
			}

			void write(::libmaus2::graph::TripleEdge const & T)
			{
				if ( (T.a != prevtrip.a) || (T.b != prevtrip.b) )
				{
					if ( prevtrip.a != prevtrip.b )
						base_type::write(prevtrip);

					prevtrip = T;
				}
				else
				{
					prevtrip.c += T.c;
				}
			}
		};
	}
}
#endif
