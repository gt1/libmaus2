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
#if ! defined(LIBMAUS2_AIO_INPUTSTREAMINSTANCEATOFFSET_HPP)
#define LIBMAUS2_AIO_INPUTSTREAMINSTANCEATOFFSET_HPP

#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/util/shared_ptr.hpp>
#include <libmaus2/exception/LibMausException.hpp>
#include <fstream>
#include <string>
#include <libmaus2/aio/InputStreamInstance.hpp>

namespace libmaus2
{
	namespace aio
	{
		/**
		 * checked input stream opening file at a given offset. Otherwise
		 * behaves like InputStreamInstance
		 **/
		struct InputStreamInstanceAtOffset : InputStreamInstance
		{
			//! this type
			typedef InputStreamInstanceAtOffset this_type;
			//! unique pointer type
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			//! shared pointer type
			typedef ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
			
			public:
			/**
			 * constructor
			 * 
			 * @param rfilename file name
			 * @param mode file open mode (as for std::ifstream)
			 **/
			InputStreamInstanceAtOffset(std::string const & rfilename, uint64_t const offset = 0)
			: InputStreamInstance(rfilename)
			{
				seekg(offset);
			}
		};
		
		/**
		 * class wrapping a InputStreamInstanceAtOffset object
		 **/
		struct InputStreamInstanceAtOffsetWrapper
		{
			//! wrapped object
			InputStreamInstanceAtOffset object;
			
			public:
			/**
			 * constructor
			 * 
			 * @param rfilename file name
			 * @param mode file open mode (as for std::ifstream)
			 **/
			InputStreamInstanceAtOffsetWrapper(std::string const & rfilename, uint64_t const offset = 0)
			: object(rfilename,offset) {}
		};
	}
}
#endif
