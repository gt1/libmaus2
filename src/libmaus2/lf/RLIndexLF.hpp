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
#if ! defined(LIBMAUS2_LF_RLINDEXLF_HPP)
#define LIBMAUS2_LF_RLINDEXLF_HPP

#include <libmaus2/rl/RLIndex.hpp>

namespace libmaus2
{
	namespace lf
	{
		template<typename _rlindex_type>
		struct RLIndexLFTemplate
		{
			typedef _rlindex_type rlindex_type;
			typedef RLIndexLFTemplate<rlindex_type> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
			
			typedef rlindex_type wt_type;
			typedef typename wt_type::unique_ptr_type wt_ptr_type;
		
			wt_ptr_type const W;
			uint64_t const n;
			uint64_t const n0;
			::libmaus2::autoarray::AutoArray<uint64_t> D;
			
			uint64_t getN() const
			{
				return n;
			}
			
			::libmaus2::autoarray::AutoArray<int64_t> getSymbols() const
			{
				::libmaus2::autoarray::AutoArray<int64_t> symbols = W->symbolArray();
				std::sort(symbols.begin(),symbols.end());
				return symbols;
			}
			
			uint64_t getSymbolThres() const
			{
				::libmaus2::autoarray::AutoArray<int64_t> const syms = getSymbols();
				if ( syms.size() )
					return syms[syms.size()-1]+1;
				else
					return 0;
			}

			::libmaus2::autoarray::AutoArray<uint64_t> computeD() const
			{
				::libmaus2::autoarray::AutoArray<int64_t> symbols = W->symbolArray();
				int64_t maxsym = symbols.size() ? symbols[0] : -1;
				int64_t minsym = maxsym;
				for ( uint64_t i = 0; i < symbols.size(); ++i )
				{
					maxsym = std::max(maxsym,symbols[i]);
					minsym = std::min(minsym,symbols[i]);
				}
				#if 0
				std::cerr << "minsym: " << maxsym << std::endl;
				std::cerr << "maxsym: " << maxsym << std::endl;
				#endif
				
				assert ( minsym >= 0 );
				
				::libmaus2::autoarray::AutoArray<uint64_t> D(maxsym+1);
				for ( uint64_t i = 0; i < symbols.size(); ++i )
				{
					int64_t const sym = symbols[i];
					D [ sym ] = n ? W->rank(sym,n-1) : 0;
					#if 0
					std::cerr << "D[" << sym << "]=" << D[sym] << std::endl;
					#endif
				}
				D.prefixSums();	

				return D;		
			}
			
			static unique_ptr_type loadSequential(std::string const & filename)
			{
				std::ifstream istr(filename.c_str(),std::ios::binary);
				if ( ! istr.is_open() )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "RLIndexLFTemplate::load() failed to open file " << filename << std::endl;
					se.finish();
					throw se;
				}
				
				unique_ptr_type ptr ( new this_type ( istr ) );
				
				if ( ! istr )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "RLIndexLFTemplate::load() failed to read file " << filename << std::endl;
					se.finish();
					throw se;					
				}
				
				return UNIQUE_PTR_MOVE(ptr);
			}

			static unique_ptr_type load(std::string const & filename)
			{
				return UNIQUE_PTR_MOVE(unique_ptr_type(new this_type(filename)));
			}

			RLIndexLFTemplate(std::istream & in)
			: W(new wt_type(in)), n(W->n), n0(n ? W->rank(0,n-1) : 0), D(computeD())
			{
			}

			RLIndexLFTemplate(std::string const & filename)
			: W(wt_type::load(filename)), n(W->n), n0(n ? W->rank(0,n-1) : 0), D(computeD())
			{
			}
			
			uint64_t operator()(uint64_t const r) const
			{
				std::pair< int64_t,uint64_t> const is = W->inverseSelect(r);
				return D[is.first] + is.second;
			}

			uint64_t operator[](uint64_t pos) const
			{
				return (*W)[pos];
			}

			std::pair<int64_t,uint64_t> extendedLF(uint64_t const r) const
			{
				std::pair< int64_t,uint64_t> const is = W->inverseSelect(r);
				return std::pair<int64_t,uint64_t>(is.first, D[is.first] + is.second);
			}

			public:
			uint64_t step(uint64_t const k, uint64_t const sp) const { return D[k] + W->rankm(k,sp); }
			std::pair<uint64_t,uint64_t> step(uint64_t const k, uint64_t const sp, uint64_t const ep) const 
			{
				return W->rankm(k,sp,ep,D.get());
			}
			std::pair<uint64_t,uint64_t> step(uint64_t const k, std::pair<uint64_t,uint64_t> const & P) const 
			{
				return W->rankm(k,P.first,P.second,D.get());
			}

			template<typename iterator>	
			inline void search(iterator query, uint64_t const m, uint64_t & sp, uint64_t & ep) const
			{
				sp = 0, ep = n;
						
				for ( uint64_t i = 0; i < m && sp != ep; ++i )
				{
					int64_t const sym = query[m-i-1];
					std::pair<uint64_t,uint64_t> const P = step(sym,sp,ep);
					sp = P.first;
					ep = P.second;
				}
			}
			template<typename iterator>	
			inline void search(iterator query, uint64_t const m, std::pair<uint64_t,uint64_t> & P) const
			{
				P = std::pair<uint64_t,uint64_t>(0,n);
						
				for ( uint64_t i = 0; i < m && P.first != P.second; ++i )
				{
					int64_t const sym = query[m-i-1];
					P = step(sym,P);
				}
			}

			uint64_t sortedSymbol(uint64_t r) const
			{
				uint64_t const syms = D.size();
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

		typedef RLIndexLFTemplate< ::libmaus2::rl::RLIndex > RLIndexLF;
		typedef RLIndexLFTemplate< ::libmaus2::rl::RLSimpleIndex > RLSimpleIndexLF;
	}
}
#endif
