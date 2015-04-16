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
#if ! defined(LIBMAUS2_FASTX_FASTABGZFINDEXENTRY_HPP)
#define LIBMAUS2_FASTX_FASTABGZFINDEXENTRY_HPP

#include <string>
#include <vector>
#include <libmaus2/util/StringSerialisation.hpp>
#include <istream>
#include <ostream>

namespace libmaus2
{
	namespace fastx
	{
		struct FastABgzfIndexEntry
		{
			std::string name;
			std::string shortname;
			uint64_t patlen;
			uint64_t zoffset;
			uint64_t numblocks;
			std::vector<uint64_t> blocks;
			
			FastABgzfIndexEntry()
			{
			
			}
			
			FastABgzfIndexEntry(std::istream & in)
			{
				name = libmaus2::util::StringSerialisation::deserialiseString(in);
				shortname = libmaus2::util::StringSerialisation::deserialiseString(in);
				patlen = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				zoffset = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				numblocks = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				blocks.resize(numblocks+1);
				for ( uint64_t i = 0; i < numblocks+1; ++i )
					blocks[i] = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
			}
		};

		::std::ostream & operator<<(::std::ostream & out, FastABgzfIndexEntry const & F);
	}
}
#endif
