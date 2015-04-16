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

#include <libmaus2/util/OutputFileNameTools.hpp>

std::string libmaus2::util::OutputFileNameTools::lcp(std::string const & a, std::string const & b)
{
	uint64_t i = 0;
	
	while ( i < a.size() && i < b.size() && a[i] == b[i] )
		i++;
		
	return a.substr(0,i);
}

std::string libmaus2::util::OutputFileNameTools::lcp(std::vector<std::string> const & V)
{
	if ( V.size() )
	{
		std::string cand = V[0];
		for ( uint64_t i = 1; i < V.size(); ++i )
			cand = lcp(cand,V[i]);
		return cand;
	}
	else
	{
		return std::string();
	}
}

bool libmaus2::util::OutputFileNameTools::endsOn(std::string const & a, std::string const & b)
{
	return a.size() >= b.size() && a.substr(a.size()-b.size()) == b;
}

std::string libmaus2::util::OutputFileNameTools::clipOff(std::string const & a, std::string const & b)
{
	if ( endsOn(a,b) )
		return a.substr(0,a.size()-b.size());
	else
		return a;
}

std::string libmaus2::util::OutputFileNameTools::endClip(std::string a, char const * const * V)
{
	for ( ; *V ; ++V )
	{
		a = clipOff(a,*V);
	}
	
	return a;
}

std::string libmaus2::util::OutputFileNameTools::endClipLcp(std::vector<std::string> const & V, char const * const * const E)
{
	return endClip(lcp(V),E);
}
