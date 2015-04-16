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

#if ! defined(GETHOSTNAME_HPP)
#define GETHOSTNAME_HPP

#include <libmaus2/autoarray/AutoArray.hpp>
#include <cerrno>

namespace libmaus2
{
        namespace network
        {
                struct GetHostName
                {
                        static std::string getHostName()
                        {
                                ::libmaus2::autoarray::AutoArray<char> hostnamebuf(1025);
                                if ( gethostname(hostnamebuf.get(),hostnamebuf.size()-1) )
                                {
                                        ::libmaus2::exception::LibMausException se;
                                        se.getStream() << "gethostname() failed: " << strerror(errno) << std::endl;
                                        se.finish();
                                        throw se;
                                }
                                
                                std::string const shostname(hostnamebuf.get());
                                
                                return shostname;
                        }
                };
        }
}
#endif
