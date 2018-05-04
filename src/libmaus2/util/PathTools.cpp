/*
    libmaus2
    Copyright (C) 2018 German Tischler-Höhle

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
#include <libmaus2/util/PathTools.hpp>

void libmaus2::util::PathTools::lchdir(std::string const & s)
{
	bool running = true;
	while ( running )
	{
		int const r = chdir(s.c_str());

		if ( r == 0 )
			running = false;
		else
		{
			int const error = errno;

			switch ( error )
			{
				case EAGAIN:
				case EINTR:
					break;
				default:
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "[E] chdir(" << s << "): " << strerror(error) << std::endl;
					lme.finish();
					throw lme;
			}
		}
	}
}

std::string libmaus2::util::PathTools::sbasename(std::string const & s)
{
	libmaus2::autoarray::AutoArray<char> A(s.size()+1,false);
	std::copy(s.begin(),s.end(),A.begin());
	A[s.size()] = 0;
	char * const c = basename(A.begin());
	std::string const b(c);
	return b;
}

std::string libmaus2::util::PathTools::sdirname(std::string const & s)
{
	libmaus2::autoarray::AutoArray<char> A(s.size()+1,false);
	std::copy(s.begin(),s.end(),A.begin());
	A[s.size()] = 0;
	char * const c = dirname(A.begin());
	std::string const b(c);
	return b;
}

std::string libmaus2::util::PathTools::getAbsPath(std::string const fn)
{
	if ( ! fn.size() || fn[0] == '/' )
		return fn;

	std::string const rundir = libmaus2::util::ArgInfo::getCurDir();
	std::string const d = libmaus2::util::ArgInfo::getDirName(fn);

	lchdir(d);

	std::string const absd = libmaus2::util::ArgInfo::getCurDir();

	lchdir(rundir);

	std::string const absp = absd + "/" + sbasename(fn);

	return absp;
}
