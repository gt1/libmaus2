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

#if ! defined(LIBMAUS_REGEX_POSIX_REGEX_HPP)
#define LIBMAUS_REGEX_POSIX_REGEX_HPP

#include <libmaus/LibMausConfig.hpp>

#if defined(LIBMAUS_HAVE_REGEX_H)

#include <sys/types.h>
#include <regex.h>
#include <cstring>
#include <string>
#include <libmaus/exception/LibMausException.hpp>

namespace libmaus
{
	namespace regex
	{
		struct PosixRegex
		{
			regex_t preg;
			
			PosixRegex(std::string const & pattern, int const cflags = REG_EXTENDED)
			{
				int res = regcomp(&preg,pattern.c_str(),cflags);
				if ( res )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Failed to compile regular expression " << pattern << std::endl;
					se.finish();
					throw se;		
				}
			}
			~PosixRegex()
			{
				regfree(&preg);
			}
			int64_t findFirstMatch(char const * c) const
			{
				regmatch_t pmatch[1];
				memset(&pmatch,0,sizeof(pmatch));
				if ( regexec(&preg,c, sizeof(pmatch)/sizeof(pmatch[0]), &pmatch[0], 0) == 0 )
				{
					return pmatch[0].rm_so;
				}
				else
				{
					return -1;
				}
			}
			std::vector<uint64_t> findAllMatches(char const * c) const
			{
				int64_t r = 0;
				char const * d = c;
				std::vector<uint64_t> V;
				
				while ( (r = findFirstMatch(d)) >= 0 )
				{
					V.push_back( (d-c)+r );
					d = d+r+1;
				}
				
				return V;
			}
			std::string replaceFirstMatch(char const * c, char const * repl) const
			{
				regmatch_t pmatch[1];
				memset(&pmatch,0,sizeof(pmatch));
				if ( regexec(&preg,c, sizeof(pmatch)/sizeof(pmatch[0]), &pmatch[0], 0) == 0 )
				{
					uint64_t const start = pmatch[0].rm_so;
					uint64_t const end = pmatch[0].rm_eo;
					uint64_t const plen = end-start;
					
					std::string R( strlen(c) - plen + strlen(repl), ' ' );

					std::copy(c,c+pmatch[0].rm_so,R.begin());
					std::copy(repl,repl+strlen(repl),R.begin()+pmatch[0].rm_so);
					std::copy(c+pmatch[0].rm_eo,c+strlen(c),R.begin()+pmatch[0].rm_so+strlen(repl));
					
					return R;
				}
				else
				{
					return std::string(c);
				}
			}
			
			std::string replaceAllMatches(char const * c, char const * repl) const
			{
				std::vector<char> ostr;
				
				while ( *c )
				{
					regmatch_t pmatch[1];
					memset(&pmatch,0,sizeof(pmatch));
					if ( regexec(&preg,c, sizeof(pmatch)/sizeof(pmatch[0]), &pmatch[0], 0) == 0 )
					{
						uint64_t const start = pmatch[0].rm_so;
						uint64_t const end = pmatch[0].rm_eo;
						// uint64_t const plen = end-start;
				
						char const * d = c + start;
						for ( ; c != d; ++c )
							ostr.push_back( *c );
						c += end-start;
						for ( char const * trepl = repl; *trepl; ++trepl )
							ostr.push_back( *trepl);
					}
					else
					{
						while ( *c )
							ostr.push_back( *(c++) );
					}
				}
				
				return std::string(ostr.begin(),ostr.end());
			}
		};
	}
}
#endif
#endif
