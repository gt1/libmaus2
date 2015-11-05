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

#include <libmaus2/lsf/LSFProcess.hpp>

#if defined(HAVE_LSF)
#include <lsf/lsbatch.h>
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#include <libmaus2/types/types.hpp>
#include <libmaus2/util/WriteableString.hpp>
#include <libmaus2/util/PosixExecute.hpp>
#include <libmaus2/util/stringFunctions.hpp>

uint64_t libmaus2::lsf::LSF::Mscale = 1000;

::libmaus2::parallel::OMPLock libmaus2::lsf::LSF::lsflock;

void libmaus2::lsf::LSF::init(std::string const & sappname)
{
	::libmaus2::parallel::ScopeLock lock(lsflock);

	::libmaus2::autoarray::AutoArray<char> appname(sappname.size()+1);
	::std::copy(sappname.begin(),sappname.end(),appname.get());

        if (lsb_init(appname.get()) < 0)
        {
		char buf[] = "lsb_init()";
		lsb_perror(buf);

		::libmaus2::exception::LibMausException se;
		se.getStream() << "lsb_init() in libmaus2::lsf::LSF::init" << std::endl;
		se.finish();
		throw se;
	}
}

std::string libmaus2::lsf::LSF::getClusterName()
{
	::libmaus2::parallel::ScopeLock lock(lsflock);

	char const *clustername = ls_getclustername();

        if (clustername == NULL) {
                char buf[] = "ls_getclustername in libmaus2::lsf::LSF::getClusterName";
                ls_perror(buf);

		::libmaus2::exception::LibMausException se;
		se.getStream() << "ls_getclustername() in libmaus2::lsf::LSF::getClusterName" << std::endl;
		se.finish();
		throw se;
        }

        return std::string(clustername);
}

int64_t libmaus2::lsf::LSFProcess::submitJob(
        std::string const & scommand,
        std::string const & sjobname,
        std::string const & sproject,
        std::string const & squeuename,
        unsigned int const numcpu,
        unsigned int const maxmem,
        std::string const & sinfilename,
        std::string const & soutfilename,
        std::string const & serrfilename,
        std::vector < std::string > const * hosts,
        char const * rcwd,
        uint64_t const tmpspace,
        char const * model
        )
{
	// no lock, submitting happens in a different process
	// ::libmaus2::parallel::ScopeLock lock(::libmaus2::lsf::LSF::lsflock);

        std::ostringstream bsubstr;
        bsubstr
                << "bsub"
                << " " << "-G \"" << sproject << "\""
                << " " << "-P \"" << sproject << "\""
                << " " << "-q \"" << squeuename << "\""
                << " " << "-J \"" << sjobname << "\""
                << " " << "-o \"" << soutfilename << "\""
                << " " << "-e \"" << serrfilename << "\""
                << " " << "-i \"" << sinfilename << "\""
                << " " << "-M \"" << maxmem*libmaus2::lsf::LSF::Mscale << "\""
                << " " << "-R'" << "select[(mem>="<<maxmem<<" && type==X86_64";
	if ( model != 0 )
		bsubstr << " && model==" << model;
        if ( tmpspace != 0 )
                bsubstr << " && tmp>=" << tmpspace;
        bsubstr << ")] rusage[mem="<<maxmem;
        if ( tmpspace != 0 )
        	bsubstr << ",tmp=" << tmpspace;
        bsubstr <<"] span[hosts=1]" << "'";
                ;
        if ( numcpu != 1 )
                bsubstr
                        << " " << "-n \"" << numcpu << "\"";
        if ( rcwd )
                bsubstr << " " << "-cwd \"" << rcwd << "\"";
        if ( hosts && hosts->size() >= 1 )
        {
                bsubstr << " " << "-m \"";
                std::vector < std::string > const & vhosts = *hosts;

                for ( uint64_t h = 0; h < vhosts.size(); ++h )
                {
                        bsubstr << vhosts[h];
                        if ( h+1 < vhosts.size() )
                                bsubstr << " ";
                }

                bsubstr << "\"";
        }

        bsubstr << " ";

        for ( uint64_t i = 0; i < scommand.size(); ++i )
                if ( scommand[i] == '"' )
                        bsubstr << '"';
                else
                        bsubstr << scommand[i];

        std::string const bsub = bsubstr.str();

        std::string out, err;
        #if 1
        std::cerr << "[[" << bsub << "]]" << std::endl;
        #endif
        int const ret = ::libmaus2::util::PosixExecute::execute(bsub,out,err);

        if ( ret == 0 )
        {
                std::deque<std::string> tokens = ::libmaus2::util::stringFunctions::tokenize(out,std::string(" "));
                if ( tokens.size() >= 2 && tokens[1].size() && tokens[1][0] == '<' && tokens[1][tokens[1].size()-1] == '>' )
                {
                        std::string stringid = tokens[1].substr(1,tokens[1].size()-2);
                        // std::cerr << stringid << std::endl;
                        std::istringstream stringistr(stringid);
                        int64_t id;
                        stringistr >> id;
                        if ( stringistr )
                                return id;
                }
        }

        ::libmaus2::exception::LibMausException se;
        se.getStream() << "Failed to create LSF process.";
        se.finish();
        throw se;
}

