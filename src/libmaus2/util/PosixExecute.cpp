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

#include <libmaus2/util/PosixExecute.hpp>

#include <string>
#include <cerrno>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/util/ArgInfo.hpp>
#include <libmaus2/util/GetFileSize.hpp>
#include <libmaus2/aio/FileRemoval.hpp>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

/* According to POSIX.1-2001 */
#include <sys/select.h>
/* According to earlier standards */
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

int libmaus2::util::PosixExecute::getTempFile(std::string const stemplate, std::string & filename)
{
	::libmaus2::autoarray::AutoArray<char> Atemplate(stemplate.size()+1);
	std::copy ( stemplate.begin(), stemplate.end(), Atemplate.get() );
	int const fd = mkstemp ( Atemplate.get() );

	if ( fd < 0 )
	{
		::libmaus2::exception::LibMausException se;
		se.getStream() << "Failed in mkstemp: " << strerror(errno);
		se.finish();
		throw se;
	}

	filename = Atemplate.get();

	return fd;
}

std::string libmaus2::util::PosixExecute::getBaseName(std::string const & name)
{
	::libmaus2::autoarray::AutoArray<char> Aname(name.size()+1);
	std::copy ( name.begin(), name.end(), Aname.get() );
	char * bn = basename(Aname.get());
	return std::string ( bn );
}

std::string libmaus2::util::PosixExecute::getProgBaseName(::libmaus2::util::ArgInfo const & arginfo)
{
	return getBaseName ( arginfo.progname );
}

int libmaus2::util::PosixExecute::getTempFile(::libmaus2::util::ArgInfo const & arginfo, std::string & filename)
{
	::std::ostringstream prefixstr;
	prefixstr << "/tmp/" << getProgBaseName(arginfo) << "_XXXXXX";
	return getTempFile(prefixstr.str(),filename);
}

std::string libmaus2::util::PosixExecute::loadFile(std::string const filename)
{
	uint64_t const len = ::libmaus2::util::GetFileSize::getFileSize(filename);
	std::ifstream istr(filename.c_str(),std::ios::binary);
	::libmaus2::autoarray::AutoArray<char> data(len,false);
	istr.read ( data.get(), len );
	assert ( istr );
	assert ( istr.gcount() == static_cast<int64_t>(len) );
	return std::string(data.get(),data.get()+data.size());
}

void libmaus2::util::PosixExecute::executeOld(::libmaus2::util::ArgInfo const & arginfo, std::string const & command, std::string & out, std::string & err)
{
	std::string stdoutfilename;
	std::string stderrfilename;

	int const stdoutfd = getTempFile(arginfo,stdoutfilename);
	int const stderrfd = getTempFile(arginfo,stderrfilename);

	pid_t const pid = fork();

	if ( pid == -1 )
	{
		int const error = errno;

		close(stdoutfd);
		close(stderrfd);
		libmaus2::aio::FileRemoval::removeFile ( stdoutfilename );
		libmaus2::aio::FileRemoval::removeFile ( stderrfilename );

		::libmaus2::exception::LibMausException se;
		se.getStream() << "Failed to fork(): " << strerror(error);
		se.finish();
		throw se;
	}

	/* child */
	if ( pid == 0 )
	{
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
		int const nullfd = open("/dev/null",O_RDONLY);
		dup2(nullfd,STDIN_FILENO);
		dup2(stdoutfd,STDOUT_FILENO);
		dup2(stderrfd,STDERR_FILENO);
		int const ret = system ( command.c_str() );
		_exit(ret);
	}
	else
	{
		close(stdoutfd);
		close(stderrfd);

		int status = 0;
		pid_t const wpid = waitpid(pid, &status, 0);
		assert ( wpid == pid );

		out = loadFile(stdoutfilename);
		err = loadFile(stderrfilename);
	}
}

int libmaus2::util::PosixExecute::setNonBlockFlag (int desc, bool on)
{
	int oldflags = fcntl (desc, F_GETFL, 0);
	/* If reading the flags failed, return error indication now. */
	if (oldflags < 0)
		return oldflags;
	/* Set just the flag we want to set. */
	if (on)
		oldflags |= O_NONBLOCK;
	else
		oldflags &= ~O_NONBLOCK;
	/* Store modified flag word in the descriptor. */
	return fcntl (desc, F_SETFL, oldflags);
}

template<typename _type>
struct LocalAutoArray
{
	typedef _type type;

	size_t const n;
	type * const A;

