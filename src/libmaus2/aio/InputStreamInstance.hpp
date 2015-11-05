/*
    libmaus2
    Copyright (C) 2015 German Tischler

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
#if ! defined(LIBMAUS2_AIO_INPUTSTREAMINSTANCE_HPP)
#define LIBMAUS2_AIO_INPUTSTREAMINSTANCE_HPP

#include <libmaus2/aio/InputStreamPointerWrapper.hpp>
#include <istream>

namespace libmaus2
{
	namespace aio
	{
		struct InputStreamInstance : protected InputStreamPointerWrapper, public std::istream
		{
			typedef InputStreamInstance this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			InputStreamInstance(std::string const & fn);

			/**
			 * get a single symbol from file filename at position offset
			 *
			 * @param filename name of file
			 * @param offset file offset
			 * @return symbol/byte at position offset in file filename
			 **/
			static int getSymbolAtPosition(std::string const & filename, uint64_t const offset)
			{
				this_type CIS(filename);
				CIS.seekg(offset);
				return CIS.get();
			}

			/**
			 * return size of file filename
			 *
			 * @param filename name of file
			 * @return size of file filename in bytes
			 **/
			static uint64_t getFileSize(std::string const & filename)
			{
				this_type CIS(filename);
				CIS.seekg(0,std::ios::end);
				return CIS.tellg();
			}
		};
	}
}
#endif
