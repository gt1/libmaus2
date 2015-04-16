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
#if ! defined(LIBMAUS_AIO_CHECKEDINPUTSTREAMATOFFSET_HPP)
#define LIBMAUS_AIO_CHECKEDINPUTSTREAMATOFFSET_HPP

#include <libmaus/util/unique_ptr.hpp>
#include <libmaus/util/shared_ptr.hpp>
#include <libmaus/exception/LibMausException.hpp>
#include <fstream>
#include <string>

namespace libmaus
{
	namespace aio
	{
		/**
		 * checked input stream opening file at a given offset. Otherwise
		 * behaves like CheckedInputStream
		 **/
		struct CheckedInputStreamAtOffset : CheckedInputStream
		{
			//! this type
			typedef CheckedInputStreamAtOffset this_type;
			//! unique pointer type
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			//! shared pointer type
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			public:
			/**
			 * constructor
			 * 
			 * @param rfilename file name
			 * @param mode file open mode (as for std::ifstream)
			 **/
			CheckedInputStreamAtOffset(std::string const & rfilename, uint64_t const offset = 0, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::binary )
			: CheckedInputStream(rfilename,mode)
			{
				seekg(offset);
			}
		};
		
		/**
		 * class wrapping a CheckedInputStreamAtOffset object
		 **/
		struct CheckedInputStreamAtOffsetWrapper
		{
			//! wrapped object
			CheckedInputStreamAtOffset object;
			
			public:
			/**
			 * constructor
			 * 
			 * @param rfilename file name
			 * @param mode file open mode (as for std::ifstream)
			 **/
			CheckedInputStreamAtOffsetWrapper(std::string const & rfilename, uint64_t const offset = 0, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::binary )
			: object(rfilename,offset,mode) {}
		};
	}
}
#endif
