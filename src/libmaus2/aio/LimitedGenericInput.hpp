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

#if ! defined(LIMITEDGENERICINPUT_HPP)
#define LIMITEDGENERICINPUT_HPP

#include <libmaus2/aio/GenericInput.hpp>

namespace libmaus2
{
	namespace aio
	{
		/**
		 * asynchronous generic input with an element count limit
		 **/
		template < typename input_type >
		struct LimitedGenericInput : public GenericInput<input_type>
		{
			private:
			//! limiot
			uint64_t const limit;

			public:
			/**
			 * constructor
			 *
			 * @param filename name of input file
			 * @param rbufsize size of buffer in elements
			 * @param rlimit number of elements to be read
			 * @param roffset in file offset
			 **/
			LimitedGenericInput(
				std::string const & filename,
				uint64_t const rbufsize,
				uint64_t const rlimit,
				uint64_t const roffset = 0
			)
			: GenericInput<input_type>(filename,rbufsize,roffset), limit(rlimit)
			{

			}

			/**
			 * get next element
			 *
			 * @param word reference to be filled with next element
			 * @return true iff next element was available
			 **/
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
