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

#if ! defined(LIBMAUS_UTIL_TOUPPERTABLE_HPP)
#define LIBMAUS_UTIL_TOUPPERTABLE_HPP

#include <libmaus2/autoarray/AutoArray.hpp>
#include <cctype>

namespace libmaus2
{
	namespace util
	{
                struct ToUpperTable
                {
			typedef ToUpperTable this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			::libmaus2::autoarray::AutoArray<uint8_t> touppertable;

                        ToUpperTable() : touppertable(256)
                        {
                                for ( unsigned int i = 0; i < 256; ++i )
                                	if ( isalpha(i) )
                                		touppertable[i] = ::toupper(i);
					else
						touppertable[i] = i;
                        }
                        
                        char operator()(char const a) const
                        {
                        	return static_cast<char>(touppertable[static_cast<uint8_t>(a)]);
                        }

                        uint8_t operator()(uint8_t const a) const
                        {
                        	return touppertable[a];
                        }
                        
                        void toupper(std::string & s) const
                        {
                        	for ( std::string::size_type i = 0; i < s.size(); ++i )
                        		s[i] = static_cast<char>(touppertable[static_cast<uint8_t>(s[i])]);
                        }

                        std::string operator()(std::string const & s) const
                        {
                        	std::string t = s;
                        	toupper(t);
                        	return t;
                        }
                };
	}
}
#endif
