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

#if ! defined(MEMUSAGE_HPP)
#define MEMUSAGE_HPP

#include <fstream>
#include <map>
#include <sstream>

namespace libmaus2
{
	namespace util
	{
		struct MemUsage
		{
			uint64_t VmPeak;
			uint64_t VmSize;
			uint64_t VmLck;
			uint64_t VmHWM;
			uint64_t VmRSS;
			uint64_t VmData;
			uint64_t VmStk;
			uint64_t VmExe;
			uint64_t VmLib;
			uint64_t VmPTE;

			MemUsage();
			MemUsage(MemUsage const & o);
			MemUsage & operator=(MemUsage const & o);

			bool operator==(MemUsage const & o) const;
			bool operator!=(MemUsage const & o) const;

			static uint64_t getMemParam(std::map<std::string,std::string> const & M, std::string const key);
			static uint64_t parseMemPair(std::string const & V);
			static bool tokenise(std::string line, std::pair<std::string,std::string> & P);
			static std::map<std::string,std::string> getProcSelfStatusMap();
		};

		std::ostream & operator<<(std::ostream & out, MemUsage const & M);
	}
}
#endif
