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
#if ! defined(LIBMAUS2_LF_IMPCACHELINERANKLF_HPP)
#define LIBMAUS2_LF_IMPCACHELINERANKLF_HPP

#include <libmaus2/huffman/RLDecoder.hpp>
#include <libmaus2/rank/ImpCacheLineRank.hpp>

namespace libmaus2
{
	namespace lf
	{
		struct ImpCacheLineRankLF
		{
			typedef ::libmaus2::rank::ImpCacheLineRank rank_type;
			typedef rank_type::WriteContext writer_type;
			typedef rank_type::unique_ptr_type rank_ptr_type;			
			
			typedef ImpCacheLineRankLF this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
			
			uint64_t n;
			::libmaus2::autoarray::AutoArray < rank_ptr_type > rank_dictionaries;
			::libmaus2::autoarray::AutoArray < uint64_t > D;

			static unique_ptr_type constructFromRL(std::string const & filename, uint64_t const maxval, uint64_t * const zcnt = 0)
			{
				::libmaus2::huffman::RLDecoder decoder(std::vector<std::string>(1,filename));
				return unique_ptr_type(new this_type(decoder,maxval,zcnt));
			}

			static unique_ptr_type constructFromRL(std::vector<std::string> const & filenames, uint64_t const maxval, uint64_t * const zcnt = 0)
			{
				::libmaus2::huffman::RLDecoder decoder(filenames);
				return unique_ptr_type(new this_type(decoder,maxval,zcnt));
			}

			ImpCacheLineRankLF (::libmaus2::huffman::RLDecoder & decoder, uint64_t const maxval, uint64_t * const zcnt = 0)
			: n(decoder.getN())
			{
				if ( n )
				{
					rank_dictionaries = ::libmaus2::autoarray::AutoArray < rank_ptr_type >(maxval+1);
					::libmaus2::autoarray::AutoArray<writer_type> writers(rank_dictionaries.size());
					
					for ( uint64_t i = 0; i < rank_dictionaries.size(); ++i )
					{
						rank_ptr_type trank_dictionariesi(new rank_type(n+1));
						rank_dictionaries[i] = UNIQUE_PTR_MOVE(trank_dictionariesi);
						writers[i] = rank_dictionaries[i]->getWriteContext();
					}
					
					for ( uint64_t i = 0; i < n; ++i )
					{
						uint64_t const sym = decoder.decode();
						
						for ( uint64_t j = 0; j < rank_dictionaries.size(); ++j )
							writers[j].writeBit(sym == j);
					}
					
					for ( uint64_t j = 0; j < rank_dictionaries.size(); ++j )
					{
						writers[j].writeBit(0);
						writers[j].flush();
					}
					
					D = ::libmaus2::autoarray::AutoArray < uint64_t >(rank_dictionaries.size()+1);
					for ( uint64_t i = 0; i < rank_dictionaries.size(); ++i )
						D [ i ] = rank_dictionaries[i]->rank1(n-1);
					D.prefixSums();
					
					if ( zcnt )
						*zcnt = D[0];
				}
			}

			template<typename iterator>
			ImpCacheLineRankLF ( iterator BWT, uint64_t const rn, uint64_t const rmaxval = 0, uint64_t * const zcnt = 0)
			: n(rn)
			{
				if ( n )
				{
					uint64_t maxval = rmaxval;
					for ( uint64_t i = 0; i < n; ++i )
						maxval = std::max ( maxval, static_cast<uint64_t>(BWT[i]) );
						
					rank_dictionaries = ::libmaus2::autoarray::AutoArray < rank_ptr_type >(maxval+1);
					
					for ( uint64_t i = 0; i < rank_dictionaries.size(); ++i )
					{
						rank_ptr_type trank_dictionariesi(new rank_type(n+1));
						rank_dictionaries[i] = UNIQUE_PTR_MOVE(trank_dictionariesi);
						writer_type writer = rank_dictionaries[i]->getWriteContext();
						
						for ( uint64_t j = 0; j < n; ++j )
							writer.writeBit(BWT[j] == i);
						// write additional bit to make rankm1 defined for n
						writer.writeBit(0);
						
						writer.flush();
					}
					
					D = ::libmaus2::autoarray::AutoArray < uint64_t >(rank_dictionaries.size()+1);
					for ( uint64_t i = 0; i < rank_dictionaries.size(); ++i )
						D [ i ] = rank_dictionaries[i]->rank1(n-1);
					D.prefixSums();
					
					if ( zcnt )
						*zcnt = 0;
				}
			}
			
			uint64_t getN() const
			{
				return n;
			}
			
			private:
			
			public:
			inline uint64_t rankm1(uint64_t const k, uint64_t const sp) const { return rank_dictionaries[k]->rankm1(sp); }
			inline uint64_t step(uint64_t const k, uint64_t const sp) const { return D[k] + rankm1(k,sp); }
			
			uint64_t operator[](uint64_t const i) const
			{
				for ( uint64_t j = 0; j < rank_dictionaries.size(); ++j )
					if ( (*(rank_dictionaries[j]))[i] )
						return j;
				return rank_dictionaries.size();
			}

			uint64_t operator()(uint64_t const r) const
			{
				return step((*this)[r],r);
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
				uint64_t const syms = rank_dictionaries.size();
				for ( unsigned int i = 0; i < syms; ++i )
					if ( D[syms-i-1] <= r )
						return syms-i-1;
				return 0;
			}
			
			uint64_t phi(uint64_t r) const
			{
				uint64_t const sym = sortedSymbol(r);
				r -= D[sym];
				return rank_dictionaries[sym]->select1(r);
			}
		};
	}
}
#endif