	LocalAutoArray(size_t const rn)
	: n(rn), A(new type[n])
	{
	}
	~LocalAutoArray()
	{
		delete [] A;
	}

	type * get()
	{
		return A;
	}

	type const * get() const
	{
		return A;
	}

	type * begin()
	{
		return get();
	}

	type const * begin() const
	{
		return get();
	}

	size_t size() const
	{
		return n;
	}

	type * end()
	{
		return begin()+size();
	}

	type const * end() const
	{
		return begin()+size();
	}

	type & operator[](size_t const i)
	{
		return A[i];
	}

	type const & operator[](size_t const i) const
	{
		return A[i];
	}
};

#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/util/shared_ptr.hpp>

struct Pipe
{
	typedef Pipe this_type;
	typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
	typedef libmaus2::util::unique_ptr<this_type>::type shared_ptr_type;

	int fd[2];

	Pipe(bool const donotthrow)
	{
		fd[0] = -1;
		fd[1] = -1;

		if ( pipe(&fd[0]) != 0 )
		{
			if ( donotthrow )
			{
				std::cerr << "pipe() failed: " << strerror(errno) << std::endl;
				fd[0] = -1;
				fd[1] = -1;
			}
			else
			{
				::libmaus2::exception::LibMausException se;
				se.getStream() << "pipe() failed: " << strerror(errno);
				se.finish();
				throw se;
			}
		}
	}
	~Pipe()
	{
		closeReadEnd();
		closeWriteEnd();
	}

	static int open(unique_ptr_type & P, bool const donotthrow)
	{
		try
		{
			unique_ptr_type T(new Pipe(donotthrow));

			if ( T->fd[0] < 0 )
				return -1;
			else
			{
				P = UNIQUE_PTR_MOVE(T);
				return 0;
			}
		}
		catch(...)
		{
			if ( donotthrow )
				return -1;
			else
				throw;
		}
	}

	void closeReadEnd()
	{
		if ( fd[0] >= 0 )
		{
			::close(fd[0]);
			fd[0] = -1;
		}
	}

	void closeWriteEnd()
	{
		if ( fd[1] >= 0 )
		{
			::close(fd[1]);
			fd[1] = -1;
		}
	}
};

static int doClose(int fd)
{
	while ( true )
	{
		int const r = ::close(fd);

		if ( r < 0 )
		{
			switch ( errno )
			{
				case EINTR:
					break;
				default:
					return r;
			}
		}
		else
		{
			return r;
		}
	}
}

int parseEscapeCode(std::string const & command, size_t const j)
{
	if ( j+1 < command.size() )
	{
		switch ( command[j+1] )
		{
			case '0':
				// extend this to general numbers
				return (0);
				break;
			case 'a':
				return ('\a');
				break;
			case 'b':
				return ('\b');
				break;
			case 't':
				return ('\t');
				break;
			case 'n':
				return ('\n');
				break;
			case 'v':
				return ('\v');
				break;
			case 'f':
				return ('\f');
				break;
			case 'r':
				return ('\r');
				break;
			default:
				return -1;
		}
	}
	else
	{
		return -1;
	}
}

std::vector<std::string> parseCommand(std::string const & command)
{
	uint64_t i = 0;
	std::vector<std::string> V;

	while ( i < command.size() )
	{
		while ( i < command.size() && isspace(command[i]) )
			++i;

		uint64_t j = i;

		std::ostringstream ostr;
		while ( j < command.size() && !isspace(command[j]) )
		{
			if ( command[j] == '\\' )
			{
				int const r = parseEscapeCode(command,j);
				if ( r >= 0 )
					ostr.put(r);
				j += 2;
			}
			else if ( command[j] == '"' )
			{
				++j;
				while ( j < command.size() && command[j] != '"' )
				{
					if ( command[j] == '\\' )
					{
						int const r = parseEscapeCode(command,j);
						if ( r >= 0 )
							ostr.put(r);
						j += 2;
					}
					else
					{
						ostr.put(command[j++]);
					}
				}
				++j;
			}
			else if ( command[j] == '\'' )
			{
				++j;
				while ( j < command.size() && command[j] != '\'' )
					ostr.put(command[j++]);
				++j;
			}
			else
			{
				ostr.put(command[j++]);
			}
		}

		std::string const s = ostr.str();
		if ( s.size() )
			V.push_back(s);

		i = j;
	}

	return V;
}

