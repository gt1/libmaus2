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

#if ! defined(LFZERO_HPP)
#define LFZERO_HPP

#include <memory>
#include <libmaus2/rank/ERank222B.hpp>
#include <libmaus2/bitio/CompactArray.hpp>

#include <libmaus2/wavelet/toWaveletTreeBits.hpp>
#include <libmaus2/wavelet/WaveletTree.hpp>
#include <libmaus2/wavelet/ImpHuffmanWaveletTree.hpp>
#include <libmaus2/math/bitsPerNum.hpp>
#include <libmaus2/rl/RLIndex.hpp>

namespace libmaus2
{
	namespace lf
	{
		typedef ::libmaus2::wavelet::WaveletTree< ::libmaus2::rank::ERank222B, uint64_t > lfz_wt_type;
		typedef ::libmaus2::wavelet::QuickWaveletTree< ::libmaus2::rank::ERank222B, uint64_t > lfz_quick_wt_type;
		typedef ::libmaus2::wavelet::ImpHuffmanWaveletTree lfz_imp_huf_wt_type;
		typedef ::libmaus2::rl::RLIndex lfz_rlindex_type;
		typedef ::libmaus2::rl::RLSimpleIndex lfz_rlsimpleindex_type;
		
		template<typename _wt_type, typename _z_array_type>
		struct LFZeroTemplate
		{
			typedef LFZeroTemplate<_wt_type,_z_array_type> this_type;
			typedef typename ::libmaus2::util::unique_ptr < this_type > :: type unique_ptr_type;
			typedef typename ::libmaus2::util::shared_ptr < this_type > :: type shared_ptr_type;
		
			typedef _wt_type wt_type;
			typedef typename wt_type::unique_ptr_type wt_ptr_type;
			
			typedef _z_array_type z_array_type;
			typedef typename z_array_type::unique_ptr_type z_array_ptr_type;

			wt_ptr_type PW;
			wt_type const * W;
			z_array_ptr_type PZ;
			z_array_type const * Z;
			uint64_t p0rank;
			::libmaus2::autoarray::AutoArray<uint64_t> D;
			uint64_t readlen;

			uint64_t serialize(std::ostream & out)
			{
				uint64_t s = 0;
				s += W->serialize(out);
				Z->serialize(out);
				::libmaus2::serialize::Serialize<uint64_t>::serialize(out,p0rank);
				return s;
			}
				
			static ::libmaus2::autoarray::AutoArray<uint64_t> computeD(wt_type const * W)
			{
				::libmaus2::autoarray::AutoArray<int64_t> syms = W->getSymbolArray();
				
				if ( syms.size() )
				{
					std::sort(syms.begin(),syms.end());
					int64_t const maxsym = syms[syms.size()-1];
					::libmaus2::autoarray::AutoArray<uint64_t> D(maxsym+1);
					for ( uint64_t i = 0; i < syms.size(); ++i )
						D [ syms[i] ] = W->rank(syms[i],W->getN()-1);
					D.prefixSums();
					return D;
				}
				else
				{
					return ::libmaus2::autoarray::AutoArray<uint64_t>();
				}				
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
				std::cerr << "LFZero loading wavelet tree...";
				PW = UNIQUE_PTR_MOVE(wt_ptr_type ( new wt_type (istr,s) ));
				W = PW.get();
				std::cerr << "done." << std::endl;
				
				std::cerr << "LFZero Loading Z array...";
				PZ = UNIQUE_PTR_MOVE(z_array_ptr_type(new z_array_type(istr,s)));
				Z = PZ.get();
				std::cerr << "done." << std::endl;
				
				std::cerr << "LFZero Loading p0rank...";
				s += ::libmaus2::serialize::Serialize<uint64_t>::deserialize(istr,&p0rank);
				std::cerr << "done." << std::endl;
				
				std::cerr << "LFZero computing D vector...";
				D = computeD(W);
				std::cerr << "done." << std::endl;
				
				std::cerr << "LFZero: " << s << " bytes = " << s*8 << " bits" << " = " << (s+(1024*1024-1))/(1024*1024) << " mb " << std::endl;
				
				readlen = W->getN() / Z->size();
				
				return s;
			}

			LFZeroTemplate ( std::istream & istr )
			{
				deserialize(istr);
			}

			LFZeroTemplate ( std::istream & istr, uint64_t & s )
			{
				s += deserialize(istr);
			}
			
			LFZeroTemplate ( 
				wt_ptr_type & rPW, 
				z_array_ptr_type & rPZ, 
				uint64_t const rp0rank
			) : PW(UNIQUE_PTR_MOVE(rPW)), W(PW.get()), PZ(UNIQUE_PTR_MOVE(rPZ)), Z(PZ.get()), p0rank(rp0rank), D(computeD(W)),
			    readlen ( W->getN() / Z->size() )
			{
			}

			LFZeroTemplate ( 
				wt_type const * rW, 
				z_array_type const * rZ, 
				uint64_t const rp0rank
			) : PW(), W(rW), PZ(), Z(rZ), p0rank(rp0rank), D(computeD(W)),
			    readlen ( W->getN() / Z->size() )
			{
			}

			std::pair<uint64_t,uint64_t> paccess(uint64_t const r) const
			{
				std::pair<uint64_t,uint64_t> P = W->inverseSelect(r);

				if ( P.first )
					return std::pair<uint64_t,uint64_t>(P.first,0);
				else
					return std::pair<uint64_t,uint64_t>(P.first,(*Z)[P.second]);
			}
						
			// lf mapping
			uint64_t operator()(uint64_t const r) const
			{
				std::pair<uint64_t,uint64_t> P = W->inverseSelect(r);
				
				if ( P.first )
					return D[P.first] + P.second;
				else
					return (*Z) [ P.second ];
			}
			
