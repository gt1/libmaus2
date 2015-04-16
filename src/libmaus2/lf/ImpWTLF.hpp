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
#if ! defined(LIBMAUS2_LF_IMPWTLF_HPP)
#define LIBMAUS2_LF_IMPWTLF_HPP

#include <libmaus2/huffman/RLDecoder.hpp>
#include <libmaus2/util/TempFileNameGenerator.hpp>
#include <libmaus2/wavelet/ImpWaveletTree.hpp>
#include <libmaus2/wavelet/ImpExternalWaveletGenerator.hpp>

namespace libmaus2
{
	namespace lf
	{
		struct ImpWTLF
		{
			typedef ::libmaus2::wavelet::ImpWaveletTree wt_type;
			typedef wt_type::unique_ptr_type wt_ptr_type;
			
			typedef ImpWTLF this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
			
			uint64_t n;
			wt_ptr_type W;
			::libmaus2::autoarray::AutoArray < uint64_t > D;
			
			void serialise(std::ostream & out)
			{
				::libmaus2::util::NumberSerialisation::serialiseNumber(out,n);
				W->serialise(out);
			}
			
			ImpWTLF(std::istream & in)
			: n(::libmaus2::util::NumberSerialisation::deserialiseNumber(in)),
			  W (new wt_type(in))
			{				
				if ( n )
				{
					D = ::libmaus2::autoarray::AutoArray < uint64_t >((1ull<<W->getB())+1);
					for ( uint64_t i = 0; i < (1ull<<W->getB()); ++i )
						D [ i ] = W->rank(i,n-1);
					D.prefixSums();
				}
			}

			template<typename iterator>
			ImpWTLF ( iterator BWT, uint64_t const rn, ::libmaus2::util::TempFileNameGenerator & rtmpgen, uint64_t const rmaxval = 0)
			: n(rn)
			{
				if ( n )
				{	
					uint64_t maxval = rmaxval;
					for ( uint64_t i = 0; i < n; ++i )
						maxval = std::max ( maxval, static_cast<uint64_t>(BWT[i]) );
					uint64_t const b = ::libmaus2::math::numbits(maxval);

					::libmaus2::wavelet::ImpExternalWaveletGenerator IEWG(b,rtmpgen);
					for ( uint64_t i = 0; i < n; ++i )
						IEWG.putSymbol(BWT[i]);
					std::string const tmpfilename = rtmpgen.getFileName();
					IEWG.createFinalStream(tmpfilename);
					
					std::ifstream istr(tmpfilename.c_str(),std::ios::binary);
					wt_ptr_type tW(new wt_type(istr));
					W = UNIQUE_PTR_MOVE(tW);
					istr.close();
					remove ( tmpfilename.c_str() );
					
					D = ::libmaus2::autoarray::AutoArray < uint64_t >((1ull<<W->getB())+1);
					for ( uint64_t i = 0; i < (1ull<<W->getB()); ++i )
						D [ i ] = W->rank(i,n-1);
					D.prefixSums();
				}
			}

			static unique_ptr_type constructFromRL(std::string const & filename, uint64_t const maxval, std::string const & tmpprefix )
			{
				::libmaus2::util::TempFileNameGenerator tmpgen(tmpprefix,2);
				::libmaus2::huffman::RLDecoder decoder(std::vector<std::string>(1,filename));
				return unique_ptr_type(new this_type(decoder,::libmaus2::math::numbits(maxval),tmpgen));
			}

			static unique_ptr_type constructFromRL(std::vector<std::string> const & filenames, uint64_t const maxval, std::string const & tmpprefix )
			{
				::libmaus2::util::TempFileNameGenerator tmpgen(tmpprefix,2);
				::libmaus2::huffman::RLDecoder decoder(filenames);
				return unique_ptr_type(new this_type(decoder,::libmaus2::math::numbits(maxval),tmpgen));
			}

			ImpWTLF (::libmaus2::huffman::RLDecoder & decoder, uint64_t const b, ::libmaus2::util::TempFileNameGenerator & rtmpgen)
			: n(decoder.getN())
			{
				if ( n )
				{	
					::libmaus2::wavelet::ImpExternalWaveletGenerator IEWG(b,rtmpgen);
					for ( uint64_t i = 0; i < n; ++i )
						IEWG.putSymbol(decoder.decode());
					std::string const tmpfilename = rtmpgen.getFileName();
					IEWG.createFinalStream(tmpfilename);
					
					std::ifstream istr(tmpfilename.c_str(),std::ios::binary);
					wt_ptr_type tW(new wt_type(istr));
					W = UNIQUE_PTR_MOVE(tW);
					istr.close();
					remove ( tmpfilename.c_str() );
					
					D = ::libmaus2::autoarray::AutoArray < uint64_t >((1ull<<W->getB())+1);
					for ( uint64_t i = 0; i < (1ull<<W->getB()); ++i )
						D [ i ] = W->rank(i,n-1);
					D.prefixSums();
				}
			}
			
			uint64_t getN() const
			{
				return n;
			}
			
			private:
			
			public:
			inline uint64_t rankm1(uint64_t const k, uint64_t const sp) const { return W->rankm1(k,sp); }
			inline uint64_t step(uint64_t const k, uint64_t const sp) const { return D[k] + rankm1(k,sp); }
			
			uint64_t operator[](uint64_t const i) const
			{
				return (*W)[i];
			}

			uint64_t operator()(uint64_t const r) const
			{
				std::pair<uint64_t,uint64_t> const P = W->inverseSelect(r);
				return D[P.first] + P.second;
			}

			template<typename iterator>	
			inline void search(iterator query, uint64_t const m, uint64_t & sp, uint64_t & ep) const
			{
				sp = 0, ep = n;
				
				for ( uint64_t i = 0; i < m && sp != ep; ++i )
					sp = step(query[m-i-1],sp),
					ep = step(query[m-i-1],ep);
			}

			uint64_t sortedSymbol(uint64_t r) const
			{
				uint64_t const syms = (1ull<<W->getB());
				for ( unsigned int i = 0; i < syms; ++i )
					if ( D[syms-i-1] <= r )
						return syms-i-1;
				return 0;
			}
			
			uint64_t phi(uint64_t r) const
			{
				uint64_t const sym = sortedSymbol(r);
				r -= D[sym];
				return W->select(sym,r);
			}
		};
	}
}
#endif
