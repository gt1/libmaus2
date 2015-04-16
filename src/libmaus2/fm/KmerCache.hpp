/*
    libmaus
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#if ! defined(LIBMAUS_FM_KMERCACHE_HPP)
#define LIBMAUS_FM_KMERCACHE_HPP

#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/fm/BidirectionalIndexInterval.hpp>
#include <libmaus/fastx/acgtnMap.hpp>
#include <libmaus/math/lowbits.hpp>

namespace libmaus
{
	namespace fm
	{
		struct KmerCache
		{
			typedef KmerCache this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			unsigned int const k;
			libmaus::autoarray::AutoArray< libmaus::fm::BidirectionalIndexInterval > C;
			
			static unsigned int const bits_per_symbol = 2;
			
			static uint64_t replaceSymbol(uint64_t v, unsigned int const k, unsigned int const i, uint64_t const sym)
			{
				unsigned int const shift = (k-i-1) * bits_per_symbol;
				
				// remove previous symbol
				v &= ~(libmaus::math::lowbits(bits_per_symbol) << shift);
				// add new symbol
				v |= sym << shift;
				
				return v;
			}
			
			static std::string printWord(uint64_t const v, unsigned int const k)
			{
				std::ostringstream ostr;
				
				for ( unsigned int i = 0; i < k; ++i )
					ostr << libmaus::fastx::remapChar(
						(v >> ((k-i-1)*bits_per_symbol)) & (libmaus::math::lowbits(bits_per_symbol))
					);
					
				return ostr.str();
			}

			static std::string printVector(std::vector<char> const & V, unsigned int const k)
			{
				std::ostringstream ostr;
				
				for ( unsigned int i = 0; i < k; ++i )
					ostr << libmaus::fastx::remapChar(V[i]-1);
					
				return ostr.str();
			}
			
			private:
			KmerCache(unsigned int const rk)
			: k(rk), C( 1ull << (2*k) )
			{
			
			}
			
			public:
			template<typename index_type, unsigned int range_low = libmaus::fastx::mapChar('A')+1, unsigned int range_high = libmaus::fastx::mapChar('T')+1>
			static unique_ptr_type construct(index_type const & index, unsigned int const k)
			{
				unique_ptr_type Tptr(new this_type(k));
				this_type & me = *Tptr;
				
				std::vector<char> S(k+1,range_low);

				std::vector<libmaus::fm::BidirectionalIndexInterval> B(k+1);
				B[k] = index.epsilon();
				for ( uint64_t i = 0; i < k; ++i )
					B[k-i-1] = index.backwardExtend(B[k-i],S[k-i-1]);
					
				uint64_t m = 0;
				
				while ( true )
				{
					me.C[m] = B[0];
					
					#if 0
					std::cerr << printWord(m,k) << "\t" << me.C[m] << std::endl;
					
					assert ( me.C[m] == index.biSearchBackward(S.begin(),k) );
					#endif
			
					int i = 0;
					while ( S[i] == range_high )
						++i;
								
					if ( i == static_cast<int>(k) )
						break;
					else
					{
						B[i] = index.backwardExtend(B[i+1],++S[i]);
						m = replaceSymbol(m,k,i,S[i]-range_low);
						
						while ( --i >= 0 )
						{
							B[i] = index.backwardExtend(B[i+1],(S[i]=range_low));
							m = replaceSymbol(m,k,i,0 /* == S[i]-range_low */);
						}
					}
				}
				
				return UNIQUE_PTR_MOVE(Tptr);
			}
			
			template<typename index_type, typename iterator>
			libmaus::fm::BidirectionalIndexInterval lookup(index_type const & index, iterator query, uint64_t m) const
			{
				if ( m < k )
				{
					return index.biSearchBackward(query,m);
				}
				else
				{
					iterator cquery = query + m - k;
					
					uint64_t v = 0;
					for ( unsigned int i = 0; i < k; ++i )
					{
						v <<= bits_per_symbol;
						v |= (*(cquery++))-1;
					}

					libmaus::fm::BidirectionalIndexInterval bint = C[v];
								
					cquery = query + m - k;
					
					while ( bint.siz  && cquery != query )
						bint = index.backwardExtend(bint,*(--cquery));
					
					return bint;
				}
			}
		};
	}
}
#endif
