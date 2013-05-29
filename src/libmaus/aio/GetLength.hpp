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


#if ! defined(GETLENGTH_HPP)
#define GETLENGTH_HPP

#include <string>
#include <fstream>
#include <libmaus/util/NumberSerialisation.hpp>

namespace libmaus
{
	namespace aio
	{
		/**
		 * class for get length of file type functions. please use the GetFileSize
		 * class instead.
		 **/
		struct GetLength
		{
			/**
			 * get length of file filename
			 *
			 * @param filename name of file
			 * @return length of file filename in bytes
			 **/
			static uint64_t getLength(std::string const & filename)
			{
				std::ifstream istr(filename.c_str(),std::ios::binary);
				if ( ! istr.is_open() )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Failed to open file " << filename << std::endl;
					se.finish();
					throw se;
				}
				uint64_t const n = ::libmaus::util::NumberSerialisation::deserialiseNumber(istr);
				return n;
			}

			/**
			 * get the sum of the lengths of the files in the given list of file names
			 *
			 * @param a file name list begin iterator (inclusive)
			 * @param b file name list end iterator (exclusive)
			 * @return sum of the lengths of the files in the given list of file names [a,b)
			 **/
			template<typename iterator>
			static uint64_t getLength(iterator a, iterator b)
			{
				uint64_t n = 0;
				for ( ; a != b ; ++a )
					n += getLength(*a);
				return n;
			}
		};
	}
}
#endif
