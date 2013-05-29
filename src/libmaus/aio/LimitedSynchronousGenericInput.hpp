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


#if ! defined(LIMITEDSYNCHRONOUSGENERICINPUT_HPP)
#define LIMITEDSYNCHRONOUSGENERICINPUT_HPP

#include <libmaus/aio/SynchronousGenericInput.hpp>

namespace libmaus
{
	namespace aio
	{
		/**
		 * synchronous buffered input class with element count limit
		 **/
		template < typename input_type >
		struct LimitedSynchronousGenericInput : public SynchronousGenericInput<input_type>
		{
		        typedef input_type value_type;
		        typedef LimitedSynchronousGenericInput<value_type> this_type;
		        typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
		
		        /**
		         * constructor
		         *
		         * @param filename name of input file
		         * @param rbufsize size of buffer in elements
		         * @param rlimit maximum number of extracted elements
		         * @param roffset reading start offset
		         **/
			LimitedSynchronousGenericInput(
				std::string const & filename, 
				uint64_t const rbufsize, 
				uint64_t const rlimit,
				uint64_t const roffset = 0
			)
			: SynchronousGenericInput<input_type>(filename,rbufsize,roffset,rlimit)
			{
			
			}
		};
        }
}
#endif
