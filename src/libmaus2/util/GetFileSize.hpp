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

#if ! defined(GETFILESIZE_HPP)
#define GETFILESIZE_HPP

#include <fstream>
#include <string>
#include <vector>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/aio/InputStreamInstance.hpp>
#include <libmaus2/aio/OutputStreamInstance.hpp>

namespace libmaus2
{
	namespace util
	{
		/**
		 * file operations class
		 **/
		struct GetFileSize
		{
			/**
			 * read file filename containing an array of type data_type to an array
			 *
			 * @param filename name of input file
			 * @return array containing the contents of the input file as elements of type data_type
			 **/
			template<typename data_type>
			static ::libmaus2::autoarray::AutoArray<data_type> readFile(std::string const & filename)
			{
				// number of bytes
				uint64_t const n8 = ::libmaus2::util::GetFileSize::getFileSize(filename);
				if ( n8 % sizeof(data_type) )
				{
					libmaus2::exception::LibMausException se;
					se.getStream() << "GetFileSize::readFile(): size of file " << n8 << " is not a multiple of the data type size " << sizeof(data_type) << std::endl;
					se.finish();
					throw se;
				}
				// number of entities
				uint64_t const n = n8/sizeof(data_type);

				// allocate array
				::libmaus2::autoarray::AutoArray<data_type> A(n,false);

				// open file
				libmaus2::aio::InputStreamInstance istr(filename);
				istr.read ( reinterpret_cast<char *>(A.get()), n8 );

				return A;
			}

			/**
			 * copy n elements each multiplier bytes wide from in to out
			 *
			 * @param in input stream
			 * @param out output stream
			 * @param n number of entities
			 * @param multiplier size of each entity in bytes
			 **/
			template<typename in_type, typename out_type>
			static void copy(
				in_type & in,
				out_type & out,
				uint64_t n,
				uint64_t const multiplier = 1)
			{
				n *= multiplier;
				::libmaus2::autoarray::AutoArray < char > buf(16*1024,false);

				while ( n )
				{
					uint64_t const tocopy = std::min(n,buf.getN());

					in.read(buf.get(), tocopy);
					if ( in.gcount() != static_cast<int64_t>(tocopy) )
					{
						int error = errno;
						::libmaus2::exception::LibMausException se;
						se.getStream() << "Failed to read " << tocopy << " bytes in ::libmaus2::util::GetFileSize::copy(): " << strerror(error) << std::endl;
						se.finish();
						throw se;
					}

					out.write ( buf.get(), tocopy );
					if ( ! out )
					{
						int error = errno;
						::libmaus2::exception::LibMausException se;
						se.getStream() << "Failed to write " << tocopy << " bytes in ::libmaus2::util::GetFileSize::copy(): " << strerror(error) << std::endl;
						se.finish();
						throw se;
					}


					n -= tocopy;
				}
			}

			/**
			 * copy n elements each multiplier bytes wide from in to out; while
			 * copying map the bytes from input to output via cmap
			 *
			 * @param in input stream
			 * @param out output stream
			 * @param cmap character mapping table
			 * @param n number of entities
			 * @param multiplier size of each entity in bytes
			 **/
			template<typename in_type, typename out_type, typename map_type>
			static void copyMap(
				in_type & in,
				out_type & out,
				map_type const & cmap,
				uint64_t n,
				uint64_t const multiplier = 1
			)
			{
				n *= multiplier;
				::libmaus2::autoarray::AutoArray < char > buf(16*1024,false);

				while ( n )
				{
					uint64_t const tocopy = std::min(n,buf.getN());

					in.read(buf.get(), tocopy);
					if ( in.gcount() != static_cast<int64_t>(tocopy) )
					{
						int error = errno;
						::libmaus2::exception::LibMausException se;
						se.getStream() << "Failed to read " << tocopy << " bytes in ::libmaus2::util::GetFileSize::copy(): " << strerror(error) << std::endl;
						se.finish();
						throw se;
					}

					for ( uint64_t i = 0; i < tocopy; ++i )
						buf[i] = cmap[static_cast<int>(static_cast<uint8_t>(buf[i]))];

					out.write ( buf.get(), tocopy );
					if ( ! out )
					{
						int error = errno;
						::libmaus2::exception::LibMausException se;
						se.getStream() << "Failed to write " << tocopy << " bytes in ::libmaus2::util::GetFileSize::copy(): " << strerror(error) << std::endl;
						se.finish();
						throw se;
					}


					n -= tocopy;
				}
			}

			/**
			 * copy n elements each multiplier bytes wide from in to out
			 *
			 * @param in input stream
			 * @param out output iterator
			 * @param n number of entities
			 * @param multiplier size of each entity in bytes
			 **/
			template<typename iterator>
			static void copyIterator(
				std::istream & in,
				iterator & out,
				uint64_t n,
				uint64_t const multiplier = 1)
			{
				n *= multiplier;
				::libmaus2::autoarray::AutoArray < char > buf(16*1024,false);

				while ( n )
				{
					uint64_t const tocopy = std::min(n,buf.getN());

					in.read(buf.get(), tocopy);
					assert ( in.gcount() == static_cast<int64_t>(tocopy) );

					std::copy(buf.get(),buf.get()+tocopy,out);
					//out.write ( buf.get(), tocopy );
					// assert ( out );

					n -= tocopy;
				}
			}

			/**
			 * copy file from to file to
			 *
			 * @param from input file
			 * @param to output file
			 **/
			static void copy(std::string const & from, std::string const & to);
			/**
			 * get symbol at position pos of file filename
			 *
			 * @param filename name of input file
			 * @param pos position in file
			 * @return symbol at position pos of file filename
			 **/
			static int getSymbolAtPosition(std::string const & filename, uint64_t const pos);
			/**
			 * read file and store it in a byte array
			 *
			 * @param filename input file name
			 * @return contents of file as byte array
			 **/
			static ::libmaus2::autoarray::AutoArray<uint8_t> readFile(std::string const & filename);
			/**
			 * @param filename input file name
			 * @return true iff file exists (can be opened for reading)
			 **/
			static bool fileExists(std::string const & filename);
			/**
			 * return size of stream istr; istr needs to support seek operation
			 *
			 * @param istr input stream
			 * @return size of stream
			 **/
			static uint64_t getFileSize(std::istream & istr);
			/**
			 * return size of stream istr; istr needs to support seek operation
			 *
			 * @param istr input stream
			 * @return size of stream
			 **/
			static uint64_t getFileSize(std::wistream & istr);
			/**
			 * return size of file filename
			 *
			 * @param filename input file name
			 * @return size of file filename
			 **/
			static uint64_t getFileSize(std::string const & filename);
			/**
			 * return sum of the sizes of the files enumerated in the vector filenames
			 *
			 * @param filenames vector of file names
			 * @return sum of the file sizes of the files in filenames
			 **/
			static uint64_t getFileSize(std::vector<std::string> const & filenames);
			/**
			 * return sum of the sizes of the files enumerated in the vector of vectors filenames
			 *
			 * @param filenames vector of vector of file names
			 * @return sum of the file sizes of the files in filenames
			 **/
			static uint64_t getFileSize(std::vector< std::vector<std::string> > const & filenames);
			/**
			 * check whether file A is older than file B
			 **/
			static bool isOlder(std::string const & fn_A, std::string const & fn_B);
		};
	}
}
#endif
