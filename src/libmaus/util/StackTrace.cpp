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

#include <libmaus/util/StackTrace.hpp>
#if defined(__linux__)
#include <libmaus/util/PosixExecute.hpp>
#endif

static std::string chomp(std::string s)
{
        while ( s.size() && isspace(s[s.size()-1]) )
                s = s.substr(0,s.size()-1);
        return s;
}

std::string libmaus::util::StackTrace::toString(bool const translate) const
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
                        ::libmaus::util::PosixExecute::execute(comline,addrout,addrerr,true /* do not throw exceptions */);
                        addrout = chomp(addrout);
                        
                        /*
                        ostr << ":::" << execname << ":::\n";
                        ostr << ":::" << addr << ":::\n";
                        ostr << ":::" << comline << ":::\n";
                        ostr << ":::" << addrout << ":::\n";
                        ostr << trace[i] << "\n";
                        */
                        
                        ostr << A.first << "[" << A.second << ":" << addrout << "]\n";
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
