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

#if ! defined(LIBMAUS2_UTIL_FORKPROCESSLSFSIM_HPP)
#define LIBMAUS2_UTIL_FORKPROCESSLSFSIM_HPP

#include <libmaus2/util/ForkProcess.hpp>
#include <libmaus2/lsf/LSFStateBase.hpp>
#include <libmaus2/util/LSFSim.hpp>
#include <map>

namespace libmaus2
{
	namespace util
	{
		/**
		 * rudimentary class for simulation LSF jobs by ForkProcess objects
		 **/
		struct ForkProcessLSFSim : public ForkProcess, public ::libmaus2::lsf::LSFStateBase
		{
			//! this type
			typedef ForkProcessLSFSim this_type;
			//! unique pointer type
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			/**
			 * init the fake LSF system
			 *
			 * @param s program name
			 **/
			static void initLSF(std::string const & s);
			/**
			 * @return true if unique host names can be achieved for multiple jobs
			 *         (for this class this function always returns false)
			 **/
		        static bool distributeUnique();

		        /**
		         * constructor
		         *
		         * @param scommand command name
		         * @param maxmem maximum amount of memory in MB
		         * @param sinfilename input file name, default /dev/null
		         * @param soutputfilename output file name, default /dev/null
		         * @param serrfilename error file name, default /dev/null
		         * @param cwd current working directory for new job
		         **/
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
			/**
			 * @return process state
			 **/
			state getState();
			/**
			 * @return true iff process finished
			 **/
			bool isFinished();
			/**
			 * @return true iff process finished ok
			 **/
			bool finishedOk();
			/**
			 * wait for process to finish
			 *
			 * @param sleepinterval number of seconds to sleep between checking finished()
			 **/
			void wait(int sleepinterval = 1);
			/**
			 * @return execution host
			 **/
			std::string getSingleHost() const;
			/**
			 * get list of host names the job is running on
			 *
			 * @param reference to host names vector
			 * @return true if successful
			 **/
			bool getHost(::std::vector<std::string> & hostnames);
			/**
			 * @return true if job is known in the scheduling system
			 **/
			bool isKnown();
			/**
			 * @return true if job is unfinished
			 **/
			bool isUnfinished();
			/**
			 * @return state map for all processes (this map is empty for the LSFSim class)
			 **/
			static std::map<int64_t,int64_t> getIntStates();
			/**
			 * get state for this job
			 *
			 * @return state of this job
			 **/
			state getState(std::map<int64_t,int64_t> const &);
		};
	}
}
#endif
