/*
    libmaus2
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
#if ! defined(LIBMAUS2_LZ_SIMPLECOMPRESSEDSTREAMINTERVAL_HPP)
#define LIBMAUS2_LZ_SIMPLECOMPRESSEDSTREAMINTERVAL_HPP

#include <map>
#include <libmaus2/types/types.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct SimpleCompressedStreamInterval
		{
			std::pair<uint64_t,uint64_t> start;
			std::pair<uint64_t,uint64_t> end;

			SimpleCompressedStreamInterval() : start(0,0), end(0,0) {}
			SimpleCompressedStreamInterval(
				std::pair<uint64_t,uint64_t> const & rstart,
				std::pair<uint64_t,uint64_t> const & rend
			) : start(rstart), end(rend)
			{

			}
		};
	}
}
#endif
