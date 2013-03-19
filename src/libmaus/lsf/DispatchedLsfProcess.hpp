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
#if ! defined(LIBMAUS_LSF_DISPATCHEDLSFPROCESS_HPP)
#define LIBMAUS_LSF_DISPATCHEDLSFPROCESS_HPP

#include <libmaus/lsf/LSFProcess.hpp>
#include <libmaus/util/ArgInfo.hpp>
#include <libmaus/network/Socket.hpp>

#include <iomanip>

namespace libmaus
{
	namespace lsf
	{
		struct DispatchedLsfProcess : public ::libmaus::lsf::LSFProcess
		{
			typedef DispatchedLsfProcess this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

			static std::string getFile   (std::string const & exe) { return exe.substr(exe.find_last_of("/")+1); }
			static std::string getJobName(std::string const & exe, uint64_t const id)
			{
				std::ostringstream ostr;
				ostr << getFile(exe) << "_" << ::libmaus::network::GetHostName::getHostName() << "_" << getpid() << "_" << time(0) << "_" << std::setw(4) << std::setfill('0') << id;
				return ostr.str();
			}

			static std::string getProject(::libmaus::util::ArgInfo const & arginfo, std::string const def = "hpag-pipeline") { return arginfo.getValue<std::string>("project",   def); }
			static std::string getQueue  (::libmaus::util::ArgInfo const & arginfo, std::string const def = "long"         ) { return arginfo.getValue<std::string>("queue",     def); }
			static uint64_t    getNumThr (::libmaus::util::ArgInfo const & arginfo, uint64_t const def    = 1              ) { return arginfo.getValue<uint64_t   >("numthreads",def); }

			static std::string getDispatcherCommand(
				std::string const & dispatcherexe,
				std::string const & sid,
				std::string const & serverhostname,
				unsigned int const logport,
				uint64_t const id,
				uint64_t const mem,
				std::string const & cmd,
				bool const valgrind)
			{
				std::ostringstream ostr;
				ostr
					<< dispatcherexe
					<< " sid=" << sid
					<< " serverhostname=" << serverhostname
					<< " logport=" << logport
					<< " id=" << id
					<< " mem=" << mem
					<< " cmd=" << cmd
					<< " valgrind=" << valgrind;
				return ostr.str();
			}
			
			static std::string getAdaptedPath(::libmaus::util::ArgInfo const & arginfo, std::string const & payloadexe)
			{
				if ( ! payloadexe.size() )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "string is empty in DispatchedLsfProcess::getAdaptedPath()" << std::endl;
					se.finish();
					throw se;
				}
				
				if ( payloadexe[0] == '/' )
				{
					// std::cerr << "q=" << payloadexe << std::endl;
					return payloadexe;
				}
				else
				{
					std::string const p = arginfo.getProgDirName() + "/" + payloadexe;
					// std::cerr << "p=" << p << std::endl;
					return p;
				}
			}

			::libmaus::network::SocketBase::shared_ptr_type controlsocket;
			std::string hostname;
																							
			DispatchedLsfProcess(
				::libmaus::util::ArgInfo const & arginfo,
				std::string const & sid,
				std::string const & serverhostname,
				unsigned int const logport,
				uint64_t const id,
				std::string const & dispatcherexe,
				std::string const & payloadexe,
				uint64_t const numthreads,
				uint64_t const mem,
				std::vector<std::string> const * hosts = 0,
				char const * cwd = 0,
				uint64_t const tmpspace = 0,
				bool const valgrind = false
			)
			:
				::libmaus::lsf::LSFProcess(
					getDispatcherCommand(getAdaptedPath(arginfo,dispatcherexe),sid,serverhostname,logport,id,mem,getAdaptedPath(arginfo,payloadexe),valgrind),
					getJobName(payloadexe,id),
					getProject(arginfo),
					getQueue(arginfo),
					numthreads,
					mem,
					"/dev/null",
					"/dev/null",
					"/dev/null",
					hosts,
					cwd,
					tmpspace
				)
			{
			
			}
			
			void waitKnown()
			{
				while ( ! isKnown() )
					sleep(1);
			}
			
			void join()
			{
				while ( isUnfinished() )
					sleep(1);
			}

			static void start(
				std::vector < shared_ptr_type > & procs,
				::libmaus::util::ArgInfo const & arginfo,
				std::string const & sid,
				std::string const & serverhostname,
				unsigned int const logport,
				uint64_t const n,
				std::string const & dispatcherexe,
				std::string const & payloadexe,
				uint64_t const numthreads,
				uint64_t const mem,
				std::vector<std::string> const * hosts = 0,
				char const * cwd = 0,
				uint64_t const tmpspace = 0,
				bool const valgrind = false
			)
			{
				for ( uint64_t i = 0; i < n; ++i )
				{
					uint64_t const id = procs.size();
					shared_ptr_type nproc(new this_type(arginfo,sid,serverhostname,logport,id,dispatcherexe,payloadexe,numthreads,mem,hosts,cwd,tmpspace,valgrind));
					procs.push_back(nproc);
				}
				// wait until the new processes are visible in LSF
				for ( uint64_t i = 0; i < n; ++i )
					procs[procs.size()-n+i]->waitKnown();			
			}
			
			static std::vector < shared_ptr_type > start(
				::libmaus::util::ArgInfo const & arginfo,
				std::string const & sid,
				std::string const & serverhostname,
				unsigned int const logport,
				uint64_t const n,
				std::string const & dispatcherexe,
				std::string const & payloadexe,
				uint64_t const numthreads,
				uint64_t const mem,
				std::vector<std::string> const * hosts = 0,
				char const * cwd = 0,
				uint64_t const tmpspace = 0,
				bool const valgrind = false
			)
			{
				std::vector < shared_ptr_type > procs;
				start(procs,arginfo,sid,serverhostname,logport,n,dispatcherexe,payloadexe,numthreads,mem,hosts,cwd,tmpspace,valgrind);
				return procs;
			}

			static void join(std::vector < shared_ptr_type > & procs)
			{
				for ( uint64_t i = 0; i < procs.size(); ++i )
					if ( procs[i] )
						procs[i]->join();
			}
		};
	}
}
#endif
