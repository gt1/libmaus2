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

#include <libmaus2/util/ForkProcessLSFSim.hpp>
#include <unistd.h>

void libmaus2::util::ForkProcessLSFSim::initLSF(std::string const & s) {
	LSFSim::init(s);	
}

bool libmaus2::util::ForkProcessLSFSim::distributeUnique() { return false; }

libmaus2::util::ForkProcessLSFSim::ForkProcessLSFSim(
	std::string const & scommand,
	std::string const & /* sjobname */,
	std::string const & /* sproject */,
	std::string const & /* queuename */,
	unsigned int const /* numcpu */,
	unsigned int const maxmem,
	std::string const & sinfilename,
	std::string const & soutfilename,
	std::string const & serrfilename,
	std::vector < std::string > const * /* hosts */,
	char const * cwd ,
	uint64_t const /* tmpspace */
	)
: ForkProcess(scommand,(cwd?std::string(cwd):std::string(".")),
  (maxmem*1024ull*1024ull),sinfilename,soutfilename,serrfilename)
{

}

libmaus2::util::ForkProcessLSFSim::state libmaus2::util::ForkProcessLSFSim::getState()
{
	if ( isFinished() )
		return state_done;
	else
		return state_run;
}

bool libmaus2::util::ForkProcessLSFSim::isFinished()
{
	return (!running());
}
bool libmaus2::util::ForkProcessLSFSim::finishedOk()
{
	if ( isFinished() )
	{
		bool const ok = join();
		return ok;
	}
	else
	{
		return false;
	}
}
void libmaus2::util::ForkProcessLSFSim::wait(int sleepinterval)
{
	while ( ! isFinished() )
		sleep(sleepinterval);
	join();
}

std::string libmaus2::util::ForkProcessLSFSim::getSingleHost() const
{
	return "localhost";
}
		
bool libmaus2::util::ForkProcessLSFSim::getHost(::std::vector<std::string> & hostnames)
{
	hostnames = std::vector < std::string >(1,"localhost");
	return true;
}

bool libmaus2::util::ForkProcessLSFSim::isKnown()
{
	return true;
}

bool libmaus2::util::ForkProcessLSFSim::isUnfinished()
{
	return ! isFinished();
}

std::map<int64_t,int64_t> libmaus2::util::ForkProcessLSFSim::getIntStates()
{
	return std::map<int64_t,int64_t>();
}

libmaus2::util::ForkProcessLSFSim::state libmaus2::util::ForkProcessLSFSim::getState(std::map<int64_t,int64_t> const &)
{
	if ( isFinished() )
		return state_done;
	else
		return state_run;
}