libmaus2::lsf::LSFProcess::LSFProcess(
        std::string const & scommand,
        std::string const & sjobname,
        std::string const & sproject,
        std::string const & squeuename,
        unsigned int const numcpu,
        unsigned int const maxmem,
        std::string const & sinfilename,
        std::string const & soutfilename,
        std::string const & serrfilename,
        std::vector < std::string > const * hosts,
        char const * cwd,
        uint64_t const tmpspace,
        char const * model
        )
: id(submitJob(scommand,sjobname,sproject,squeuename,numcpu,maxmem,sinfilename,soutfilename,serrfilename,hosts,cwd,tmpspace,model))
{
}

bool libmaus2::lsf::LSFProcess::isFinished() const
{
	::libmaus2::parallel::ScopeLock lock(::libmaus2::lsf::LSF::lsflock);

        bool finished = false;

        int numrec = lsb_openjobinfo(id,0,0,0,0,ALL_JOB);

        if ( numrec == -1 )
        {
                // std::cerr << "Job " << id << " not found in isFinished()" << std::endl;
                return false;
        }

        if ( numrec > 0 )
        {
                jobInfoEnt const * jie = lsb_readjobinfo(&numrec);

                if ( jie )
                        // finished = (IS_FINISH(jie->status) && IS_POST_FINISH(jie->status));
                        finished = IS_FINISH(jie->status);
                else
                        finished = false;
        }

        lsb_closejobinfo();

        // std::cerr << "Returning " << finished << " for job " << id << std::endl;

        return finished;
}

bool libmaus2::lsf::LSFProcess::isKnown() const
{
	::libmaus2::parallel::ScopeLock lock(::libmaus2::lsf::LSF::lsflock);

        int numrec = lsb_openjobinfo(id,0,0,0,0,ALL_JOB);

        lsb_closejobinfo();

	if ( numrec <= 0 )
		return false;
	else
		return true;
}

bool libmaus2::lsf::LSFProcess::isUnfinished() const
{
	::libmaus2::parallel::ScopeLock lock(::libmaus2::lsf::LSF::lsflock);

        int numrec = lsb_openjobinfo(id,0,0,0,0,ALL_JOB);

        // error, assume job is finished
        if ( numrec < 0 )
		return false;

        // job unknown, assume it is finished
	if ( numrec == 0 )
	{
	        lsb_closejobinfo();
		return false;
	}
	else // numrec > 0
	{
        	// read record
                jobInfoEnt const * jie = lsb_readjobinfo(&numrec);

                // this should not happen, assume job is finished
                if ( !jie )
                {
		        lsb_closejobinfo();
		        return false;
                }
                else
                {

                	bool const unfinished =
                		(jie->status & JOB_STAT_PEND) ||
                		(jie->status & JOB_STAT_PSUSP) ||
                		(jie->status & JOB_STAT_RUN) ||
                		(jie->status & JOB_STAT_USUSP) ||
                		(jie->status & JOB_STAT_SSUSP);

		        lsb_closejobinfo();

		        return unfinished;
                }
	}
}

