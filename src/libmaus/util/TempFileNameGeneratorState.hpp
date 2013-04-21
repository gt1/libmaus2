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
#if ! defined(LIBMAUS_UTIL_TEMPFILENAMEGENERATORSTATE_HPP)
#define LIBMAUS_UTIL_TEMPFILENAMEGENERATORSTATE_HPP

#include <libmaus/math/Log.hpp>
#include <iomanip>
#include <vector>

namespace libmaus
{
	namespace util
	{	
		struct TempFileNameGeneratorState
		{
			static int const dirmod = 64;
			static int const filemod = 64;
			static int const maxmod = (dirmod>filemod)?dirmod:filemod;
			static int const digits = ::libmaus::math::LogCeil<maxmod,10>::log;
			
			unsigned int depth;			
			std::vector < int > nextdir;
			int64_t nextfile;
			std::string const prefix;
			
			bool operator==(TempFileNameGeneratorState const & o) const;
			bool operator!=(TempFileNameGeneratorState const & o) const;

			TempFileNameGeneratorState(unsigned int const rdepth, std::string const & rprefix);
			
			void setup();
			void next();
			static std::string numToString(uint64_t const num, unsigned int dig);
			std::string getFileName();
			void removeDirs();
		};
	}
}
#endif
