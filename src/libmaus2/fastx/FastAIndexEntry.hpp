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
#if ! defined(LIBMAUS2_FASTX_FASTAINDEXENTRY_HPP)
#define LIBMAUS2_FASTX_FASTAINDEXENTRY_HPP

#include <string>
#include <ostream>
#include <libmaus2/types/types.hpp>

namespace libmaus2
{
	namespace fastx
	{
		struct FastAIndexEntry
		{
			std::string name;
			uint64_t length;
			uint64_t offset;
			uint64_t basesperline;
			uint64_t bytesperline;
			
			FastAIndexEntry() : name(), length(0), offset(0), basesperline(0), bytesperline(0)
			{
			
			}
			
			FastAIndexEntry(
				std::string const & rname,
				uint64_t const rlength,
				uint64_t const roffset,
				uint64_t const rbasesperline,
				uint64_t const rbytesperline			
			) : name(rname), length(rlength), offset(roffset), basesperline(rbasesperline), bytesperline(rbytesperline) {}
		};
		
		std::ostream & operator<<(std::ostream & out, FastAIndexEntry const & entry);
	}
}
#endif
