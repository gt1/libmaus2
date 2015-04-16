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

#if ! defined(LSFPROCESS_HPP)
#define LSFPROCESS_HPP

#include <libmaus/exception/LibMausException.hpp>
#include <libmaus/util/unique_ptr.hpp>
#include <libmaus/util/shared_ptr.hpp>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/util/ForkProcessLSFSim.hpp>
#include <libmaus/parallel/OMPLock.hpp>
#include <libmaus/lsf/LSFStateBase.hpp>
#include <vector>
#include <string>

#if defined(HAVE_LSF)
namespace libmaus
{
	namespace lsf
	{
		struct LSF
		{
			static ::libmaus::parallel::OMPLock lsflock;
			static void init(std::string const & sappname);
			static std::string getClusterName();
			
			static uint64_t Mscale;
		};
	
		struct LSFProcess : public LSFStateBase
		{
		        typedef LSFProcess this_type;
		        typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
		        typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
		
		        int64_t const id;
		        
		        static void initLSF();
		        static bool distributeUnique() { return true; }
                        
                        static int64_t submitJob(
        		        std::string const & scommand,
        		        std::string const & sjobname,
        		        std::string const & sproject,
        		        std::string const & queuename,
        		        unsigned int const numcpu,
        		        unsigned int const maxmem,
        		        std::string const & sinfilename,
        		        std::string const & soutfilename,
        		        std::string const & serrfilename,
        		        std::vector < std::string > const * hosts = 0,
        		        char const * cwd = 0,
        		        uint64_t const tmpspace = 0,
        		        char const * model = 0
                        );

        		LSFProcess(
        		        std::string const & scommand,
        		        std::string const & sjobname,
        		        std::string const & sproject,
        		        std::string const & queuename,
        		        unsigned int const numcpu,
        		        unsigned int const maxmem,
        		        std::string const & sinfilename = "/dev/null",
        		        std::string const & soutfilename = "/dev/null",
        		        std::string const & serrfilename = "/dev/null",
        		        std::vector < std::string > const * hosts = 0,
        		        char const * cwd = 0,
        		        uint64_t const tmpspace = 0,
        		        char const * model = 0
                		);

			bool isKnown() const;
			bool isUnfinished() const;

			bool isRunning() const;
			bool isFinished() const;
			bool finishedOk() const;
			void wait(int sleepinterval) const;
			void kill(int const sig = SIGTERM) const;
        		
			bool getHost(::std::vector<std::string> & hostnames) const;
			std::string getSingleHost() const;

			state getState() const;
			int64_t getIntState() const;
			static std::map<int64_t,int64_t> getIntStates();
			
			int64_t getIntState(std::map<int64_t,int64_t> const & M) const
			{
				if ( M.find(id) != M.end() )
					return M.find(id)->second;
				else
					return 0;
			}
			state getState(std::map<int64_t,int64_t> const & M) const;
		};
	}
}
#else
namespace libmaus
{
	namespace lsf
	{
		typedef ::libmaus::util::LSFSim LSF;
		typedef ::libmaus::util::ForkProcessLSFSim LSFProcess;
	}
}
#endif

#endif
