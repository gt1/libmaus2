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

#if ! defined(CODEBASE_HPP)
#define CODEBASE_HPP

#include <libmaus/rank/ChooseCache.hpp>
#include <libmaus/util/unique_ptr.hpp>

namespace libmaus
{
	namespace rank
	{
		/**
		 * base class for enumerative coding
		 **/
		struct CodeBase
		{
			typedef CodeBase this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			protected:
			/**
			 * cache for choose operations
			 **/
			static ChooseCache CC64;

			public:
			/**
			 * (slow) population count
			 * @param i number
			 * @return population count of i
			 **/
			static unsigned int popcnt(uint64_t i)
			{
				unsigned int j = 0;
		
				while ( i )
				{
					if ( i & 1 )
						++j;
					i >>= 1;
				}
		
				return j;
			}
		};
	}
}
#endif
