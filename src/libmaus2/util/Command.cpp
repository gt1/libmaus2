/*
    libmaus2
    Copyright (C) 2017 German Tischler

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
#include <libmaus2/util/Command.hpp>
#include <libmaus2/util/WriteableString.hpp>
#include <libmaus2/util/GetFileSize.hpp>
#include <libmaus2/timing/RealTimeClock.hpp>
#include <libmaus2/aio/InputOutputStreamInstance.hpp>

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#if defined(__APPLE__)
#include <crt_externs.h>
#endif

#if defined(__NetBSD__) || defined(__FreeBSD__)
extern char **environ;
#endif

static char ** getEnviron()
{
	#if defined(__APPLE__)
	return *_NSGetEnviron();
	#else
	return environ;
	#endif
}

int libmaus2::util::Command::dispatch(std::string const & fn) const
{
	{
		libmaus2::aio::OutputStreamInstance OSI(fn);
		OSI << script;
		OSI.flush();

		if ( ! OSI )
		{
			libmaus2::exception::LibMausException lme;
			lme.getStream() << "[E] failed to write script " << fn << std::endl;
			lme.finish();
			throw lme;
		}
	}

	std::vector<std::string> args;
	args.push_back(shell);
	args.push_back(fn);

	libmaus2::autoarray::AutoArray < libmaus2::util::WriteableString::unique_ptr_type > AW(args.size());
	for ( uint64_t i = 0; i < args.size(); ++i )
	{
		libmaus2::util::WriteableString::unique_ptr_type tptr(new libmaus2::util::WriteableString(args[i]));
		AW[i] = UNIQUE_PTR_MOVE(tptr);
	}
	libmaus2::autoarray::AutoArray < char * > AA(args.size()+1);
	for ( uint64_t i = 0; i < args.size(); ++i )
		AA[i] = AW[i]->A.begin();
	AA[args.size()] = 0;

	std::string const com = args[0];

	libmaus2::timing::RealTimeClock rtc;
	rtc.start();

	pid_t const pid = fork();

	if ( pid == static_cast<pid_t>(-1) )
	{
		int const error = errno;
		libmaus2::exception::LibMausException lme;
		lme.getStream() << "[E] fork failed: " << strerror(error) << std::endl;
		lme.finish();
		throw lme;
	}
	if ( pid == 0 )
	{
		int const fdout = ::open(out.c_str(),O_CREAT|O_TRUNC|O_RDWR,0600);
		if ( fdout == -1 )
		{
			int const error = errno;
			std::cerr << "[E] failed to open " << out << ":" << strerror(error) << std::endl;
			_exit(EXIT_FAILURE);
		}
		int const fderr = ::open(err.c_str(),O_CREAT|O_TRUNC|O_RDWR,0600);
		if ( fderr == -1 )
		{
			int const error = errno;
			std::cerr << "[E] failed to open " << err << ":" << strerror(error) << std::endl;
			_exit(EXIT_FAILURE);
		}
		int const fdin = ::open(in.c_str(),O_RDONLY);
		if ( fdin == -1 )
		{
			int const error = errno;
			std::cerr << "[E] failed to open " << in << ":" << strerror(error) << std::endl;
			_exit(EXIT_FAILURE);
		}

		if ( ::close(STDOUT_FILENO) == -1 )
			_exit(EXIT_FAILURE);
		if ( ::close(STDERR_FILENO) == -1 )
			_exit(EXIT_FAILURE);
		if ( ::close(STDIN_FILENO) == -1 )
			_exit(EXIT_FAILURE);

		if ( dup2 ( fdout, STDOUT_FILENO ) == -1 )
			_exit(EXIT_FAILURE);
		if ( dup2 ( fderr, STDERR_FILENO ) == -1 )
			_exit(EXIT_FAILURE);
		if ( dup2 ( fdin, STDIN_FILENO ) == -1 )
			_exit(EXIT_FAILURE);

		execve(com.c_str(),AA.begin(),getEnviron());
		_exit(EXIT_FAILURE);
	}

	int status = 0;
	int const r = waitpid(pid,&status,0/*options */);
	if ( r == -1 )
	{
		int const error = errno;
		libmaus2::exception::LibMausException lme;
		lme.getStream() << "[E] waitpid failed: " << strerror(error) << std::endl;
		lme.finish();
		throw lme;
	}

	double const etime = rtc.getElapsedSeconds();

	{
		libmaus2::aio::InputOutputStreamInstance IOSI(err,std::ios::in|std::ios::out);
		uint64_t const fs = libmaus2::util::GetFileSize::getFileSize(IOSI);
		IOSI.clear();
		IOSI.seekp(fs);

		IOSI << etime << "\t" << libmaus2::timing::RealTimeClock::formatTime(etime) << std::endl;
	}

	if ( ! ( WIFEXITED(status) && (WEXITSTATUS(status) == 0) ) )
	{
		libmaus2::exception::LibMausException lme;
		std::ostream & errstr = lme.getStream();

		errstr << "[E] " << args[0] << " failed" << std::endl;

		if ( WIFEXITED(status) )
		{
			errstr << "[E] exit code " << WEXITSTATUS(status) << std::endl;
		}

		lme.finish();

		throw lme;
	}

	return EXIT_SUCCESS;
}

std::ostream & libmaus2::util::operator<<(std::ostream & out, Command const & C)
{
	out << "Command("
		<< "in=" << C.in
		<< ",out=" << C.out
		<< ",err=" << C.err
		<< ",numattempts=" << C.numattempts
		<< ",maxattempts=" << C.maxattempts
		<< ",completed=" << C.completed
		<< ",ignorefail=" << C.ignorefail
		<< ",deepsleep=" << C.deepsleep
		<< ",modcall=" << C.modcall
		<< ",shell=" << C.shell
		<< ",script=" << C.script
		<< "])";
	return out;
}
