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

#if ! defined(WRITECOMPACTARRAY_HPP)
#define WRITECOMPACTARRAY_HPP

#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/util/NumberSerialisation.hpp>

namespace libmaus
{
	namespace aio
	{
		struct WriteCompactArray
		{
			template<typename N>
			static void writeArray(::libmaus::autoarray::AutoArray<N,::libmaus::autoarray::alloc_type_c> const & T, std::string const & filename, uint64_t const n)
			{
				std::ofstream ostr(filename.c_str(),std::ios::binary);
				if ( ! ostr.is_open() )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Cannot open file " << filename << " for writing." << std::endl;
					se.finish();
					throw se;
				}
				::libmaus::util::NumberSerialisation::serialiseNumber(ostr,n);
				ostr.write ( reinterpret_cast<char const *>(T.begin()), T.size()*sizeof(N) );
				ostr.flush();
				ostr.close();
				if ( ! ostr )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Cannot write file " << filename << "." << std::endl;
					se.finish();
					throw se;
				}
			}

			static void compactArray(::libmaus::autoarray::AutoArray<uint8_t,::libmaus::autoarray::alloc_type_c> & T)
			{
				uint8_t * outptr = T.begin();
				for ( uint64_t i = 0; i < T.size()/2; ++i )
					*(outptr++) = T[2*i] | (T[2*i+1] << 4);
				if ( T.size() & 1 )
					*(outptr++) = T[T.size()-1];
				T.resize( (T.size() + 1)/2 );
			}

			static void writeCompactArray(::libmaus::autoarray::AutoArray<uint8_t,::libmaus::autoarray::alloc_type_c> & T, std::string const & filename)
			{
				uint64_t const n = T.size();
				compactArray(T);
				writeArray(T,filename,n);
			}
		};
	}
}
#endif
