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

#include <libmaus2/util/StackTrace.hpp>
#if defined(__linux__)
#include <libmaus2/util/PosixExecute.hpp>
#endif

#if defined(__linux__)
static std::string chomp(std::string s)
{
        while ( s.size() && isspace(s[s.size()-1]) )
                s = s.substr(0,s.size()-1);
        return s;
}
#endif

libmaus2::util::StackTrace::~StackTrace() {}

std::string libmaus2::util::StackTrace::toString(bool const
#if defined(__linux)
translate
#endif
) const
{
        std::ostringstream ostr;

        #if defined(__linux__)
        if ( translate )
        {
                for ( uint64_t i = 0; i < trace.size(); ++i )
                {
                        std::pair < std::string, std::string > A = components(trace[i],'[',']');
                        std::pair < std::string, std::string > B = components(A.first,'(',')');
                        std::string const & execname = B.first;
                        std::string const & addr = A.second;
                        
                        std::ostringstream comlinestr;
                        comlinestr << "/usr/bin/addr2line" << " -e " << execname << " " << addr;
                        std::string const comline = comlinestr.str();
                        
                        std::string addrout,addrerr;
                        ::libmaus2::util::PosixExecute::execute(comline,addrout,addrerr,true /* do not throw exceptions */);
                        addrout = chomp(addrout);
                        
                        /*
                        ostr << ":::" << execname << ":::\n";
                        ostr << ":::" << addr << ":::\n";
                        ostr << ":::" << comline << ":::\n";
                        ostr << ":::" << addrout << ":::\n";
                        ostr << trace[i] << "\n";
                        */
                        
                        if ( B.second.find_last_of('+') != std::string::npos )
                        {
                        	std::string const mangled = B.second.substr(0,B.second.find_last_of('+'));
                        	std::string const offset = B.second.substr(B.second.find_last_of('+')+1);
                        	B.second = libmaus2::util::Demangle::demangleName(mangled) + "+" + offset;
                        }
                        
                        ostr << B.first << "(" << B.second << ")" << "[" << A.second << ":" << addrout << "]\n";
                }
        }
        else
        #endif
        {
                for ( uint64_t i = 0; i < trace.size(); ++i )
                        ostr << trace[i] << std::endl;        
        }

        return ostr.str();
}

void libmaus2::util::StackTrace::simpleStackTrace(std::ostream &
#if defined(LIBMAUS2_HAVE_BACKTRACE)
ostr
#endif
)
{
	#if defined(LIBMAUS2_HAVE_BACKTRACE)
	unsigned int const depth = 20;
	void *array[depth];
	size_t const size = backtrace(array,depth);
	char ** strings = backtrace_symbols(array,size);

	for ( size_t i = 0; i < size; ++i )
		ostr << "[" << i << "]" << strings[i] << std::endl;

	free(strings);
	#endif
}

libmaus2::util::StackTrace::StackTrace() : trace()
{
	#if defined(LIBMAUS2_HAVE_BACKTRACE)
	unsigned int const depth = 20;
	void *array[depth];
	size_t const size = backtrace(array,depth);
	char ** strings = backtrace_symbols(array,size);

	for ( size_t i = 0; i < size; ++i )
		trace.push_back(strings[i]);

	free(strings);
	#endif
}

std::string libmaus2::util::StackTrace::getExecPath()
{
	#if defined(_linux__)
	char buf[PATH_MAX+1];
	std::fill(
		&buf[0],&buf[sizeof(buf)/sizeof(buf[0])],0);
	if ( readlink("/proc/self/exe", &buf[0], PATH_MAX) < 0 )
		throw std::runtime_error("readlink(/proc/self/exe) failed.");
	return std::string(&buf[0]);
	#else
	return "program";
	#endif
}

std::pair < std::string, std::string > libmaus2::util::StackTrace::components(std::string line, char const start, char const end)
{
	uint64_t i = line.size();
	
	while ( i != 0 )
		if ( line[--i] == end )
			break;
	
	if ( line[i] != end || i == 0 )
		return std::pair<std::string,std::string>(std::string(),std::string());
	
	uint64_t const addrend = i;

	while ( i != 0 )
		if ( line[--i] == start )
			break;
		
	return std::pair<std::string,std::string>(
		line.substr(0,i),
		line.substr(i+1, addrend-(i+1) ));
}

std::string libmaus2::util::StackTrace::getStackTrace(bool translate)
{
	StackTrace st;
	return st.toString(translate);
}
