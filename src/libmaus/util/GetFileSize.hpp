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

#if ! defined(GETFILESIZE_HPP)
#define GETFILESIZE_HPP

#include <fstream>
#include <string>
#include <vector>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/aio/CheckedInputStream.hpp>
#include <libmaus/aio/CheckedOutputStream.hpp>

namespace libmaus
{
	namespace util
	{
		struct GetFileSize
		{
			template<typename data_type>
			static ::libmaus::autoarray::AutoArray<data_type> readFile(std::string const & filename)
			{
				uint64_t const n8 = ::libmaus::util::GetFileSize::getFileSize(filename);
				assert ( n8 % sizeof(data_type) == 0 );
				uint64_t const n = n8/sizeof(data_type);
				
				::libmaus::autoarray::AutoArray<data_type> A(n,false);
				
				std::ifstream istr(filename.c_str(),std::ios::binary);
				assert ( istr );
				assert ( istr.is_open() );
				
				istr.read ( reinterpret_cast<char *>(A.get()), n8 );
				
				istr.close();
				
				return A;
			}

			template<typename in_type, typename out_type>
			static void copy(
				in_type & in, 
				out_type & out,
				uint64_t n,
				uint64_t const multiplier = 1)
			{
				n *= multiplier;
				::libmaus::autoarray::AutoArray < char > buf(16*1024,false);
				
				while ( n )
				{
					uint64_t const tocopy = std::min(n,buf.getN());
					
					in.read(buf.get(), tocopy);
					if ( in.gcount() != static_cast<int64_t>(tocopy) )
					{
						int error = errno;
						::libmaus::exception::LibMausException se;
						se.getStream() << "Failed to read " << tocopy << " bytes in ::libmaus::util::GetFileSize::copy(): " << strerror(error) << std::endl;
						se.finish();
						throw se;
					}
					
					out.write ( buf.get(), tocopy );
					if ( ! out )
					{
						int error = errno;
						::libmaus::exception::LibMausException se;
						se.getStream() << "Failed to write " << tocopy << " bytes in ::libmaus::util::GetFileSize::copy(): " << strerror(error) << std::endl;
						se.finish();
						throw se;
					}

					
					n -= tocopy;
				}
			}

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
				::libmaus::autoarray::AutoArray < char > buf(16*1024,false);
				
				while ( n )
				{
					uint64_t const tocopy = std::min(n,buf.getN());
					
					in.read(buf.get(), tocopy);
					if ( in.gcount() != static_cast<int64_t>(tocopy) )
					{
						int error = errno;
						::libmaus::exception::LibMausException se;
						se.getStream() << "Failed to read " << tocopy << " bytes in ::libmaus::util::GetFileSize::copy(): " << strerror(error) << std::endl;
						se.finish();
						throw se;
					}
					
					for ( uint64_t i = 0; i < tocopy; ++i )
						buf[i] = cmap[static_cast<int>(static_cast<uint8_t>(buf[i]))];
					
					out.write ( buf.get(), tocopy );
					if ( ! out )
					{
						int error = errno;
						::libmaus::exception::LibMausException se;
						se.getStream() << "Failed to write " << tocopy << " bytes in ::libmaus::util::GetFileSize::copy(): " << strerror(error) << std::endl;
						se.finish();
						throw se;
					}

					
					n -= tocopy;
				}
			}

			template<typename iterator>
			static void copyIterator(
				std::istream & in, 
				iterator & out,
				uint64_t n,
				uint64_t const multiplier = 1)
			{
				n *= multiplier;
				::libmaus::autoarray::AutoArray < char > buf(16*1024,false);
				
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
			
			template<typename out_type>
			static uint64_t concatenate(std::vector<std::string> const & infilenames, out_type & out, bool deleteFiles = false)
			{
				uint64_t c = 0;
				
				for ( uint64_t i = 0; i < infilenames.size(); ++i )
				{
					std::string const & fn = infilenames[i];
					uint64_t const n = getFileSize(fn);
					::libmaus::aio::CheckedInputStream CIS(fn);
					copy(CIS,out,n);
					c += n;
					
					if ( deleteFiles )
						remove ( fn.c_str() );
				}

				out.flush();
				
				return c;
			}

			static void copy(std::string const & from, std::string const & to);
			static int getSymbolAtPosition(std::string const & filename, uint64_t const pos);
			static ::libmaus::autoarray::AutoArray<uint8_t> readFile(std::string const & filename);
			static bool fileExists(std::string const & filename);
			static uint64_t getFileSize(std::istream & istr);
			static uint64_t getFileSize(std::wistream & istr);
			static uint64_t getFileSize(std::string const & filename);
			static uint64_t getFileSize(std::vector<std::string> const & filenames);
			static uint64_t getFileSize(std::vector< std::vector<std::string> > const & filenames);
		};
	}
}
#endif
