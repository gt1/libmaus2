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

#if ! defined(LIBMAUS2_UTIL_FORKPROCESS_HPP)
#define LIBMAUS2_UTIL_FORKPROCESS_HPP

#include <libmaus2/util/unique_ptr.hpp>
#include <string>
#include <vector>
#include <csignal>
#include <limits>

namespace libmaus2
{
	namespace util
	{
		/**
		 * process created by the fork() system call
		 **/
	        struct ForkProcess
		{
			//! this type
			typedef ForkProcess this_type;
			//! unique pointer type
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			//! process id
			pid_t id;
			//! pipe for receiving errors occuring while calling the program to be started
			int failpipe[2];
			//! true if join has been complete
			bool joined;
			//! true if exit code 0 was received by join
			bool result;
			//! failure message
			std::string failmessage;

			/**
			 * send signal sig to process
			 *
			 * @param sig signal number
			 **/
			void kill(int sig = SIGTERM);
			/**
			 * initialize process
			 *
			 * @param exe path to executable
			 * @param pwd current working directory for new process
			 * @param rargs arguments for new process
			 * @param maxmem memory limit per ulimit in bytes, default unlimited
			 * @param infilename input file name for stdin (empty string means: keep stdin of parent)
			 * @param outfilename output file name for stdout (empty string means: keep stdout of parent)
			 * @param errfilename output file name for stderr (empty string means: keep stderr of parent)
			 **/
			void init(
				std::string const & exe,
				std::string const & pwd,
				std::vector < std::string > const & rargs,
				uint64_t const maxmem = std::numeric_limits<uint64_t>::max(),
				std::string const infilename = std::string(),
				std::string const outfilename = std::string(),
				std::string const errfilename = std::string()
				);
			/**
			 * constructor
			 *
			 * @param exe path to executable
			 * @param pwd current working directory for new process
			 * @param rargs arguments for new process
			 * @param maxmem memory limit per ulimit in bytes, default unlimited
			 * @param infilename input file name for stdin (empty string means: keep stdin of parent)
			 * @param outfilename output file name for stdout (empty string means: keep stdout of parent)
			 * @param errfilename output file name for stderr (empty string means: keep stderr of parent)
			 **/
			ForkProcess(
				std::string const exe,
				std::string const pwd,
				std::vector < std::string > const & rargs,
				uint64_t const maxmem = std::numeric_limits<uint64_t>::max(),
				std::string const infilename = std::string(),
				std::string const outfilename = std::string(),
				std::string const errfilename = std::string()
				);
			/**
			 * constructor
			 *
			 * @param cmdline command line
			 * @param pwd current working directory for new process
			 * @param maxmem memory limit per ulimit in bytes, default unlimited
			 * @param infilename input file name for stdin (empty string means: keep stdin of parent)
			 * @param outfilename output file name for stdout (empty string means: keep stdout of parent)
			 * @param errfilename output file name for stderr (empty string means: keep stderr of parent)
			 **/
			ForkProcess(
				std::string const cmdline,
				std::string const pwd,
				uint64_t const maxmem = std::numeric_limits<uint64_t>::max(),
				std::string const infilename = std::string(),
				std::string const outfilename = std::string(),
				std::string const errfilename = std::string()
			);
			/**
			 * @return true if process is running
			 **/
			bool running();
			/**
			 * wait for process to finish
			 *
			 * @return true if process exited with code zero
			 **/
			bool join();
		};
	}
}
#endif
