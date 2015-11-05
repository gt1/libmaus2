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
#if ! defined(LIBMAUS2_LSF_LSFDISPATCHERBASE_HPP)
#define LIBMAUS2_LSF_LSFDISPATCHERBASE_HPP

#include <libmaus2/lsf/ProcessSet.hpp>

namespace libmaus2
{
	namespace lsf
	{
		struct LSFDispatcherBase
		{
			static uint64_t const memgrace = 50;

			uint64_t const exmem;
			::libmaus2::util::ArgInfo const & arginfo;
			::libmaus2::lsf::ProcessSet BPS;

			// submit processes
			::libmaus2::lsf::ProcessSet::AsynchronousWaiter::unique_ptr_type waiter;

			static uint64_t getNumProcs(uint64_t const maxprocs, ::libmaus2::util::ArgInfo const & arginfo)
			{
				return std::min(static_cast<uint64_t>(maxprocs),arginfo.getValue<uint64_t>("numprocs",32));
			}

			LSFDispatcherBase(
				uint64_t const rexmem,
				::libmaus2::util::ArgInfo const & rarginfo,
				uint64_t const maxprocs,
				std::string const & dispatchprogname,
				uint64_t const lsfthreads = 1
			)
			: exmem(rexmem), arginfo(rarginfo), BPS(dispatchprogname,arginfo)
			{
				uint64_t const blocksortmem = arginfo.getValue<uint64_t>("blocksortmem",(exmem/(1000*1000))+memgrace);
				std::string const queue = arginfo.getValue<std::string>("queue","normal");
				std::string const project = arginfo.getValue<std::string>("project","hpag-pipeline");
				uint64_t const numprocs = getNumProcs(maxprocs,arginfo);

				std::vector < std::string > reqhosts;
				std::vector < std::string > const * phosts = 0;
				if ( arginfo.hasArg("hosts") )
				{
					std::string const shosts = arginfo.getValue<std::string>("hosts","<null>");
					std::deque < std::string > tokens = ::libmaus2::util::stringFunctions::tokenize(shosts,std::string(","));
					if ( tokens.size() )
					{
						reqhosts = std::vector < std::string > (tokens.begin(),tokens.end());
						phosts = &reqhosts;
					}
				}
				waiter = UNIQUE_PTR_MOVE(BPS.startAsynchronous(numprocs,0,dispatchprogname,dispatchprogname,project,queue,lsfthreads,blocksortmem,"/dev/null","/dev/null","/dev/null",phosts));
			}

			~LSFDispatcherBase()
			{
				std::cerr << "Sending stop signal to waiter...";
				// stop waiter from accepting more connections
				waiter->stop();
				//
				std::cerr << "done." << std::endl;

				std::cerr << "Calling join for waiter...";
				// wait until waiter thread has finished
				waiter->join();
				std::cerr << "done." << std::endl;

				std::cerr << "Killing remaing processes...";
				// kill remaining processes
				for ( uint64_t i = 0; i < BPS.size(); ++i )
					BPS.process(i)->kill();
				std::cerr << "done." << std::endl;

				std::cerr << "Waiting for processes to leave...";
				// wait until they are gone
				for ( uint64_t i = 0; i < BPS.size(); ++i )
					BPS.process(i)->wait(1);
				std::cerr << "done." << std::endl;
			}
		};
	}
}
#endif
