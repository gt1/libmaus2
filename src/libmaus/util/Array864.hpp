/**
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
**/

#if ! defined(LIBMAUS_UTIL_ARRAY864)
#define LIBMAUS_UTIL_ARRAY864

#include <libmaus/rank/ERank222B.hpp>

namespace libmaus
{
	namespace util
	{
		struct Array864
		{
			typedef Array864 this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			typedef ::libmaus::rank::ERank222B rank_type;
			typedef rank_type::writer_type writer_type;
			typedef writer_type::data_type data_type;

			uint64_t n;
			::libmaus::autoarray::AutoArray<data_type> B;
			::libmaus::rank::ERank222B::unique_ptr_type R;
			::libmaus::autoarray::AutoArray<uint8_t> A8;
			::libmaus::autoarray::AutoArray<uint64_t> A64;
			
			void serialise(std::ostream & out) const
			{
				::libmaus::serialize::Serialize<uint64_t>::serialize(out,n);
				B.serialize(out);
				A8.serialize(out);
				A64.serialize(out);
			}
			
			static uint64_t deserializeNumber(std::istream & in)
			{
				uint64_t n;
				::libmaus::serialize::Serialize<uint64_t>::deserialize(in,&n);
				assert ( in );
				return n;
			}
			
			Array864(std::istream & in)
			: n(deserializeNumber(in)), B(in), R(new ::libmaus::rank::ERank222B(B.get(),B.size()*64)), A8(in), A64(in)
			{
			}
			
			template<typename iterator>
			Array864(iterator a, iterator e)
			{
				n = e-a;
				
				if ( n )
				{
					B = ::libmaus::autoarray::AutoArray<data_type>((n+63)/64);
					writer_type W(B.get());
				
					for ( iterator i = a; i != e; ++i )
						W.writeBit( *i < 256 );
					
					W.flush();
				
					R = ::libmaus::rank::ERank222B::unique_ptr_type(new ::libmaus::rank::ERank222B(B.get(), B.size()*64));
					
					uint64_t const n8 = R->rank1(n-1);
					uint64_t const n64 = R->rank0(n-1);
					
					A8 = ::libmaus::autoarray::AutoArray<uint8_t>(n8,false);
					A64 = ::libmaus::autoarray::AutoArray<uint64_t>(n64,false);

					uint64_t j = 0;
					for ( iterator i = a; i != e; ++i,++j )
						if ( *i < 256 )
							A8[ R->rank1(j)-1 ] = *i;
						else
							A64[ R->rank0(j)-1 ] = *i;
					
					#if 0		
					j = 0;
					for ( iterator i = a; i != e; ++i, ++j )
						assert ( (*this)[j] == *i );
					#endif
				
					#if defined(ARRAY864DEBUG)
					#if defined(_OPENMP)
					#pragma omp parallel for
					#endif	
					for ( int64_t i = 0; i < static_cast<int64_t>(n); ++i )
						assert ( (*this)[i] == a[i] );
					#endif
				}
				
			}
			
			uint64_t operator[](uint64_t const i) const
			{
				if ( i >= n )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Access of element " << i << " >= " << n << " in Array864::operator[]";
					se.finish();
					throw se;
				}
			
				if ( ::libmaus::bitio::getBit(B.get(),i) )
					return A8[R->rank1(i)-1];
				else
					return A64[R->rank0(i)-1];
			}
		};
	}
}
#endif
