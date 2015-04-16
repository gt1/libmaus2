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
#if ! defined(LIBMAUS2_LF_IMPCACHELINERANKNONZERO_HPP)
#define LIBMAUS2_LF_IMPCACHELINERANKNONZERO_HPP

#include <libmaus2/huffman/RLDecoder.hpp>
#include <libmaus2/rank/ImpCacheLineRank.hpp>

namespace libmaus2
{
	namespace lf
	{
		struct ImpCacheLineRankLFNonZero
		{
			typedef ::libmaus2::rank::ImpCacheLineRank rank_type;
			typedef rank_type::WriteContext writer_type;
			typedef rank_type::unique_ptr_type rank_ptr_type;			
			
			typedef ImpCacheLineRankLFNonZero this_type;
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

			ImpCacheLineRankLFNonZero (::libmaus2::huffman::RLDecoder & decoder, uint64_t const maxval, uint64_t * const zcnt = 0)
			: n(decoder.getN())
			{
				if ( n )
				{
					rank_dictionaries = ::libmaus2::autoarray::AutoArray < rank_ptr_type >(maxval);
					::libmaus2::autoarray::AutoArray<writer_type> writers(rank_dictionaries.size());
					
					for ( uint64_t i = 0; i < rank_dictionaries.size(); ++i )
					{
						rank_ptr_type trank_dictionariesi(new rank_type(n+1));
						rank_dictionaries[i] = UNIQUE_PTR_MOVE(trank_dictionariesi);
						writers[i] = rank_dictionaries[i]->getWriteContext();
					}
					
					uint64_t zshift = 0;
					for ( uint64_t i = 0; i < n; ++i )
					{
						uint64_t const sym = decoder.decode();
						
						for ( uint64_t j = 0; j < rank_dictionaries.size(); ++j )
							writers[j].writeBit(sym && ((sym-1) == j));
						
						if ( ! sym )
							zshift++;
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
					
					for ( uint64_t i = 0; i < D.size(); ++i )
						D[i] += zshift;
						
					if ( zcnt )
						*zcnt = zshift;
				}
			}

			uint64_t getN() const
			{
				return n;
			}
			
			public:
			inline uint64_t rankm1(uint64_t const k, uint64_t const sp) const
			{
				return rank_dictionaries[k-1]->rankm1(sp);
			}
			inline uint64_t step(uint64_t const k, uint64_t const sp) const 
			{
				assert ( k );
				return D[k-1] + rankm1(k,sp); 
			}
		};
	}
}
#endif
