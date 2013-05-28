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

#if ! defined(LIMITEDGENERICINPUT_HPP)
#define LIMITEDGENERICINPUT_HPP

#include <libmaus/aio/GenericInput.hpp>

namespace libmaus
{
	namespace aio
	{
		template < typename input_type >
		struct LimitedGenericInput : public GenericInput<input_type>
		{
			uint64_t const limit;
		
			LimitedGenericInput(
				std::string const & filename, 
				uint64_t const rbufsize, 
				uint64_t const rlimit,
				uint64_t const roffset = 0
			)
			: GenericInput<input_type>(filename,rbufsize,roffset), limit(rlimit)
			{
			
			}

			bool getNext(input_type & word)
			{
				if ( GenericInput<input_type>::totalwordsread == limit )
					return false;
				else
					return GenericInput<input_type>::getNext(word);
			}
		};
        }
}
#endif
