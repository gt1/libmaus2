/*
    libmaus2
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
#if ! defined(LIBMAUS2_LF_LFBASE_HPP)
#define LIBMAUS2_LF_LFBASE_HPP

#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/bitio/CompactArray.hpp>
#include <libmaus2/huffman/HuffmanTreeNode.hpp>
#include <libmaus2/wavelet/toWaveletTreeBits.hpp>

namespace libmaus2
{
	namespace lf
	{
		template<typename _wt_type>
		struct LFBase
		{
			typedef _wt_type wt_type;
			typedef typename wt_type::unique_ptr_type wt_ptr_type;
			wt_ptr_type W;
			::libmaus2::autoarray::AutoArray<uint64_t> D;

			uint64_t serialize(std::ostream & out)
			{
				uint64_t s = 0;
				s += W->serialize(out);
				return s;
			}
				
			static ::libmaus2::autoarray::AutoArray<uint64_t> computeD(wt_type const * W)
			{
				::libmaus2::autoarray::AutoArray<uint64_t> D( (1ull << W->getB()) + 1 , true );
				
				for ( uint64_t i = 0; i < (1ull << W->getB()); ++i )
					D[i] = W->rank(i, W->getN()-1);

				// output character frequencies
				#if 0
				for ( uint64_t i = 0; i < (1ull << W->getB()); ++i )
					if ( D[i] )
						std::cerr << i << " => " << D[i] << std::endl;
				#endif

				{ uint64_t c = 0; for ( uint64_t i = 0; i < ((1ull << W->getB())+1); ++i ) { uint64_t t = D[i]; D[i] = c; c += t; } }


				return D;
			}
			
			uint64_t sortedSymbol(uint64_t r) const
			{
				uint64_t const syms = (1ull<< W->getB());
				for ( unsigned int i = 0; i < syms; ++i )
					if ( D[syms-i-1] <= r )
						return syms-i-1;
				return 0;
			}
			
			uint64_t phi(uint64_t r) const
			{
				#if 0
				uint64_t const sym = W->sortedSymbol(r);
				#else		
				uint64_t const sym = sortedSymbol(r);
				#endif
				
				r -= D[sym];
				return W->select(sym,r);
			}
			
			uint64_t getN() const
			{
				return W->getN();
			}
			uint64_t getB() const
			{
				return W->getB();
			}
			
			uint64_t deserialize(std::istream & istr)
			{
				uint64_t s = 0;
				wt_ptr_type tW( new wt_type (istr,s) );
				W = UNIQUE_PTR_MOVE(tW);
				D = computeD(W.get());
				std::cerr << "LF: " << s << " bytes = " << s*8 << " bits" << " = " << (s+(1024*1024-1))/(1024*1024) << " mb " << std::endl;
				return s;
			}

			LFBase( std::istream & istr )
			{
				deserialize(istr);
			}

			LFBase( std::istream & istr, uint64_t & s )
			{
				s += deserialize(istr);
			}
			
			LFBase( wt_ptr_type & rW )
			: W(UNIQUE_PTR_MOVE(rW)), D(computeD(W.get()))
			{
				// std::cerr << "moved " << W.get() << std::endl;
			}
			
			LFBase( bitio::CompactArray::unique_ptr_type & ABWT )
			{
				// compute wavelet tree bits
				::libmaus2::autoarray::AutoArray<uint64_t> AW = ::libmaus2::wavelet::toWaveletTreeBitsParallel ( ABWT.get() );
				wt_ptr_type tW( new wt_type( AW, ABWT->n, ABWT->getB()) );
				W = UNIQUE_PTR_MOVE(tW);
				
				D = computeD(W.get());
				
				ABWT.reset(0);
			}
			LFBase( bitio::CompactArray::unique_ptr_type & ABWT, ::libmaus2::util::shared_ptr < huffman::HuffmanTreeNode >::type /* ahnode */ )
			{
				// compute wavelet tree bits
				::libmaus2::autoarray::AutoArray<uint64_t> AW = ::libmaus2::wavelet::toWaveletTreeBitsParallel ( ABWT.get() );
				wt_ptr_type tW( new wt_type ( AW, ABWT->n, ABWT->getB()) );
				W = UNIQUE_PTR_MOVE(tW);
				
				D = computeD(W.get());
				
				ABWT.reset(0);
			}
			
			uint64_t operator()(uint64_t const r) const
			{
				std::pair< typename wt_type::symbol_type,uint64_t> const is = W->inverseSelect(r);
				return D[is.first] + is.second;
			}

			uint64_t operator[](uint64_t pos) const
			{
				return (*W)[pos];
			}

			private:
			uint64_t rankm1(uint64_t const k, uint64_t const sp) const { return sp ? W->rank(k,sp-1) : 0; }
			
			public:
			uint64_t step(uint64_t const k, uint64_t const sp) const { return D[k] + rankm1(k,sp); }

			template<typename iterator>	
			inline void search(iterator query, uint64_t const m, uint64_t & sp, uint64_t & ep) const
			{
				sp = 0, ep = W->getN();
				
				for ( uint64_t i = 0; i < m && sp != ep; ++i )
					sp = step(query[m-i-1],sp),
					ep = step(query[m-i-1],ep);
			}

			uint64_t zeroPosRank() const
			{
				return W->getN() ? W->select ( W->rmq(0,W->getN()), 0) : 0;

				#if 0
				// find rank of position 0 (i.e. search terminating symbol)
				uint64_t r = 0;
				uint64_t const n = W->getN();
				
				for ( uint64_t i = 0; i < n; ++i )
				{
					if ( (i & ( 32*1024*1024-1)) == 0 )
						std::cerr << "(" << (static_cast<double>(i)/n) << ")";
				
					if ( ! (*W)[i] )
					{
						r = i;
						break;
					}
				}
				return r;
				#endif
			}
		};
	}
}
#endif
