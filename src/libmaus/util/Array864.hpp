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

#if ! defined(LIBMAUS_UTIL_ARRAY864)
#define LIBMAUS_UTIL_ARRAY864

#include <libmaus/rank/ERank222B.hpp>
#include <libmaus/bitio/getBit.hpp>
#include <libmaus/util/Histogram.hpp>
#include <libmaus/util/iterator.hpp>

namespace libmaus
{
	namespace util
	{
		struct Array864Generator;
		
		struct Array864
		{
			typedef Array864 this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			typedef ::libmaus::rank::ERank222B rank_type;
			typedef rank_type::writer_type writer_type;
			typedef writer_type::data_type data_type;

			typedef Array864Generator generator_type;
			
			typedef libmaus::util::ConstIterator<this_type,uint64_t> const_iterator;

			uint64_t n;
			::libmaus::autoarray::AutoArray<data_type> B;
			::libmaus::rank::ERank222B::unique_ptr_type R;
			::libmaus::autoarray::AutoArray<uint8_t> A8;
			::libmaus::autoarray::AutoArray<uint64_t> A64;
			
			uint64_t byteSize() const
			{
				return
					sizeof(uint64_t)+
					B.byteSize()+
					R->byteSize()+
					A8.byteSize()+
					A64.byteSize();
			}
			
			const_iterator begin() const
			{
				return const_iterator(this,0);
			}
			const_iterator end() const
			{
				return const_iterator(this,n);
			}
			
			void serialise(std::ostream & out) const;
			static uint64_t deserializeNumber(std::istream & in);
			Array864() : n(0) {}
			Array864(std::istream & in);
			
			static unique_ptr_type load(std::string const & fs)
			{
				libmaus::aio::CheckedInputStream CIS(fs);
				unique_ptr_type u(new this_type(CIS));
				return UNIQUE_PTR_MOVE(u);
			}

			static unique_ptr_type load(libmaus::aio::CheckedInputStream & CIS)
			{
				unique_ptr_type u(new this_type(CIS));
				return UNIQUE_PTR_MOVE(u);
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
			
			uint64_t get(uint64_t const i) const
			{
				return (*this)[i];
			}
		};
		
		struct Array864Generator
		{
			uint64_t const n8;
			uint64_t const n64;
			uint64_t const n;

			typedef Array864::writer_type writer_type;
			typedef Array864::data_type data_type;
			
			static uint64_t getSmallerEqual(libmaus::util::Histogram & hist, int64_t const thres)
			{
				std::map<int64_t,uint64_t> const H = hist.getByType<int64_t>();
				
				uint64_t c = 0;
				
				for ( std::map<int64_t,uint64_t>::const_iterator ita = H.begin(); ita != H.end(); ++ita )
					if ( ita->first <= thres )
						c += ita->second;
				
				return c;
			}
			
			Array864::unique_ptr_type P;
			writer_type::unique_ptr_type W;
			uint64_t p8;
			uint64_t p64;

			Array864Generator(libmaus::util::Histogram & hist) 
			: 
				n8(getSmallerEqual(hist,8)), n64(getSmallerEqual(hist,64)-n8), n(n8+n64),
				P(Array864::unique_ptr_type(new Array864)),
				p8(0), p64(0)
			{
				P->B = ::libmaus::autoarray::AutoArray<data_type>((n+63)/64);
				W = UNIQUE_PTR_MOVE(writer_type::unique_ptr_type(new writer_type(P->B.begin())));
				P->A8 = ::libmaus::autoarray::AutoArray<uint8_t>(n8);
				P->A64 = ::libmaus::autoarray::AutoArray<uint64_t>(n64);
				P->n = n;
			}
			
			void add(uint64_t const v)
			{
				if ( v < 256 )
				{
					W->writeBit(1);
					P->A8[p8++] = v;
				}
				else
				{
					W->writeBit(0);
					P->A64[p64++] = v;
				}
			}
			
			Array864::unique_ptr_type createFinal()
			{
				W->flush();
				P->R = UNIQUE_PTR_MOVE(::libmaus::rank::ERank222B::unique_ptr_type(new ::libmaus::rank::ERank222B(P->B.get(), P->B.size()*64)));
				return UNIQUE_PTR_MOVE(P);
			}
		};
	}
}
#endif
