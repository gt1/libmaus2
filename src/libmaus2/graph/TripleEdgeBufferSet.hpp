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

#if ! defined(TRIPLEEDGEBUFFERSET_HPP)
#define TRIPLEEDGEBUFFERSET_HPP

#include <libmaus2/graph/TripleEdgeBuffer.hpp>
#include <string>

namespace libmaus2
{
	namespace graph
	{
		struct TripleEdgeBufferSet
		{
			uint64_t const numparts;

			typedef ::libmaus2::graph::TripleEdgeBuffer buffer_type;
			typedef ::libmaus2::util::unique_ptr<buffer_type>::type buffer_ptr_type;

			::libmaus2::autoarray::AutoArray< buffer_ptr_type > buffers;

			TripleEdgeBufferSet ( ::libmaus2::util::TempFileNameGenerator & tmpgen, uint64_t const rnumparts)
			: numparts(rnumparts), buffers(numparts)
			{
				for ( uint64_t i = 0; i < numparts; ++i )
				{
					buffer_ptr_type tbuffersi( new buffer_type (tmpgen) );
					buffers[i] = UNIQUE_PTR_MOVE(tbuffersi);
				}
			}

			void registerFileName(uint64_t const part, std::string const & filename)
			{
				// std::cerr << "Registering " << filename << " for part " << part << std::endl;
				buffers [ part ] -> registerFileName(filename);
			}

			std::vector < std::string > flush()
			{
				std::vector < std::string > filenames;
				for ( uint64_t i = 0; i < numparts; ++i )
				{
					std::cerr << "Flushing TripleEdgeBuffer " << i << "/" << numparts << std::endl;
					filenames.push_back ( buffers[i] -> flush() );
					std::cerr << "Flushed TripleEdgeBuffer " << i << "/" << numparts << std::endl;
				}
				return filenames;
			}
		};
	}
}
#endif
