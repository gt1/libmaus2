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

#if ! defined(FILESTREAMBASETYPE_HPP)
#define FILESTREAMBASETYPE_HPP

#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/util/shared_ptr.hpp>
#include <string>
#include <istream>
#include <fstream>

namespace libmaus2
{
	namespace huffman
	{
		struct FileStreamBaseType
		{
			typedef FileStreamBaseType this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
		
			typedef ::std::ifstream file_type;
			typedef ::libmaus2::util::unique_ptr<file_type>::type file_ptr_type;
			
			file_ptr_type fileptr;
			std::istream & in;
		
			FileStreamBaseType(std::string const & filename, uint64_t const offset)
			: fileptr ( new file_type ( filename.c_str(), std::ios::binary ) ), in(*fileptr)
			{
				if ( offset )
					in.seekg(offset, std::ios::beg);
			}

			FileStreamBaseType(std::istream & rin)
			: fileptr ( ), in(rin)
			{
			}
			
			void read(char * const buf, uint64_t const n)
			{
				in.read(buf,n);
			}
		};
	}
}
#endif
