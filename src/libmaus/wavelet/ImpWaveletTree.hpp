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
#if ! defined(IMPWAVELETTREE_HPP)
#define IMPWAVELETTREE_HPP

#include <libmaus/util/NumberSerialisation.hpp>
#include <libmaus/util/unique_ptr.hpp>
#include <libmaus/rank/ImpCacheLineRank.hpp>

namespace libmaus
{
	namespace wavelet
	{
		struct ImpWaveletTree
		{
			typedef ImpWaveletTree this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
		
			uint64_t const n;
			uint64_t const b;

			typedef ::libmaus::rank::ImpCacheLineRank rank_type;
			typedef rank_type::unique_ptr_type rank_ptr_type;
			typedef ::libmaus::autoarray::AutoArray<rank_ptr_type> rank_array_type;
			
			::libmaus::autoarray::AutoArray<rank_array_type> dicts;
			rank_type const * root;

			::libmaus::autoarray::AutoArray< ::libmaus::autoarray::AutoArray<rank_type const * > > traces;
			
			uint64_t getN() const
			{
				return n;
			}

			uint64_t getB() const
			{
				return b;
			}
			
			uint64_t size() const
			{
				return n;
			}
			
			void serialise(std::ostream & out) const
			{
				::libmaus::util::NumberSerialisation::serialiseNumber(out,n);
				::libmaus::util::NumberSerialisation::serialiseNumber(out,b);
				for ( uint64_t i = 0; i < dicts.size(); ++i )
					for ( uint64_t j = 0; j < dicts[i].size(); ++j )
						dicts[i][j]->serialise(out);
				out.flush();
			}
			
			ImpWaveletTree(std::istream & in)
			:
				n(::libmaus::util::NumberSerialisation::deserialiseNumber(in)),
				b(::libmaus::util::NumberSerialisation::deserialiseNumber(in)),
				dicts(b),
				root(0),
				traces ( 1ull << b )
			{
				if ( b )
				{
					dicts[0] = rank_array_type( 1 );	
					rank_ptr_type tdicts00(new rank_type(in));
					dicts[0][0] = UNIQUE_PTR_MOVE(tdicts00);
					root = dicts[0][0].get();
					
					for ( uint64_t ib = 1; ib < b; ++ib )
					{
						dicts[ib] = rank_array_type( 1ull << ib );	
						for ( uint64_t i = 0; i < dicts[ib].size(); ++i )
						{
							rank_ptr_type tdictsibi(new rank_type(in));
							dicts[ib][i] = UNIQUE_PTR_MOVE(tdictsibi);
							
							if ( i & 1 )
							{
								dicts[ib-1][i/2]->right = dicts[ib][i].get();
							}
							else
							{
								dicts[ib-1][i/2]->left = dicts[ib][i].get();
							}
							
							dicts[ib][i]->parent = dicts[ib-1][i/2].get();
						}
					}

					for ( uint64_t i = 0; i < (1ull << b); ++i )
					{
						traces[i] = ::libmaus::autoarray::AutoArray<rank_type const *>(b);
						rank_type const * node = root;
	
						for ( uint64_t mask = 1ull << (b-1), j = 0; mask; mask >>= 1, ++j )
						{
							traces[i][j] = node;
							
							if ( i & mask )
								node = node->right;
							else
								node = node->left;
						}
					}
				}
			}
			
			std::pair<uint64_t,uint64_t> inverseSelect(uint64_t i) const
			{
				uint64_t sym = 0;
				rank_type const * node = root;
				
				for ( uint64_t ib = 0; ib < b; ++ib )
				{
					unsigned int bit;
					uint64_t const r1 = node->inverseSelect1(i,bit);
					sym <<= 1;
					
					if ( bit )
					{
						i = r1-1;
						sym |= 1;
						node = node->right;
					}
					else
					{
						i = i-r1;
						sym |= 0;
						node = node->left;
					}
				}
								
				return std::pair<uint64_t,uint64_t>(sym,i);
			}

			uint64_t operator[](uint64_t i) const
			{
				uint64_t sym = 0;
				rank_type const * node = root;
				
				for ( uint64_t ib = 0; ib < b; ++ib )
				{
					unsigned int bit;
					uint64_t const r1 = node->inverseSelect1(i,bit);
					sym <<= 1;
					
					if ( bit )
					{
						i = r1-1;
						sym |= 1;
						node = node->right;
					}
					else
					{
						i = i-r1;
						sym |= 0;
						node = node->left;
					}
				}
								
				return sym;
			}
			
			uint64_t rank(uint64_t const s, uint64_t i) const
			{
				rank_type const * node = root;
				
				for ( uint64_t mask = 1ull << (b-1); mask; mask >>= 1 )
				{
					if ( s & mask )
					{
						uint64_t const r1 = node->rank1(i);
						if ( ! r1 )
							return 0;
						i = r1-1;
						node = node->right;
					}
					else
					{
						uint64_t const r0 = node->rank0(i);
						if ( ! r0 )
							return 0;
						i = r0-1;
						node = node->left;
					}
				}
				
				return i+1;
			}
			
			uint64_t select(uint64_t const s, uint64_t i) const
			{
				rank_type const * node = dicts[b-1][s/2].get();
				
				for ( uint64_t mask = 1; node; mask <<= 1 )
				{
					if ( s & mask )
						i = node->select1(i);
					else
						i = node->select0(i);
					
					node = node->parent;
				}
				
				return i;
			}

			uint64_t rankm1(uint64_t const k, uint64_t const i) const
			{
				return i ? rank(k,i-1) : 0;
			}
		};
	}
}
#endif
