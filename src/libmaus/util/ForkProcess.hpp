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

#if ! defined(LIBMAUS_UTIL_FORKPROCESS_HPP)
#define LIBMAUS_UTIL_FORKPROCESS_HPP

#include <libmaus/util/unique_ptr.hpp>
#include <string>
#include <vector>
#include <csignal>
#include <limits>

namespace libmaus
{
	namespace util
	{
	        struct ForkProcess
		{
			typedef ForkProcess this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			pid_t id;
			// 0 read
			// 1 write
			int failpipe[2];
			bool joined;
			bool result;
			std::string failmessage;
			
			void kill(int sig = SIGTERM);
			void init(
				std::string const & exe,
				std::string const & pwd,
				std::vector < std::string > const & rargs,
				uint64_t const maxmem = std::numeric_limits<uint64_t>::max(),
				std::string const infilename = std::string(),
				std::string const outfilename = std::string(),
				std::string const errfilename = std::string()
				);
			ForkProcess(
				std::string const exe,
				std::string const pwd,
				std::vector < std::string > const & rargs,
				uint64_t const maxmem = std::numeric_limits<uint64_t>::max(),
				std::string const infilename = std::string(),
				std::string const outfilename = std::string(),
				std::string const errfilename = std::string()
				);
			ForkProcess(
				std::string const cmdline,
				std::string const pwd,
				uint64_t const maxmem = std::numeric_limits<uint64_t>::max(),
				std::string const infilename = std::string(),
				std::string const outfilename = std::string(),
				std::string const errfilename = std::string()
			);
			bool running();
			bool join();
		};
	}
}	
#endif
