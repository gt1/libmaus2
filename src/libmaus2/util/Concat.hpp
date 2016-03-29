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

#if ! defined(CONCAT_HPP)
#define CONCAT_HPP

#include <libmaus2/util/GetFileSize.hpp>

namespace libmaus2
{
	namespace util
	{
		/**
		 * class for file concatenation
		 **/
		struct Concat
		{
			/**
			 * copy in to out
			 *
			 * @param in input stream
			 * @param out output stream
			 * @return number of bytes written to out
			 **/
			static uint64_t concat(std::istream & in, std::ostream & out);
			/**
			 * append contents of file filename to output stream out
			 *
			 * @param filename name of input file
			 * @param out output stream
			 * @return number of bytes written to out
			 **/
			static uint64_t concat(std::string const & filename, std::ostream & out);
			/**
			 * append the files in the list files to the output stream out; if rem is true, then
			 * remove the files during the process
			 *
			 * @param files list of files to be appended
			 * @param out output stream
			 * @param rem if true remove files after concatenation
			 * @return number of bytes written to out
			 **/
			static uint64_t concat(std::vector < std::string > const & files, std::ostream & out, bool const rem = true);
			/**
			 * concatenate files given in the list files by filename in the file with name outputfilename
			 * in parallel
			 *
			 * @param files list of files to be concatenated
			 * @param outputfilename output file name
			 * @param rem if true remove files after concatenation
			 * @return number of bytes written to output file
			 **/
			static uint64_t concatParallel(
				std::vector < std::string > const & files,
				std::string const & outputfilename,
				bool const rem,
				uint64_t const numthreads
			);
			/**
			 * concatenate files given in files by filename in the file with name outputfilename
			 * in parallel
			 *
			 * @param files list of files to be concatenated
			 * @param outputfilename output file name
			 * @param rem if true remove files after concatenation
			 * @return number of bytes written to output file
			 **/
			static uint64_t concatParallel(
				std::vector < std::vector < std::string > > const & files,
				std::string const & outputfilename,
				bool const rem,
				uint64_t const numthreads
			);
			/**
			 * concatenate files given in the list files by filename in the file with name outputfilename
			 *
			 * @param files list of files to be concatenated
			 * @param outputfilename output file name
			 * @param rem if true remove files after concatenation
			 * @return number of bytes written to output file
			 **/
			static uint64_t concat(std::vector < std::string > const & files, std::string const & outputfile, bool const rem = true);
			/**
			 * concatenate files given in files by filename in the file with name outputfilename
			 *
			 * @param files list of files to be concatenated
			 * @param outputfilename output file name
			 * @param rem if true remove files after concatenation
			 * @return number of bytes written to output file
			 **/
			static uint64_t concat(
				std::vector < std::vector < std::string > > const & files,
				std::string const & outputfilename,
				bool const rem
			);
		};
	}
}
#endif