			int compare(uint64_t r0, uint64_t r1)
			{
				uint64_t const sym0 = (*W)[r0];
				uint64_t const sym1 = (*W)[r1];
				
				if ( sym0 == sym1 )
				{
					if ( sym0 == 0 )
					{
						uint64_t const symrank0 = W->rank(sym0,r0) - 1;
						uint64_t const symrank1 = W->rank(sym1,r1) - 1;
						uint64_t const z0 = (*Z)[symrank0];
						uint64_t const z1 = (*Z)[symrank1];
						
						if ( z0 < z1 )
							return -1;
						else if ( z0 > z1 )
							return 1;
						else
							return 0;
					}
					else
					{
						return 0;
					}				
				}
				else // sym0 != sym1
				{
					if ( sym0 < sym1 )
						return -1;
					else
						return 1;
				}
			}

			uint64_t operator[](uint64_t pos) const
			{
				return (*W)[pos];
			}

			uint64_t rank(uint64_t const k, uint64_t const sp) const { return sp ? W->rank(k,sp-1) : 0; }
			uint64_t step(uint64_t const k, uint64_t const sp) const { return D[k] + rank(k,sp); }

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
				return p0rank;
			}
			
			bool check(::libmaus2::bitio::CompactArray const * C) const
			{
				uint64_t const n = getN();
				uint64_t rr = p0rank;
				uint64_t pp = n-1;
				
				bool ok = true;
				std::cerr << "Checking LF mapping (reproducing text from BWT)...";
				for ( uint64_t i = 0; i < n; ++i )
				{
					uint64_t const sym = (*this)[rr];

					if ( sym != C->get(pp) )
					{
						ok = false;
						std::cerr << "Failure for rank " << rr << " position " << pp << std::endl;
					}
					
					rr = (*this)(rr);
					pp -= 1;

					if ( ( (n-i-1) & (1024*1024-1)) == 0 )
					{
						std::cerr << (n-i-1)/(1024*1024) << std::endl;
					}
				}
			
				return ok;
			}
			
			typedef std::pair<uint64_t,uint64_t> upair;
			
			upair rnvGeneric(uint64_t const left, uint64_t const right, upair P) const
			{
				if ( P.first == 0 )
				{
					uint64_t const zleft = left?W->rank(0,left-1):0;
					uint64_t const zright = right?W->rank(0,right-1):0;
					uint64_t const znext = Z->rnvGeneric(zleft,zright,P.second);
					
					typedef typename z_array_type::symbol_type z_symbol_type;
					
					if ( znext == std::numeric_limits<z_symbol_type>::max() )
					{
						return upair(W->rnvGeneric(left,right,1),0);
					}
					else
					{
						return upair(0,znext);
					}
				}
				else
				{
					return upair(W->rnvGeneric(left,right,P.first),0);
				}
			}
			
			static upair next(upair P)
			{
				if ( P.first )
					return upair(P.first+1,P.second);
				else
					return upair(P.first,P.second+1);
			}
			
			uint64_t rank(upair P, uint64_t i) const
			{
				if ( P.first )
					return W->rank(P.first,i);
				else
				{
					uint64_t const symrank = i ? W->rank(P.first,i-1) : 0;
					return Z->rank(P.second,symrank);
				}
			}
			uint64_t select(upair P, uint64_t i) const
			{
				if ( P.first )
				{
					return W->select(P.first,i);
				}
				else
				{
					uint64_t const symrank = Z->select(P.second,i);
					return W->select(P.first,symrank);
				}
			}
			uint64_t getSA(uint64_t const r) const
			{
				uint64_t rr = r;
				uint64_t o = 0;
      
				while ( (*(W))[rr] != 0 )
					rr = (*this)(rr), ++o;
      
				uint64_t const p = ( ( (*(Z))[W->rank(0,rr)-1] + 1 )*readlen + o ) % getN();
      
				return p;			
			}
		};

		typedef LFZeroTemplate < lfz_wt_type, ::libmaus2::autoarray::AutoArray<uint32_t> > LFZero;
		typedef LFZeroTemplate < lfz_wt_type, lfz_wt_type > LFZeroWT;
		typedef LFZeroTemplate < lfz_quick_wt_type, ::libmaus2::autoarray::AutoArray<uint32_t> > LFZeroQuick;
		typedef LFZeroTemplate < lfz_quick_wt_type, lfz_wt_type > LFZeroQuickWT;

		typedef LFZeroTemplate < lfz_imp_huf_wt_type, ::libmaus2::autoarray::AutoArray<uint32_t> > LFZeroImp;
		typedef LFZeroTemplate < lfz_imp_huf_wt_type, ::libmaus2::wavelet::WaveletTree< ::libmaus2::rank::ERank222B , uint64_t > > LFZeroImpWt;

		typedef LFZeroTemplate < lfz_rlindex_type, ::libmaus2::autoarray::AutoArray<uint32_t> > LFZeroRL;
		typedef LFZeroTemplate < lfz_rlindex_type, ::libmaus2::wavelet::WaveletTree< ::libmaus2::rank::ERank222B , uint64_t > > LFZeroRLWt;

		typedef LFZeroTemplate < lfz_rlsimpleindex_type, ::libmaus2::autoarray::AutoArray<uint32_t> > LFZeroRLSimple;
		typedef LFZeroTemplate < lfz_rlsimpleindex_type, ::libmaus2::wavelet::WaveletTree< ::libmaus2::rank::ERank222B , uint64_t > > LFZeroRLSimpleWt;
	}
}
#endif
