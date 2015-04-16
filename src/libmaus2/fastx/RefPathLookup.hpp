/*
    libmaus
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2015 Genome Research Limited

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
#if ! defined(LIBMAUS_FASTX_REFPATHLOOKUP_HPP)
#define LIBMAUS_FASTX_REFPATHLOOKUP_HPP

#include <libmaus/fastx/RefPathTokenVectorSequence.hpp>
#include <libmaus/aio/InputStreamFactoryContainer.hpp>

namespace libmaus
{
	namespace fastx
	{
		struct RefPathLookup
		{
			static char const * getDataDir()
			{
				char const * datadir = getenv("REF_CACHE");
				if ( (!datadir) || (!*datadir) )
					datadir = NULL;
				return datadir;	
			}
			
			static char const * getRefPath()
			{
				char const * refpath = getenv("REF_PATH");
				if ( (!refpath) || (!*refpath) )
					refpath = NULL;
				return refpath;	
			}

			char const * datadir;
			char const * refpath;
			RefPathTokenVector refcacheexp;
			RefPathTokenVectorSequence refpathexp;
			
			RefPathLookup()
			: 
				datadir(getDataDir()), 
				refpath(getRefPath()), 
				refcacheexp(datadir ? std::string(datadir) : std::string()),
				refpathexp(refpath ? std::string(refpath) : std::string())
			{
			
			}

			std::vector<std::string> expand(std::string const & sdigest) const
			{
				std::vector<std::string> E = refpathexp.expand(sdigest);
				E.push_back(refcacheexp.expand(sdigest));
				return E;
			}

			bool sequenceCached(std::string const & sdigest) const
			{
				std::vector<std::string> const E = expand(sdigest);
				bool found = false;
			
				for ( size_t z = 0; (!found) && z < E.size(); ++z )
				{
					std::string e = E[z];

					if ( e.find("URL=") != std::string::npos && e.find("URL=") == 0 )
						e = e.substr(strlen("URL="));

					if ( libmaus::aio::InputStreamFactoryContainer::tryOpen(e) )
						found = true;
				}
				
				return found;
			}
		};
	}
}
#endif
