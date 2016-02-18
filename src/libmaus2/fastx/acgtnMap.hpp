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


#if ! defined(ACGTNMAP_HPP)
#define ACGTNMAP_HPP

#include <string>
#include <algorithm>

namespace libmaus2
{
	namespace fastx
	{
                inline static char remapChar(char c)
                {
                        switch ( c )
                        {
                                case 0: return 'A'; break;
                                case 1: return 'C'; break;
                                case 2: return 'G'; break;
                                case 3: return 'T'; break;
                                default: return 'N'; break;
                        }
                }

                inline static char mapChar(char c)
                {
                        switch ( c )
                        {
                                case 'a': return 0; break;
                                case 'A': return 0; break;
                                case 'c': return 1; break;
                                case 'C': return 1; break;
                                case 'g': return 2; break;
                                case 'G': return 2; break;
                                case 't': return 3; break;
                                case 'T': return 3; break;
                                default: return 4; break;
                        }
                }

                inline std::string mapString(std::string a)
                {
                        for ( unsigned int i = 0; i < a.size(); ++i )
                                a[i] = mapChar(a[i]);
                        return a;
                }

                inline std::string remapString(std::string a)
                {
                        for ( unsigned int i = 0; i < a.size(); ++i )
                                a[i] = remapChar(a[i]);
                        return a;
                }

                inline char invertN(char a)
                {
                        switch ( a )
                        {
                                case 0: return 3;
                                case 1: return 2;
                                case 2: return 1;
                                case 3: return 0;
                                default: return 4;
                        }
                }
                inline char invertUnmapped(char a)
                {
                        switch ( a )
                        {
                                case 'A': case 'a': return 'T';
                                case 'C': case 'c': return 'G';
                                case 'G': case 'g': return 'C';
                                case 'T': case 't': return 'A';
                                default: return 'N';
                        }
                }
                inline std::string invertN(std::string a)
                {
                        for ( uint64_t i = 0; i < a.size(); ++i )
                                a[i] = invertN(a[i]);
                        return a;
                }
                inline std::string invertUnmapped(std::string a)
                {
                        for ( uint64_t i = 0; i < a.size(); ++i )
                                a[i] = invertUnmapped(a[i]);
                        return a;
                }
                inline std::string reverseComplement(std::string a)
                {
                        a = invertN(a);
                        std::reverse(a.begin(),a.end());
                        return a;
                }
                inline std::string reverseComplementUnmapped(std::string a)
                {
                        a = invertUnmapped(a);
                        std::reverse(a.begin(),a.end());
                        return a;
                }
                template<typename iterator>
                inline bool hasIndeterminate(iterator ita, iterator ite)
                {
                	for ( ; ita != ite; ++ita )
                		if ( *ita == mapChar('N') )
                			return true;

			return false;
                }
                inline bool hasIndeterminate(std::string const & s)
                {
                	return hasIndeterminate(s.begin(),s.end());
                }
	}
}
#endif