bool libmaus2::lsf::LSFProcess::isRunning() const
{
	::libmaus2::parallel::ScopeLock lock(::libmaus2::lsf::LSF::lsflock);

        int numrec = lsb_openjobinfo(id,0,0,0,0,ALL_JOB);

        // function failed
        if ( numrec == -1 )
                return false;

        bool running = false;

        // there are records
        if ( numrec > 0 )
        {
        	// read record
                jobInfoEnt const * jie = lsb_readjobinfo(&numrec);

                if ( jie )
                        running = IS_START(jie->status);
		else
			running = false;
        }

        lsb_closejobinfo();

        // std::cerr << "Returning " << finished << " for job " << id << std::endl;

        return running;
}

bool libmaus2::lsf::LSFProcess::finishedOk() const
{
	::libmaus2::parallel::ScopeLock lock(::libmaus2::lsf::LSF::lsflock);

        bool ok = false;

        int numrec = lsb_openjobinfo(id,0,0,0,0,ALL_JOB);

        if ( numrec == -1 )
        {
                // std::cerr << "Job " << id << " not found in isFinished()" << std::endl;
                return false;
        }

        if ( numrec > 0 )
        {
                jobInfoEnt const * jie = lsb_readjobinfo(&numrec);

                if ( jie )
                        // ok = (IS_FINISH(jie->status) && IS_POST_FINISH(jie->status));
                        ok = IS_FINISH(jie->status) && (jie->status & JOB_STAT_DONE);
                else
                        ok = false;
        }

        lsb_closejobinfo();

        // std::cerr << "Returning " << ok << " for job " << id << std::endl;

        return ok;
}

void libmaus2::lsf::LSFProcess::wait(int sleepinterval) const
{
        while ( isRunning() )
                if ( sleepinterval )
                        sleep(sleepinterval);
}

std::string libmaus2::lsf::LSFProcess::getSingleHost() const
{
        std::vector < std::string > hostnames;
        while ( ! libmaus2::lsf::LSFProcess::getHost(hostnames) )
                sleep(1);

        if ( hostnames.size() == 1 )
        {
                return hostnames[0];
        }
        else
        {
                ::libmaus2::exception::LibMausException se;
                se.getStream() << "Failed to get LSF host name.";
                se.finish();
                throw se;
        }
}

bool libmaus2::lsf::LSFProcess::getHost(std::vector < std::string > & hostnames) const
{
	::libmaus2::parallel::ScopeLock lock(::libmaus2::lsf::LSF::lsflock);

        bool hostok = false;

        // std::cerr << "Getting host names for id " << id << std::endl;

        int numrec = lsb_openjobinfo(id,0,0,0,0,ALL_JOB);

        if ( numrec <= 0 )
        {
                // std::cerr << "Job " << id << " not found in getHost()" << std::endl;
        }
        else if ( numrec > 0 )
        {
                jobInfoEnt const * jie = lsb_readjobinfo(&numrec);

                if ( jie )
                {
                        if ( jie->numExHosts )
                        {
                                for ( int i = 0; i < jie->numExHosts; ++i )
                                        hostnames.push_back(jie->exHosts[i]);
                                hostok = true;
                        }
                }
        }

        lsb_closejobinfo();

        return hostok;

}

void libmaus2::lsf::LSFProcess::kill(int const sig) const
{
	::libmaus2::parallel::ScopeLock lock(::libmaus2::lsf::LSF::lsflock);

        std::ostringstream ostr;
        ostr << "bkill -s " << sig << " " << id;

        std::string out, err;
        ::libmaus2::util::PosixExecute::execute(ostr.str(),out,err);

        // system(ostr.str().c_str());
}

