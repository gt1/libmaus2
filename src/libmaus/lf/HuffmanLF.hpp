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

#if ! defined(HUFFMANLF_HPP)
#define HUFFMANLF_HPP

#include <memory>
#include <libmaus/rank/ERank222B.hpp>
#include <libmaus/bitio/CompactArray.hpp>

#include <libmaus/huffman/huffman.hpp>
#include <libmaus/math/bitsPerNum.hpp>
#include <libmaus/wavelet/HuffmanWaveletTree.hpp>

namespace libmaus
{
	namespace lf
	{
		struct HuffmanLF
		{
			::libmaus::util::shared_ptr < ::libmaus::wavelet::HuffmanWaveletTree >::type W;
			int minsym;
			::libmaus::autoarray::AutoArray<uint64_t> D;

			uint64_t serialize(std::ostream & out)
			{
				uint64_t s = 0;
				s += W->serialize(out);
				return s;
			}
				
			static ::libmaus::autoarray::AutoArray<uint64_t> computeD(::libmaus::wavelet::HuffmanWaveletTree const * W)
			{
				uint64_t const symrange = W->enctable.getSymbolRange();
				
				::libmaus::autoarray::AutoArray<uint64_t> D( symrange + 1 );
				
				for ( uint64_t i = 0; i < symrange; ++i )
					if ( W->enctable.codeused [ i ] )
						D[i] = W->rank(static_cast<int>(i)+W->enctable.minsym, W->n-1);

				// output character frequencies
				{ uint64_t c = 0; for ( uint64_t i = 0; i < symrange + 1; ++i ) { uint64_t t = D[i]; D[i] = c; c += t; } }


				return D;
			}
			
			uint64_t sortedSymbol(uint64_t r) const
			{
				#if 0
				uint64_t const syms = W->enctable.getSymbolRange();
				uint64_t low = 0;
				for ( unsigned int i = 0; i < syms; ++i )
				{
					int const sym = i+minsym;

					if (  W->enctable.codeused[sym-minsym] )
					{
						uint64_t rank = getN() ? W->rank ( sym, getN() - 1 ) : 0;
						
						if ( low <= r && r < low+rank )
							return sym;
						
						low += rank;
					}
				}
				#else
				uint64_t const syms = W->enctable.getSymbolRange();
				for ( unsigned int i = 0; i < syms; ++i )
					if (  W->enctable.codeused[i] )
						if ( D[i] <= r && r < D[i+1] )
							return i+minsym;
				#endif
				
				throw std::runtime_error("sortedSymbol failed.");
			}
			
			uint64_t phi(uint64_t r) const
			{
				#if 0
				uint64_t const sym = W->sortedSymbol(r);
				#else		
				uint64_t const sym = sortedSymbol(r);
				#endif
				
				#if 0
				std::cerr 
					<< "Symbol " << sym 
					<< " sym-minsym " << (static_cast<int>(sym)-minsym) 
					<< " D " << D[static_cast<int>(sym)-minsym]
					<< std::endl;
				#endif
				
				r -= D[static_cast<int>(sym)-minsym];
				r = W->select(sym,r);
				
				assert ( r < getN() );
				
				return r;
			}
			
			uint64_t getN() const
			{
				return W->n;
			}
			uint64_t getB() const
			{
				return ::libmaus::math::bitsPerNum(W->enctable.getSymbolRange()-1);
			}
			
			uint64_t deserialize(std::istream & istr)
			{
				uint64_t s = 0;
				W = ::libmaus::util::shared_ptr < ::libmaus::wavelet::HuffmanWaveletTree >::type ( new ::libmaus::wavelet::HuffmanWaveletTree(istr,s) );
				minsym = W->enctable.minsym;
				D = computeD(W.get());
				std::cerr << "HLF: " << s << " bytes = " << s*8 << " bits" << " = " << (s+(1024*1024-1))/(1024*1024) << " mb " << std::endl;
				return s;
			}

			HuffmanLF ( std::istream & istr )
			{
				deserialize(istr);
			}

