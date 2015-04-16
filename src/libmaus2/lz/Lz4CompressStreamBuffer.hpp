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
#if !defined(LIBMAUS_LZ_LZ4COMPRESSSTREAMBUFFER_HPP)
#define LIBMAUS_LZ_LZ4COMPRESSSTREAMBUFFER_HPP

#include <libmaus2/lz/Lz4CompressWrapper.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <ostream>
#include <streambuf>

namespace libmaus2
{
	namespace lz
	{
		struct Lz4CompressStreamBuffer : public Lz4CompressWrapper, public ::std::streambuf
		{
			::libmaus2::autoarray::AutoArray<char> buffer;
		
			Lz4CompressStreamBuffer(std::ostream & out, uint64_t const blocksize)
			: Lz4CompressWrapper(out, blocksize), buffer(blocksize,false)
			{
				setp(buffer.begin(),buffer.end()-1);
			}
			
			int_type overflow(int_type c = traits_type::eof())
			{
				if ( c != traits_type::eof() )
				{
					*pptr() = c;
					pbump(1);
					doSync();
				}

				return c;
			}
			
			void doSync()
			{
				int64_t const n = pptr()-pbase();
				pbump(-n);
				Lz4CompressWrapper::wrapped.write(pbase(),n);
			}
			int sync()
			{
				doSync();
				Lz4CompressWrapper::wrapped.flush();
				return 0; // no error, -1 for error
			}			
		};
	}
}
#endif
