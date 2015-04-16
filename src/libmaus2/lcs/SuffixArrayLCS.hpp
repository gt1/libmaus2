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

#if ! defined(LIBMAUS2_LCS_SUFFIXARRAYLCS_HPP)
#define LIBMAUS2_LCS_SUFFIXARRAYLCS_HPP

#include <libmaus2/types/types.hpp>
#include <libmaus2/sv/sv.hpp>
#include <libmaus2/suffixsort/divsufsort.hpp>
#include <libmaus2/suffixsort/SkewSuffixSort.hpp>
#include <libmaus2/util/unordered_set.hpp>

#include <string>
#include <sstream>

namespace libmaus2
{
	namespace lcs
	{
		struct SuffixArrayLCS
		{
			// (simulated) suffix tree node
			struct QNode
			{
				// interval of leafs covered
				int32_t left;
				int32_t right;
				// string depth (not edge depth)
				int32_t depth;
				mutable int symmask;
				// number of leafs covered so far
				mutable int32_t fill;

				QNode() {}
				QNode(int32_t const rleft, int32_t const rright, int32_t const rdepth, int const rsymmask, int32_t const rfill)
				: left(rleft), right(rright), depth(rdepth), symmask(rsymmask), fill(rfill) {}
				
				inline bool operator==(QNode const & o) const
				{
					return left == o.left && right == o.right;
				}
				inline bool operator!=(QNode const & o) const
				{
					return left != o.left || right != o.right;
				}
				
				bool isFull() const
				{
					return fill == (right-left+1);
				}
				
				std::string toString() const
				{
					std::ostringstream ostr;
					ostr << "QNode(" << left << "," << right << "," << depth << "," << symmask << "," << fill << ")";
					return ostr.str();
				}
			};

			struct HashQNode
			{
				public:
				size_t operator()(QNode const & q) const
				{
					return ::std::hash<int32_t>()(q.left) ^ ::std::hash<int32_t>()(q.right);
				}
			};

			// compute parent node using prev and next arrays
			static inline QNode parent(QNode const & I, int32_t const * LCP, int32_t const * prev, int32_t const * next, int32_t const n)
			{
				int32_t const k = ( (I.right+1 >= n) || (LCP[I.left] > LCP[I.right+1]) ) ? I.left : (I.right+1);
				return QNode(prev[k],next[k]-1,LCP[k],I.symmask,I.right-I.left+1);
			}
			
			struct LCSResult
			{
				uint32_t maxlcp;
				uint32_t maxpos_a;
				uint32_t maxpos_b;
				
				LCSResult() : maxlcp(0), maxpos_a(0), maxpos_b(0) {}
				LCSResult(
					uint32_t const rmaxlcp,
					uint32_t const rmaxpos_a,
					uint32_t const rmaxpos_b	
				) : maxlcp(rmaxlcp), maxpos_a(rmaxpos_a), maxpos_b(rmaxpos_b) {}
			};

			template<unsigned int alphabet_size>
			static LCSResult lcs(std::string const & a, std::string const & b)
			{
				/* concatenate a and b into string c */
				std::string c(a.size()+b.size()+2,' ');
				for ( uint64_t i = 0; i < a.size(); ++i )
					c[i] = a[i]+2;
				c[a.size()] = 0;
				for ( uint64_t i = 0; i < b.size(); ++i )
					c[a.size()+1+i] = b[i]+2;
				c[c.size()-1] = 1;
				
				// allocate suffix sorting
				::libmaus2::autoarray::AutoArray<int32_t> SA(c.size(),false);
				
				// perform suffix sorting
				typedef ::libmaus2::suffixsort::DivSufSort<32,uint8_t *,uint8_t const *,int32_t *,int32_t const *,alphabet_size+2> sort_type;
				sort_type::divsufsort(reinterpret_cast<uint8_t const *>(c.c_str()), SA.get(), c.size());

				// compute LCP array
				::libmaus2::autoarray::AutoArray<int32_t> LCP = ::libmaus2::suffixsort::SkewSuffixSort<uint8_t,int32_t>::lcpByPlcp(
					reinterpret_cast<uint8_t const *>(c.c_str()), c.size(), SA.get());

				// compute psv and nsv arrays for simulating parent operation on suffix tree
				::libmaus2::autoarray::AutoArray<int32_t> const prev = ::libmaus2::sv::PSV::psv(LCP.get(),LCP.size());
				::libmaus2::autoarray::AutoArray<int32_t> const next = ::libmaus2::sv::NSV::nsv(LCP.get(),LCP.size());
				
				#if defined(LCS_DEBUG)
				for ( uint64_t i = 0; i < c.size(); ++i )
				{
					std::cerr << i << "\t" << LCP[i] << "\t" << prev[i] << "\t" << next[i] << "\t";
					for ( std::string::const_iterator ita = c.begin()+SA[i]; ita != c.end(); ++ita )
						if ( isalnum(*ita) )
							std::cerr << *ita;
						else
							std::cerr << "<" << static_cast<int>(*ita) << ">" ;
					std::cerr << std::endl;
				}
				
				std::cerr << "---" << std::endl;
				#endif

				int32_t const n = c.size();
				// queue all suffix tree leafs
				std::deque < QNode > Q;
				for ( int32_t i = 0; i < n; ++i )
					Q.push_back ( QNode(i,i,0, (SA[i]< static_cast<int32_t>(a.size()+1)) ? 1:2, 1 ) );

				// construct hash for tree nodes we have seen so far
				typedef ::libmaus2::util::unordered_set < QNode , HashQNode >::type hash_type;
				typedef hash_type::iterator hash_iterator_type;
				typedef hash_type::const_iterator hash_const_iterator_type;
				hash_type H(n);
				
				// we simulate a bottom up traversal of the generalised suffix tree for a and b
				while ( Q.size() )
				{
					// get node and compute parent
					QNode const I = Q.front(); Q.pop_front();
					QNode P = parent(I,LCP.get(),prev.get(),next.get(),n);

					// have we seen this node before?
					hash_iterator_type it = H.find(P);

					// no, insert it
					if ( it == H.end() )
					{
						it = H.insert(P).first;
					}
					// yes, update symbol mask and extend visited interval
					else
					{
						it->symmask |= I.symmask;
						it->fill += (I.right-I.left+1);
					}
					
					// if this is not the root and the node is full (we have seen all its children), 
					// then put it in the queue
					if ( P.right-P.left + 1 < n && it->isFull() )
						Q.push_back(P);
				}
				
				// maximum lcp value
				int32_t maxlcp = 0;
				uint32_t maxpos_a = 0;
				uint32_t maxpos_b = 0;
				
				// consider all finished nodes
				for ( hash_const_iterator_type it = H.begin(); it != H.end(); ++it )
				{
					#if defined(LCS_DEBUG)
					std::cerr << *it << std::endl;
					#endif
					
					// we need to have nodes from both strings a and b under this
					// node (sym mask has bits for 1 and 2 set) and the lcp value must be 
					// larger than what we already have
					if ( 
						it->symmask == 3 && it->depth > maxlcp 
					)
					{
						maxlcp = it->depth;
						
						for ( int32_t q = it->left; q <= it->right; ++q )
						{
							if ( SA[q] < static_cast<int32_t>(a.size()) )
								maxpos_a = SA[q];
							else
								maxpos_b = SA[q] - (a.size()+1);
						}
					}
				}
				
				return LCSResult(maxlcp,maxpos_a,maxpos_b);
			}
		};
	}
}
#endif
