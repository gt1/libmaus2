/*
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
*/

#if ! defined(LIBMAUS_UTIL_FORKPROCESSLSFSIM_HPP)
#define LIBMAUS_UTIL_FORKPROCESSLSFSIM_HPP

#include <libmaus/util/ForkProcess.hpp>
#include <libmaus/lsf/LSFStateBase.hpp>
#include <libmaus/util/LSFSim.hpp>
#include <map>

namespace libmaus
{
	namespace util
	{
		struct ForkProcessLSFSim : public ForkProcess, public ::libmaus::lsf::LSFStateBase
		{
			typedef ForkProcessLSFSim this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			
			static void initLSF(std::string const & s);
		        static bool distributeUnique();
		
			ForkProcessLSFSim(
				std::string const & scommand,
				std::string const & /* sjobname */,
				std::string const & /* sproject */,
				std::string const & /* queuename */,
				unsigned int const /* numcpu */,
				unsigned int const maxmem,
				std::string const & sinfilename = "/dev/null",
				std::string const & soutfilename = "/dev/null",
				std::string const & serrfilename = "/dev/null",
				std::vector < std::string > const * /* hosts */ = 0,
				char const * cwd = 0,
				uint64_t const = 0 /* tmpspace */
				);
			state getState();
			bool isFinished();
			bool finishedOk();
			void wait(int sleepinterval = 1);
			std::string getSingleHost() const;
			bool getHost(::std::vector<std::string> & hostnames);
			bool isKnown();
			bool isUnfinished();
			static std::map<int64_t,int64_t> getIntStates();
			state getState(std::map<int64_t,int64_t> const &);
		};
	}
}
#endif