static libmaus2::lsf::LSFProcess::state statusToState(int64_t const status)
{
	if ( status & JOB_STAT_PEND )
		return libmaus2::lsf::LSFProcess::state_pending;
	else if ( status & JOB_STAT_PSUSP )
		return libmaus2::lsf::LSFProcess::state_psusp;
	else if ( status & JOB_STAT_SSUSP )
		return libmaus2::lsf::LSFProcess::state_ssusp;
	else if ( status & JOB_STAT_USUSP )
		return libmaus2::lsf::LSFProcess::state_ususp;
	else if ( status & JOB_STAT_RUN )
		return libmaus2::lsf::LSFProcess::state_run;
	else if ( status & JOB_STAT_EXIT )
		return libmaus2::lsf::LSFProcess::state_exit;
	else if ( status & JOB_STAT_DONE )
		return libmaus2::lsf::LSFProcess::state_done;
	else if ( status & JOB_STAT_PDONE )
		return libmaus2::lsf::LSFProcess::state_pdone;
	else if ( status & JOB_STAT_PERR )
		return libmaus2::lsf::LSFProcess::state_perr;
	else if ( status & JOB_STAT_WAIT )
		return libmaus2::lsf::LSFProcess::state_wait;
	else
		return libmaus2::lsf::LSFProcess::state_unknown;
}

libmaus2::lsf::LSFProcess::state libmaus2::lsf::LSFProcess::getState(std::map<int64_t,int64_t> const & M) const
{
	state const s = statusToState(getIntState(M));

	// std::cerr << "id " << id << " state " << s << std::endl;

	return s;
}

libmaus2::lsf::LSFProcess::state libmaus2::lsf::LSFProcess::getState() const
{
	return statusToState(getIntState());
}

int64_t libmaus2::lsf::LSFProcess::getIntState() const
{
	int lsfr = lsb_openjobinfo(id,0/*jobname*/,0/*user*/,0/*queue*/,0/*host*/,ALL_JOB);

	if ( lsfr < 0 )
	{
		return 0;
	}
	else if ( lsfr == 0 )
	{
		lsb_closejobinfo();
		return 0;
	}
	else
	{
		jobInfoEnt const * jie = lsb_readjobinfo(&lsfr);

		if ( ! jie )
		{
			lsb_closejobinfo();
			return 0;
		}

		int64_t const status = jie->status;
		lsb_closejobinfo();

		return status;
	}
}

std::string getUserName()
{
	uid_t uid = geteuid();
	long const maxpwlen = sysconf(_SC_GETPW_R_SIZE_MAX);
	// std::cerr << "maxpwlen " << maxpwlen << std::endl;
	::libmaus2::autoarray::AutoArray<char> Abuf(maxpwlen);
	struct passwd pwd, * ppwd;
	int const fail = getpwuid_r(uid,&pwd,Abuf.get(),maxpwlen,&ppwd);

	if ( ! fail )
	{
		return std::string(pwd.pw_name);
	}
	else
	{
		::libmaus2::exception::LibMausException se;
		se.getStream() << "Failed to get username: " << strerror(errno) << std::endl;
		se.finish();
		throw se;
	}
}

std::map < int64_t, int64_t> libmaus2::lsf::LSFProcess::getIntStates()
{
	::libmaus2::parallel::ScopeLock lock(::libmaus2::lsf::LSF::lsflock);

	std::string const username = getUserName();
	::libmaus2::util::WriteableString W(username);
	int lsfr = lsb_openjobinfo(0,0/*jobname*/,W.A.begin()/*user*/,0/*queue*/,0/*host*/,ALL_JOB);

	if ( lsfr < 0 )
	{
		::libmaus2::exception::LibMausException se;
		se.getStream() << "lsb_openjobinfo failed." << std::endl;
		se.finish();
		throw se;
	}

	std::map<int64_t,int64_t> M;

	while ( lsfr )
	{
		jobInfoEnt const * jie = lsb_readjobinfo(&lsfr);

		if ( ! jie )
		{
			lsb_closejobinfo();

			::libmaus2::exception::LibMausException se;
			se.getStream() << "lsb_readjobinfo failed." << std::endl;
			se.finish();
			throw se;
		}

		int64_t const jobId = jie->jobId;
		int64_t const state = jie->status;
		M [ jobId ] = state;
		// std::cerr << "jobId\t" << jobId << "\tstate\t" << state << std::endl;
	}

	lsb_closejobinfo();

	return M;
}

#endif