int libmaus2::util::PosixExecute::execute(std::string const & command, std::string & out, std::string & err, bool const donotthrow)
{
	libmaus2::parallel::ScopePosixMutex slock(libmaus2::util::PosixExecute::lock);
	char stderrfn[] = "/tmp/libmaus2::util::PosixExecute::execute_XXXXXX";
	char stdoutfn[] = "/tmp/libmaus2::util::PosixExecute::execute_XXXXXX";
	bool stderrfnvalid = false;
	bool stdoutfnvalid = false;
	int stderrfd = -1;
	int stdoutfd = -1;
	int nullfd = -1;
	int returncode = EXIT_SUCCESS;
	pid_t child = -1;
	int error = 0;
	char * tempmemerr = NULL;
	char * tempmemout = NULL;
	struct stat staterr;
	struct stat statout;
	size_t errread = 0;
	size_t outread = 0;
	int stdindup = -1;
	int stdoutdup = -1;
	int stderrdup = -1;
	bool copybackin = false;
	bool copybackout = false;
	bool copybackerr = false;
	/* int r; */
	std::vector<std::string> V = parseCommand(command);
	char * argmem = NULL;
	size_t argmemsize = 0;
	char * argmemt = NULL;
	char ** argptrs = NULL;

	for ( uint64_t i = 0; i < V.size(); ++i )
	{
		// std::cerr << "command[" << i << "]=" << V[i] << std::endl;
		argmemsize += V[i].size()+1;
	}

	argmem = (char *)malloc(argmemsize);

	if ( ! argmem )
	{
		returncode = EXIT_FAILURE;
		error = ENOMEM;
		goto cleanup;
	}

	memset(argmem,0,argmemsize);

	argptrs = (char **)malloc((V.size()+1) * sizeof(char *));

	if ( ! argptrs )
	{
		returncode = EXIT_FAILURE;
		error = ENOMEM;
		goto cleanup;
	}

	memset(argptrs,0,(V.size()+1) * sizeof(char *));

	argmemt = argmem;
	for ( uint64_t i = 0; i < V.size(); ++i )
	{
		memcpy(argmemt,V[i].c_str(),V[i].size());
		argptrs[i] = argmemt;
		argmemt += V[i].size()+1;
	}

	#if 0
	std::cerr << "calling " << argptrs[0] << std::endl;
	for ( uint64_t i = 1; i < V.size(); ++i )
		std::cerr << "arg[" << i << "]=" << argptrs[i] << std::endl;
	assert ( argptrs[V.size()] == NULL );
	#endif

	if ( (stderrfd = mkstemp(&stderrfn[0])) < 0 )
	{
		returncode = EXIT_FAILURE;
		error = errno;
		goto cleanup;
	}
	else
	{
		stderrfnvalid = true;
	}

	if ( (stdoutfd = mkstemp(&stdoutfn[0])) < 0 )
	{
		returncode = EXIT_FAILURE;
		error = errno;
		goto cleanup;
	}
	else
	{
		stdoutfnvalid = true;
	}

	if ( (nullfd = open("/dev/null",O_RDONLY)) < 0 )
	{
		returncode = EXIT_FAILURE;
		error = errno;
		goto cleanup;
	}

	if ( (stdindup = dup(STDIN_FILENO)) < 0 )
	{
		returncode = EXIT_FAILURE;
		error = errno;
		goto cleanup;
	}

	if ( (stdoutdup = dup(STDOUT_FILENO)) < 0 )
	{
		returncode = EXIT_FAILURE;
		error = errno;
		goto cleanup;
	}

	if ( (stderrdup = dup(STDERR_FILENO)) < 0 )
	{
		returncode = EXIT_FAILURE;
		error = errno;
		goto cleanup;
	}

	if ( doClose(STDIN_FILENO) < 0 )
	{
		returncode = EXIT_FAILURE;
		error = errno;
		goto cleanup;
	}
	else
	{
		copybackin = true;
	}

	if ( doClose(STDOUT_FILENO) < 0 )
	{
		returncode = EXIT_FAILURE;
		error = errno;
		goto cleanup;
	}
	else
	{
		copybackout = true;
	}

	if ( doClose(STDERR_FILENO) < 0 )
	{
		returncode = EXIT_FAILURE;
		error = errno;
		goto cleanup;
	}
	else
	{
		copybackerr = true;
	}

	if ( dup2(nullfd,STDIN_FILENO) < 0 )
	{
		returncode = EXIT_FAILURE;
		error = errno;
		goto cleanup;
	}
	if ( dup2(stdoutfd,STDOUT_FILENO) < 0 )
	{
		returncode = EXIT_FAILURE;
		error = errno;
		goto cleanup;
	}
	if ( dup2(stderrfd,STDERR_FILENO) < 0 )
	{
		returncode = EXIT_FAILURE;
		error = errno;
		goto cleanup;
	}

	#if 0
	r = system(command.c_str());

	if ( r == -1 )
	{
		returncode = EXIT_FAILURE;
		error = errno;
		goto cleanup;
	}
	else
	{
		returncode = WEXITSTATUS(r);
	}
	#else
	child = vfork();

	if ( child < 0 )
	{
		returncode = EXIT_FAILURE;
		error = errno;
		goto cleanup;
	}

	if ( child == 0 )
	{
		try
		{
			#if 0
			int const r = system(command.c_str());
			_exit(WEXITSTATUS(r));
			#endif

			execvp(argptrs[0], argptrs);

			_exit(EXIT_FAILURE);
		}
		catch(...)
		{
			_exit(EXIT_FAILURE);
		}
	}

	while ( true )
	{
		int status = 0;

		pid_t const r = waitpid(child, &status, 0);

		if ( r < 0 )
		{
			int const lerror = errno;

			if ( lerror == EAGAIN || lerror == EINTR )
			{

			}
			else
			{
				returncode = EXIT_FAILURE;
				error = errno;
				goto cleanup;
			}
		}
		if ( r == child )
		{
			if ( WIFEXITED(status) )
			{
				returncode = WEXITSTATUS(status);
			}
			else
			{
				returncode = EXIT_FAILURE;
			}

			break;
		}
	}
	#endif

	if ( lseek(stderrfd,0,SEEK_SET) != 0 )
	{
		returncode = EXIT_FAILURE;
		error = errno;
		goto cleanup;
	}

	if ( lseek(stdoutfd,0,SEEK_SET) != 0 )
	{
		returncode = EXIT_FAILURE;
		error = errno;
		goto cleanup;
	}

	if ( fstat(stderrfd,&staterr) < 0 )
	{
		returncode = EXIT_FAILURE;
		error = errno;
		goto cleanup;
	}
	if ( fstat(stdoutfd,&statout) < 0 )
	{
		returncode = EXIT_FAILURE;
		error = errno;
		goto cleanup;
	}

	if ( (tempmemerr = (char *)malloc(staterr.st_size)) == NULL )
	{
		returncode = EXIT_FAILURE;
		error = errno;
		goto cleanup;
	}
	if ( (tempmemout = (char *)malloc(statout.st_size)) == NULL )
	{
		returncode = EXIT_FAILURE;
		error = errno;
		goto cleanup;
	}

	while ( static_cast<ssize_t>(errread) < static_cast<ssize_t>(staterr.st_size) )
	{
		size_t toread = staterr.st_size - errread;
		ssize_t const r = ::read(stderrfd,tempmemerr+errread,toread);

		if ( r < 0 )
		{
			if ( errno == EAGAIN || errno == EINTR )
			{

			}
			else
			{
				returncode = EXIT_FAILURE;
				error = errno;
				goto cleanup;
			}
		}
		else if ( r == 0 )
		{
			returncode = EXIT_FAILURE;
			error = errno;
			goto cleanup;
		}
		else
		{
			errread += r;
		}
	}
	while ( static_cast<ssize_t>(outread) < static_cast<ssize_t>(statout.st_size) )
	{
		size_t toread = statout.st_size - outread;
		ssize_t const r = ::read(stdoutfd,tempmemout+outread,toread);

		if ( r < 0 )
		{
			if ( errno == EAGAIN || errno == EINTR )
			{

			}
			else
			{
				returncode = EXIT_FAILURE;
				error = errno;
				goto cleanup;
			}
		}
		else if ( r == 0 )
		{
			returncode = EXIT_FAILURE;
			error = errno;
			goto cleanup;
		}
		else
		{
			outread += r;
		}
	}

	try
	{
		out = std::string(statout.st_size,' ');
		err = std::string(staterr.st_size,' ');

		for ( ssize_t i = 0; i < statout.st_size; ++i )
			out[i] = tempmemout[i];
		for ( ssize_t i = 0; i < staterr.st_size; ++i )
			err[i] = tempmemerr[i];
	}
	catch(...)
	{
		returncode = EXIT_FAILURE;
		error = ENOMEM;
		goto cleanup;
	}

	cleanup:

	if ( copybackin )
	{
		if ( dup2(stdindup,STDIN_FILENO) < 0 )
		{
			if ( returncode >= 0 )
			{
				returncode = EXIT_FAILURE;
				error = errno;
			}
		}
	}
	if ( copybackout )
	{
		if ( dup2(stdoutdup,STDOUT_FILENO) < 0 )
		{
			if ( returncode >= 0 )
			{
				returncode = EXIT_FAILURE;
				error = errno;
			}
		}
	}
	if ( copybackerr )
	{
		if ( dup2(stderrdup,STDERR_FILENO) < 0 )
		{
			if ( returncode >= 0 )
			{
				returncode = EXIT_FAILURE;
				error = errno;
			}
		}
	}
	if ( stdindup >= 0 )
	{
		doClose(stdindup);
		stdindup = -1;
	}
	if ( stdoutdup >= 0 )
	{
		doClose(stdoutdup);
		stdoutdup = -1;
	}
	if ( stderrdup >= 0 )
	{
		doClose(stderrdup);
		stderrdup = -1;
	}

	if ( stderrfd >= 0 )
	{
		doClose(stderrfd);
		stderrfd = -1;
	}
	if ( stdoutfd >= 0 )
	{
		doClose(stdoutfd);
		stdoutfd = -1;
	}
	if ( nullfd >= 0 )
	{
		doClose(nullfd);
		nullfd = -1;
	}

	if ( stderrfnvalid )
		libmaus2::aio::FileRemoval::removeFile(&stderrfn[0]);
	if ( stdoutfnvalid )
		libmaus2::aio::FileRemoval::removeFile(&stdoutfn[0]);

	if ( tempmemerr )
	{
		free(tempmemerr);
		tempmemerr = NULL;
	}
	if ( tempmemout )
	{
		free(tempmemout);
		tempmemout = NULL;
	}

	if ( argmem )
	{
		::free(argmem);
		argmem = NULL;
	}

	if ( argptrs )
	{
		::free(argptrs);
		argptrs = NULL;
	}

	if ( returncode != EXIT_SUCCESS )
	{
		if ( donotthrow )
		{
			try
			{
				if ( error == 0 )
					std::cerr << "libmaus2::util::PosixExecute::execute(): \"" << command << "\" exited with status " << returncode << std::endl;
				else
					std::cerr << "libmaus2::util::PosixExecute::execute() failed: " << strerror(error) << std::endl;
			}
			catch(...)
			{
			}
		}
		else
		{
			libmaus2::exception::LibMausException lme;
			if ( error == 0 )
				lme.getStream() << "libmaus2::util::PosixExecute::execute(): \"" << command << "\" exited with status " << returncode << std::endl;
			else
				lme.getStream() << "libmaus2::util::PosixExecute::execute() failed: " << strerror(error) << std::endl;
			lme.finish();
			throw lme;
		}
	}

	return returncode;
}