			HuffmanLF ( std::istream & istr, uint64_t & s )
			{
				s += deserialize(istr);
			}
			
			HuffmanLF ( ::libmaus::util::shared_ptr < ::libmaus::wavelet::HuffmanWaveletTree>::type & RHWT ) : W(RHWT)
			{
				minsym = W->enctable.minsym;
				D = computeD(W.get());		
			}

			HuffmanLF ( ::libmaus::util::shared_ptr < bitio::CompactArray >::type ABWT )
			{
				W = ::libmaus::util::shared_ptr < ::libmaus::wavelet::HuffmanWaveletTree >::type ( new ::libmaus::wavelet::HuffmanWaveletTree(
					bitio::CompactArray::const_iterator(ABWT.get()),
					bitio::CompactArray::const_iterator(ABWT.get())+ABWT->n
					) );
				minsym = W->enctable.minsym;
				// ABWT.reset(0);
			}
			HuffmanLF ( ::libmaus::util::shared_ptr < bitio::CompactArray >::type ABWT, ::libmaus::util::shared_ptr < huffman::HuffmanTreeNode >::type ahnode )
			{
				std::cerr << "Constructing Huffman shaped wavelet tree..." << std::endl;
				W = ::libmaus::util::shared_ptr < ::libmaus::wavelet::HuffmanWaveletTree >::type ( new ::libmaus::wavelet::HuffmanWaveletTree(
					bitio::CompactArray::const_iterator(ABWT.get()),
					bitio::CompactArray::const_iterator(ABWT.get())+ABWT->n,
					ahnode
					) );
				std::cerr << "Constructing Huffman shaped wavelet tree done." << std::endl;
				
				std::cerr << "Checking wavelet tree...";
#if defined(_OPENMP)
#pragma omp parallel for
#endif
				for ( int64_t i = 0; i < static_cast<int64_t>(ABWT->n); ++i )
					assert (
						static_cast<int>(ABWT->get(i))
						==
						(*W)[i]
					);	
				std::cerr << "done." << std::endl;
				
				std::cerr << "Computing D...";
				minsym = W->enctable.minsym;
				D = computeD(W.get());	
				std::cerr << "done." << std::endl;
				
				#if 0
				std::cerr << "Resetting ABWT...";	
				ABWT.reset(0);
				std::cerr << "done." << std::endl;
				#endif
			}
			
			uint64_t operator()(uint64_t const r) const
			{
				int const sym = (*W)[r];
				return D[sym-minsym] + (r ? W->rank(sym,r-1) : 0);
			}

			int operator[](uint64_t pos) const
			{
				return (*W)[pos];
			}

			uint64_t rank(uint64_t const k, uint64_t const sp) const { return sp ? W->rank(k,sp-1) : 0; }
			uint64_t step(uint64_t const k, uint64_t const sp) const { return D[k-minsym] + rank(k,sp); }

			template<typename iterator>	
			inline void search(iterator query, uint64_t const m, uint64_t & sp, uint64_t & ep) const
			{
				sp = 0, ep = W->n;
				
				for ( uint64_t i = 0; i < m && sp != ep; ++i )
				{
					int const sym = query[m-i-1];
					
					if ( W->enctable.checkSymbol(sym) )
					{
						sp = step(sym,sp),
						ep = step(sym,ep);
					}
					else
					{
						sp = ep = 0;
					}
				}
			}

			uint64_t zeroPosRank() const
			{
				if ( ! W->n )
					throw std::runtime_error("zeroPosRank called on empty sequence.");

				uint64_t const symrange = W->enctable.getSymbolRange();
				
				for ( uint64_t i = 0; i < symrange; ++i )
					if ( W->enctable.codeused [ i ] )
						if ( W->rank(static_cast<int>(i)+W->enctable.minsym, W->n-1) )
							return W->select(static_cast<int>(i)+W->enctable.minsym, 0);
				
				throw std::runtime_error("Rank of position zero not found.");
			}
		};
	}
}
#endif
