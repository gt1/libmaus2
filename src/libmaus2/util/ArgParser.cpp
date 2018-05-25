/*
    libmaus2
    Copyright (C) 2015 German Tischler

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
#include <libmaus2/util/ArgParser.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/util/PathTools.hpp>
#include <libmaus2/util/GetFileSize.hpp>

std::ostream & libmaus2::util::operator<<(std::ostream & out, libmaus2::util::ArgParser const & O)
{
	out << "ArgParser(";
	out << "progname=" << O.progname << ";";
	out << "kvargs={";
	for ( std::multimap<std::string,std::string>::const_iterator ita = O.kvargs.begin(); ita != O.kvargs.end(); ++ita )
		out << "(" << ita->first << "," << ita->second << ")";
	out << "};";
	out << "restargs=[";
	for ( uint64_t i = 0; i < O.restargs.size(); ++i )
		out << O.restargs[i] << ((i+1<O.restargs.size()) ? ";" : "");
	out << "]";
	out << ")";
	return out;
}

std::string libmaus2::util::ArgParser::getAbsProgName() const
{
	#if defined(__linux__)
	char buf[PATH_MAX+1];
	std::fill(
		&buf[0],&buf[sizeof(buf)/sizeof(buf[0])],0);
	if ( readlink("/proc/self/exe", &buf[0], PATH_MAX) < 0 )
		throw std::runtime_error("readlink(/proc/self/exe) failed.");
	return std::string(&buf[0]);
	#else
	// absolute path
	if ( progname.size() > 0 && progname[0] == '/' )
		return progname;

	// does progname contain a slash anywhere?
	bool containsSlash = false;
	for ( uint64_t i = 0; i < progname.size(); ++i )
		if ( progname[i] == '/' )
			containsSlash = true;
	if ( containsSlash )
		return libmaus2::util::PathTools::getCurDir() + "/" + progname;

	// otherwise try PATH
	char const * cPATH = getenv("PATH");
	while ( *cPATH != 0 )
	{
		char const * bPATH = cPATH;
		while ( *cPATH != 0 && *cPATH != ':' )
			++cPATH;
		assert ( *cPATH == 0 || *cPATH == ':' );

		std::string const pd(bPATH,cPATH);
		std::string const e = pd + "/" + progname;
		if ( libmaus2::util::GetFileSize::fileExists(e) )
			// should check whether e is executable
			return e;

		if ( *cPATH == ':' )
			++cPATH;
	}

	// not found
	return progname;
	#endif
}
