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
#if ! defined(LIBMAUS_BAMBAM_CHROMOSOME_HPP)
#define LIBMAUS_BAMBAM_CHROMOSOME_HPP

#include <string>
#include <libmaus/types/types.hpp>
#include <libmaus/util/unordered_map.hpp>

namespace libmaus
{
	namespace bambam
	{
		struct Chromosome
		{
			std::string name;
			uint64_t len;
			::libmaus::util::unordered_map<std::string,std::string>::type M;
				
			Chromosome()
			{}
				
			Chromosome(std::string const & rname, uint64_t const rlen) : name(rname), len(rlen)
			{}
		};
	}
}
#endif
