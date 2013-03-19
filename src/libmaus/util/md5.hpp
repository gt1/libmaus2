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

#if ! defined(MD5_HPP)
#define MD5_HPP

#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/util/md5.h>

#include <iomanip>
#include <sstream>

struct MD5
{
        static bool md5(std::string const & input, std::string & output)
        {
                md5_state_s state;
                md5_init(&state);
                md5_append(&state, reinterpret_cast<md5_byte_t const *>(input.c_str()), input.size());
                md5_byte_t digest[16];
                md5_finish(&state,digest);
                
                std::ostringstream ostr;
                for ( uint64_t i = 0; i < sizeof(digest)/sizeof(digest[0]); ++i )
                        ostr << std::hex << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(digest[i]);
                output = ostr.str();

                return true;
        }
        static bool md5(std::vector<std::string> const & V, std::string & output)
        {
                std::ostringstream ostr;
                for ( uint64_t i = 0; i < V.size(); ++i )
                        ostr << V[i];
                return md5(ostr.str(),output);
        }
        static bool md5(std::vector<std::string> const & V, uint64_t const k, std::string & output)
        {
                std::ostringstream ostr;
                for ( uint64_t i = 0; i < V.size(); ++i )
                        ostr << V[i] << ":" << ::libmaus::util::GetFileSize::getFileSize(V[i]) << ":";
                ostr << k;
                return md5(ostr.str(),output);
        }
};
#endif
