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
#if ! defined(LIBMAUS_UTIL_COUNTPUTITERATOR)
#define LIBMAUS_UTIL_COUNTPUTITERATOR

#include <libmaus/types/types.hpp>

namespace libmaus
{
	namespace util
	{
		template<typename _output_type, typename _value_type>
		struct CountPutIterator
		{
			typedef _output_type output_type;
			typedef _value_type value_type;
			typedef CountPutIterator<output_type,value_type> this_type;
		
			output_type * out;
			uint64_t c;
		
			CountPutIterator(output_type * const rout)
			: out(rout), c(0) {}
			
			operator bool() const
			{
				return *out;
			}
			
			void put(value_type const v)
			{
				c += 1;
				out->put(v);
			}
			
			void put(std::string const & s)
			{
				uint8_t const * u = reinterpret_cast<uint8_t const *>(s.c_str());
				uint8_t const * const ue = u + s.size();

				while ( u != ue )
					put(*(u++));
			}
			
			this_type & operator=(value_type const v)
			{
				put(v);
				return *this;
			}
			
			this_type & operator*() { return *this; }
			this_type & operator++() { return *this; }
			this_type & operator++(int) { return *this; }		
		};	
	}
}
#endif
