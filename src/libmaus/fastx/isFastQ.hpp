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

#if ! defined(ISFASTQ_HPP)
#define ISFASTQ_HPP

#include <istream>
#include <fstream>
#include <string>
#include <stdexcept>
#include <vector>
#include <cassert>

#include <libmaus/exception/LibMausException.hpp>

namespace libmaus
{
	namespace fastx
	{
		struct IsFastQ
		{
			static bool isFastQ(std::istream & istr);
			static bool isFastQ(std::string const & filename);
			static bool isFastQ(std::vector<std::string> const & filenames);
			
			static int getFirstCharacter(std::istream & in)
			{
				int const c = in.peek();
				if ( c >= 0 )
					in.putback(c);
				return c;
			}

			static int getFirstCharacter(std::vector<std::string> const & filenames)
			{
				for ( uint64_t i = 0; i < filenames.size(); ++i )
				{
					std::string const & fn = filenames[i];
					std::ifstream istr(fn.c_str(),std::ios::binary);
					if ( ! istr.is_open() )
					{
						::libmaus::exception::LibMausException se;
						se.getStream() << "IsFastQ::getFirstCharacter(): Failed to open file " << fn << std::endl;
						se.finish();
						throw se;
					}
					
					int const c = getFirstCharacter(istr);
					
					istr.close();
					
					if ( c >= 0 )
						return c;
				}
				
				return -1;
			}

			static bool isNonCompact(std::vector<std::string> const & filenames)
			{
				char const c = getFirstCharacter(filenames);
				return (c == '@') || (c == '>');
			}
		};
	}
}

#endif