#if 0
int libmaus2::util::PosixExecute::execute(std::string const & command, std::string & out, std::string & err, bool const donotthrow)
{
	try
	{
		Pipe::unique_ptr_type Pstdoutpipe;
		if ( Pipe::open(Pstdoutpipe,donotthrow) < 0 )
			return EXIT_FAILURE;
		Pipe::unique_ptr_type Pstderrpipe;
		if ( Pipe::open(Pstderrpipe,donotthrow) < 0 )
			return EXIT_FAILURE;

		// fork off process
		pid_t const pid = fork();

		// fork failure?
		if ( pid == -1 )
		{
			int const error = errno;

			if ( donotthrow )
			{
				std::cerr << "Failed to fork(): " << strerror(error) << std::endl;
				return EXIT_FAILURE;
			}
			else
			{
				::libmaus2::exception::LibMausException se;
				se.getStream() << "Failed to fork(): " << strerror(error) << std::endl;
				se.finish();
				throw se;
			}
		}

		/* child */
		if ( pid == 0 )
		{
			try
			{
				// close read ends
				Pstdoutpipe->closeReadEnd();
				Pstderrpipe->closeReadEnd();

				// open dev null
				int nullfd = open("/dev/null",O_RDONLY);

				if ( nullfd < 0 )
					return EXIT_FAILURE;

				dup2(nullfd,STDIN_FILENO);
				dup2(Pstdoutpipe->fd[1],STDOUT_FILENO);
				dup2(Pstderrpipe->fd[1],STDERR_FILENO);

				Pstdoutpipe->closeWriteEnd();
				Pstderrpipe->closeWriteEnd();
				::close(nullfd);
				nullfd = -1;

				// call system
				int const ret = system ( command.c_str() );

				#if 0
				std::ostringstream ostr;
				ostr << "After return code " << ret << std::endl;
				std::string ostrstr = ostr.str();
				write ( STDERR_FILENO, ostrstr.c_str(), ostrstr.size() );
				#endif

				_exit(WEXITSTATUS(ret));
			}
			catch(std::exception const & ex)
			{
				try
				{
					std::cerr << ex.what() << std::endl;
				}
				catch(...)
				{

				}

				_exit(EXIT_FAILURE);
			}
			catch(...)
			{
				try
				{
					std::cerr << "Unknown exception in PosixExecute child process." << std::endl;
				}
				catch(...)
				{

				}

				_exit(EXIT_FAILURE);
			}
		}
		else
		{
			Pstdoutpipe->closeWriteEnd();
			Pstderrpipe->closeWriteEnd();

			setNonBlockFlag ( Pstdoutpipe->fd[0], true );
			setNonBlockFlag ( Pstderrpipe->fd[0], true );

			bool done = false;
			int status;

			LocalAutoArray<char> LB(8*8192);

			std::vector<char> outv;
			std::vector<char> errv;

			while ( ! done )
			{
				fd_set fileset;
				FD_ZERO(&fileset);
				FD_SET(Pstdoutpipe->fd[0],&fileset);
				FD_SET(Pstderrpipe->fd[0],&fileset);
				int nfds = std::max(Pstdoutpipe->fd[0],Pstderrpipe->fd[0])+1;

				struct timeval timeout = { 0,0 };
				int selret = select(nfds, &fileset, 0, 0, &timeout);

				if ( selret > 0 )
				{
					// std::cerr << "Select returned " << selret << std::endl;

					if ( FD_ISSET(Pstdoutpipe->fd[0],&fileset) )
					{
						ssize_t red = read ( Pstdoutpipe->fd[0], LB.begin(), LB.size() );
						// std::cerr << "Got " << red << " from stdout." << std::endl;
						if ( red > 0 )
							for ( ssize_t i = 0; i < red; ++i )
								outv.push_back(LB[i]);
					}
					if ( FD_ISSET(Pstderrpipe->fd[0],&fileset) )
					{
						ssize_t red = read ( Pstderrpipe->fd[0], LB.begin(), LB.size() );
						// std::cerr << "Got " << red << " from stderr." << std::endl;
						if ( red > 0 )
							for ( ssize_t i = 0; i < red; ++i )
								errv.push_back(LB[i]);
					}
				}

				status = 0;
				pid_t const wpid = waitpid(pid, &status, WNOHANG);

				if ( wpid == pid )
				{
					done = true;
				}
				else if ( wpid < 0 )
				{
					done = true;
				}
				else
				{
					if ( ! selret )
					{
						struct timespec waittime = { 0, 100000000 };
						nanosleep(&waittime,0);
					}
				}
			}

			setNonBlockFlag ( Pstdoutpipe->fd[0], false );
			setNonBlockFlag ( Pstderrpipe->fd[0], false );

			ssize_t red = -1;

			// read rest from pipes
			while ( (red = read ( Pstdoutpipe->fd[0], LB.begin(), LB.size() ) > 0 ) )
			{
				// std::cerr << "Got " << red << " from stdout in final." << std::endl;
				for ( ssize_t i = 0; i < red; ++i )
					outv.push_back(LB[i]);
			}
			while ( (red = read ( Pstderrpipe->fd[0], LB.begin(), LB.size() ) > 0 ) )
			{
				// std::cerr << "Got " << red << " from stderr in final." << std::endl;
				for ( ssize_t i = 0; i < red; ++i )
					errv.push_back(LB[i]);
			}

			// copy data
			out = std::string(outv.begin(),outv.end());
			err = std::string(errv.begin(),errv.end());

			if ( ! WIFEXITED(status) )
			{
				if ( donotthrow )
				{
					std::cerr << "Calling process " << command << " failed." << std::endl;
					return EXIT_FAILURE;
				}
				else
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "Calling process " << command << " failed.";
					se.finish();
					throw se;
				}
			}
			else
			{
				return WEXITSTATUS(status);
			}
		}
	}
	catch(...)
	{
		if ( donotthrow )
			return EXIT_FAILURE;
		else
			throw;
	}
}
#endif
