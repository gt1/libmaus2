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
#if ! defined(CHECKOVERLAPRESULTINPUT_HPP)
#define CHECKOVERLAPRESULTINPUT_HPP

#include <libmaus/lcs/CheckOverlapResult.hpp>

namespace libmaus
{
	namespace lcs
	{
		struct CheckOverlapResultInput
		{
			typedef CheckOverlapResultInput this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			std::ifstream ifstr;
			std::istream & istr;
		
			CheckOverlapResultInput(std::string const & filename)
			: ifstr(filename.c_str(),std::ios::binary), istr(ifstr)
			{
			
			}
			
			CheckOverlapResultInput(std::istream & ristr)
			: ifstr(), istr(ristr)
			{
			
			}
			
			CheckOverlapResult::shared_ptr_type get()
			{
				return CheckOverlapResult::load(istr);
			}
		};
	}
}
#endif
