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
#if ! defined(LIBMAUS_HUFFMAN_HUFFMANTREE_HPP)
#define LIBMAUS_HUFFMAN_HUFFMANTREE_HPP

#include <libmaus/types/types.hpp>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/util/NumberSerialisation.hpp>
#include <libmaus/bitio/BitVector.hpp>
#include <libmaus/math/numbits.hpp>

namespace libmaus
{
	namespace huffman
	{
		struct HuffmanTree
		{
			typedef HuffmanTree this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			private:
			struct HuffmanLeafNode
			{
				int64_t sym;
				uint64_t cnt;

				// HuffmanLeafNode & operator=(HuffmanLeafNode const & o) { sym = o.sym; cnt = o.cnt; return *this; }
				
				int64_t getSym() const { return sym; }
				uint64_t getCnt() const { return cnt; }
				void setSym(int64_t const rsym) { sym = rsym; }
				void setCnt(uint64_t const rcnt) { cnt = rcnt; }
				
				bool operator<(HuffmanLeafNode const & o) const
				{
					if ( cnt != o.cnt )
						return cnt < o.cnt;
					else
						return sym < o.sym;
				}
				
				bool operator==(HuffmanLeafNode const & o) const
				{
					return sym == o.sym && cnt == o.cnt;
				}
				bool operator!=(HuffmanLeafNode const & o) const
				{
					return !((*this) == o);
				}				
			};

			struct HuffmanInnerNode
			{
				uint32_t left;
				uint32_t right;
				uint64_t cnt;
				
				// HuffmanInnerNode & operator=(HuffmanInnerNode const & o) { left = o.left; right = o.right; cnt = o.cnt; return *this; }
				
				bool operator<(HuffmanInnerNode const & o) const
				{
					if ( cnt != o.cnt )
						return cnt < o.cnt;
					else if ( left != o.left )
						return left < o.left;
					else
						return right < o.right;
				}

				bool operator==(HuffmanInnerNode const & o) const
				{
					return left == o.left && right == o.right && cnt == o.cnt;
				}
				bool operator!=(HuffmanInnerNode const & o) const
				{
					return !((*this) == o);
				}				
			};

			typedef union { HuffmanLeafNode L; HuffmanInnerNode I; } HuffmanNodeUnion;

			struct HuffmanNode
			{
				HuffmanNodeUnion node;
				
				HuffmanNode() 
				{
					memset(&node,0,sizeof(node));
				}
				HuffmanNode(HuffmanNode const & o)
				{
					memcpy(&node,&(o.node),sizeof(node));
				}
				HuffmanNode & operator=(HuffmanNode const & o)
				{
					memcpy(&node,&(o.node),sizeof(node));
					return *this;
				}
				HuffmanNode & operator=(HuffmanLeafNode const & o)
				{
					node.L = o;
					return *this;
				}
				HuffmanNode & operator=(HuffmanInnerNode const & o)
				{
					node.I = o;
					return *this;
				}
			};

			struct HuffmanNodeLeafComparator
			{
				bool operator()(HuffmanNode const & A, HuffmanNode const & B) const
				{
					return A.node.L < B.node.L;
				}
			};

			struct LeafDepthComparator
			{
				HuffmanNode const * N;
				
				LeafDepthComparator(HuffmanNode const * rN) : N(rN) {}
				
				bool operator()(uint64_t const i, uint64_t const j) const
				{
					return N[i].node.L.cnt < N[j].node.L.cnt;
				}
				
				bool operator()(HuffmanNode const & A, HuffmanNode const & B) const
				{
					return A.node.L.cnt < B.node.L.cnt;
				}
			};

			// sort leafs by depth and adjust pointers accordingly
			void sortSymbols()
			{
				libmaus::autoarray::AutoArray<uint32_t> P(2*leafs(),false);
				for ( uint64_t i = 0; i < leafs(); ++i )
					P[i] = i;

				// sort indices by depth
				std::stable_sort(P.begin(),P.begin()+leafs(),LeafDepthComparator(N.begin()));
				
				// compute inverse
				uint32_t * const R = P.begin()+leafs();
				for ( uint64_t i = 0; i < leafs(); ++i )
					R[P[i]] = i;
				
				// sort leaf objects
				std::sort(N.begin(),N.begin()+leafs(),LeafDepthComparator(0));
				
				// map pointers
				for ( uint64_t i = 0; i < inner(); ++i )
				{
					if ( N[leafs()+i].node.I.left < leafs() )
						N[leafs()+i].node.I.left  = R[N[leafs()+i].node.I.left];
					if ( N[leafs()+i].node.I.right < leafs() )
						N[leafs()+i].node.I.right = R[N[leafs()+i].node.I.right];
				}
			}

			void computeSubTreeCounts()
			{
				for ( uint64_t i = leafs(); i < leafs() + inner(); ++i )
				{
					uint64_t const leftcnt = 
						isLeaf(leftChild(i)) 
						? 
						1 
						: 
							(
								1
								+
								((N[leftChild(i)].node.I.cnt >> 32)&0xFFFFFFFFULL)
								+
								((N[leftChild(i)].node.I.cnt >>  0)&0xFFFFFFFFULL)
							);
					uint64_t const rightcnt = 
						isLeaf(rightChild(i)) 
						? 
						1 
						: 
							(
								1
								+
								((N[rightChild(i)].node.I.cnt >> 32)&0xFFFFFFFFULL)
								+
								((N[rightChild(i)].node.I.cnt >>  0)&0xFFFFFFFFULL)
							);
					
					N[i].node.I.cnt = (leftcnt << 32) | (rightcnt << 0);
				}
			}

			void computeSubTreeCountsNoLeafs()
			{
				for ( uint64_t i = leafs(); i < leafs() + inner(); ++i )
				{
					uint64_t const leftcnt = 
						isLeaf(leftChild(i)) 
						? 
						0
						: 
							(
								1
								+
								((N[leftChild(i)].node.I.cnt >> 32)&0xFFFFFFFFULL)
								+
								((N[leftChild(i)].node.I.cnt >>  0)&0xFFFFFFFFULL)
							);
					uint64_t const rightcnt = 
						isLeaf(rightChild(i)) 
						? 
						0 
						: 
							(
								1
								+
								((N[rightChild(i)].node.I.cnt >> 32)&0xFFFFFFFFULL)
								+
								((N[rightChild(i)].node.I.cnt >>  0)&0xFFFFFFFFULL)
							);
					
					N[i].node.I.cnt = (leftcnt << 32) | (rightcnt << 0);
				}
			}
			
			void assignDfsIds()
			{
				computeSubTreeCountsNoLeafs();
			
				if ( inner() )
				{
					uint64_t const rightmask = 0xFFFFFFFF00000000ULL;
					
					// set dfs id for root
					N[root()].node.I.cnt &= rightmask;
				
					// traverse tree and set new ids
					for ( uint64_t i = 0; i < inner(); ++i )
					{
						uint64_t const node = N.size()-i-1;
						
						// number of nodes in left sub tree
						uint64_t const leftcnt = (N[node].node.I.cnt >> 32) & 0xFFFFFFFFULL;
						// own id
						uint64_t const ownid   = (N[node].node.I.cnt >>  0) & 0xFFFFFFFFULL;
						
						if ( ! isLeaf(leftChild(node)) )
						{
							N[leftChild(node)].node.I.cnt &= rightmask;
							N[leftChild(node)].node.I.cnt |= (ownid+1);
						}
						if ( ! isLeaf(rightChild(node)) )
						{
							N[rightChild(node)].node.I.cnt &= rightmask;
							N[rightChild(node)].node.I.cnt |= (ownid+leftcnt+1);						
						}
						
						// remove left subtree count
						N[node].node.I.cnt &= 0xFFFFFFFFULL;
					}
				}
			}
			
			struct InnerNodeCountComparator
			{
				bool operator()(HuffmanNode const & A, HuffmanNode const & B) const
				{
					return A.node.I.cnt < B.node.I.cnt;
				}
			};
			
			
			void checkDfsIds(uint64_t node, uint64_t & id)
			{
				bool const ok = ( id == N[node].node.I.cnt );
				
				if ( ! ok )
				{
					std::cerr << "expected " << id << " got " << N[node].node.I.cnt << std::endl;
				}
				
				assert ( ok );
				id++;
				
				if ( ! isLeaf(leftChild(node)) )
					checkDfsIds(leftChild(node),id);
				if ( ! isLeaf(rightChild(node)) )
					checkDfsIds(rightChild(node),id);
			}
			
			void checkDfsIds()
			{
				uint64_t id = 0;
				checkDfsIds(root(),id);
			}
			
			void testAssignDfsIds()
			{
				assignDfsIds();
				checkDfsIds();
			}
			
			uint64_t fillCntMap(uint64_t node, std::map<uint64_t,uint64_t> & M)
			{
				if ( isLeaf(node) )
					M[node] = 1;
				else
					M[node] = 1 + fillCntMap(leftChild(node),M) + fillCntMap(rightChild(node),M);

				return M[node];
			}
			
			void testComputeSubTreeCounts()
			{
				std::map<uint64_t,uint64_t> M;
				fillCntMap(root(),M);
				computeSubTreeCounts();
			
				std::cerr << "Checking " << inner() << " inner nodes." << std::endl;	
				for ( uint64_t i = leafs(); i < N.size(); ++i )
				{
					assert (
						((N[i].node.I.cnt >> 32) & 0xFFFFFFFFULL)
						+
						((N[i].node.I.cnt >>  0) & 0xFFFFFFFFULL)
						+
						1
						==
						M.find(i)->second
					);
				}
			}

			void reorderByDfs()
			{
				assignDfsIds();
				
				// reassign pointers
				for ( uint64_t i = leafs(); i < N.size(); ++i )
				{
					if ( N[i].node.I.left >= leafs() )
						N[i].node.I.left = N[ N[i].node.I.left ] . node.I.cnt + leafs();
					if ( N[i].node.I.right >= leafs() )
						N[i].node.I.right = N[ N[i].node.I.right ] . node.I.cnt + leafs();
				}
				
				std::sort(N.begin()+leafs(),N.end(),InnerNodeCountComparator());
				
				treeroot = leafs();
			}

			// huffman tree
			libmaus::autoarray::AutoArray<HuffmanNode> N;
			bool setcode;
			uint64_t treeroot;

			public:
			// size of alphabet
			uint64_t size() const
			{
				return (N.size()+1)>>1;
			}
			
			// number of leafs
			uint64_t leafs() const
			{
				return size();
			}
			
			// number of inner nodes
			uint64_t inner() const
			{
				return N.size() - leafs();
			}
			
			// empty constructor
			HuffmanTree() : N(), setcode(false), treeroot(0)
			{
				
			}
			
			HuffmanTree(std::istream & in) 
			: N(libmaus::util::NumberSerialisation::deserialiseNumber(in),false), setcode(false), treeroot(0)
			{
				for ( uint64_t i = 0; i < leafs(); ++i )
				{
					N[i].node.L.sym = libmaus::util::NumberSerialisation::deserialiseSignedNumber(in);
					N[i].node.L.cnt = libmaus::util::NumberSerialisation::deserialiseNumber(in);
				}
				for ( uint64_t i = 0; i < inner(); ++i )
				{	
					uint64_t const lr = libmaus::util::NumberSerialisation::deserialiseNumber(in);
					N[leafs()+i].node.I.left  = (lr >> 32) & 0xFFFFFFFFULL;
					N[leafs()+i].node.I.right = (lr >>  0) & 0xFFFFFFFFULL;
					N[leafs()+i].node.I.cnt   = libmaus::util::NumberSerialisation::deserialiseNumber(in);
				}
			
				setcode  = libmaus::util::NumberSerialisation::deserialiseNumber(in);
				treeroot = libmaus::util::NumberSerialisation::deserialiseNumber(in);
			
				if ( ! in )
				{
					libmaus::exception::LibMausException ex;
					ex.getStream() << "HuffmanTree: failed to deserialise tree." << std::endl;
					ex.finish();
					throw ex;
				}
			}

			HuffmanTree(std::istream & in, uint64_t & s) 
			: N(libmaus::util::NumberSerialisation::deserialiseNumber(in),false), setcode(false), treeroot(0)
                                      {
				for ( uint64_t i = 0; i < leafs(); ++i )
				{
					N[i].node.L.sym = libmaus::util::NumberSerialisation::deserialiseSignedNumberCount(in,s);
					N[i].node.L.cnt = libmaus::util::NumberSerialisation::deserialiseNumberCount(in,s);
				}
				for ( uint64_t i = 0; i < inner(); ++i )
				{	
					uint64_t const lr = libmaus::util::NumberSerialisation::deserialiseNumberCount(in,s);
					N[leafs()+i].node.I.left  = (lr >> 32) & 0xFFFFFFFFULL;
					N[leafs()+i].node.I.right = (lr >>  0) & 0xFFFFFFFFULL;
					N[leafs()+i].node.I.cnt   = libmaus::util::NumberSerialisation::deserialiseNumberCount(in,s);
				}
				setcode  = libmaus::util::NumberSerialisation::deserialiseNumberCount(in,s);
				treeroot = libmaus::util::NumberSerialisation::deserialiseNumberCount(in,s);
			
				if ( ! in )
				{
					libmaus::exception::LibMausException ex;
					ex.getStream() << "HuffmanTree: failed to deserialise tree." << std::endl;
					ex.finish();
					throw ex;
				}
			}
			
			template<typename stream_type>
			uint64_t serialise(stream_type & out) const
			{
				uint64_t o = 0;
				
				o += libmaus::util::NumberSerialisation::serialiseNumber(out,N.size());
				for ( uint64_t i = 0; i < leafs(); ++i )
				{
					o += libmaus::util::NumberSerialisation::serialiseSignedNumber(out,N[i].node.L.sym);
					o += libmaus::util::NumberSerialisation::serialiseNumber(out,N[i].node.L.cnt);
				}
				for ( uint64_t i = 0; i < inner(); ++i )
				{
					o += libmaus::util::NumberSerialisation::serialiseNumber(out,
						(static_cast<uint64_t>(N[leafs()+i].node.I.left)<<32) |
						(static_cast<uint64_t>(N[leafs()+i].node.I.right)<<0)
					);
					o += libmaus::util::NumberSerialisation::serialiseNumber(out,N[leafs()+i].node.I.cnt);
				}
				o += libmaus::util::NumberSerialisation::serialiseNumber(out,setcode);
				o += libmaus::util::NumberSerialisation::serialiseNumber(out,treeroot);
				
				return o;
			}
			
			std::string serialise() const
			{
				std::ostringstream ostr;
				serialise(ostr);
				return ostr.str();
			}
			
			uint64_t serialisedSize() const
			{
				return serialise().size();
			}
			
			// copy constructor
			HuffmanTree(HuffmanTree const & o)
			: N(o.N.size()), setcode(o.setcode), treeroot(o.treeroot)
			{
				uint64_t const l = o.leafs();
				uint64_t const i = o.inner();
				uint64_t p = 0;
				
				for ( uint64_t j = 0; j < l; ++j, ++p ) N[p].node.L = o.N[p].node.L;
				for ( uint64_t j = 0; j < i; ++j, ++p ) N[p].node.I = o.N[p].node.I;
			}
			
			// construct tree from symbols by value (equal freq)
			template<typename symbol_type>
			HuffmanTree(std::vector<symbol_type> const & syms)
			: N(syms.size() ? (2*syms.size()-1) : 0, false), setcode(true),  treeroot(syms.size() ? (N.size()-1) : 0)
			{
				// fill leafs				
				for ( uint64_t i = 0; i < syms.size(); ++i )
				{
					N[i].node.L.sym = syms[i];
					N[i].node.L.cnt = 0;
				}

				// sort leafs by symbol (all counts are zero)
				std::stable_sort(N.begin(),N.begin()+syms.size(),HuffmanNodeLeafComparator());

				// copy symbol to count
				for ( uint64_t i = 0; i < syms.size(); ++i )
					N[i].node.L.cnt = N[i].node.L.sym;
				
				typedef std::pair<uint64_t,uint64_t> upair;
				std::deque<upair> Q;
				
				if ( syms.size() > 1 )
					Q.push_back(upair(0,syms.size()));
				
				uint64_t nodeid = 0;	
				while ( Q.size() )
				{
					upair const P = Q.front();
					
					// clip off top bit if top is equal for all symbols
					while ( 
						libmaus::math::numbits(N[P.first].node.L.cnt)
						==
						libmaus::math::numbits(N[P.second-1].node.L.cnt)
					)
					{
						unsigned int const bits = libmaus::math::numbits(N[P.first].node.L.cnt);
						assert ( bits );
						uint64_t const mask = ~(1ull << (bits-1));
							
						for ( uint64_t i = P.first; i < P.second; ++i )
							N[i].node.L.cnt &= mask;
					}
					
					uint64_t const shift = libmaus::math::numbits(N[P.second-1].node.L.cnt)-1;
					uint64_t cnt[2] = {0,0};
					for ( uint64_t i = P.first; i < P.second; ++i )
					{
						assert ( (N[i].node.L.cnt >> shift) < 2 );
						cnt[N[i].node.L.cnt >> shift]++;
					}
					
					assert ( cnt[0] * cnt[1] );

					uint64_t insinner = N.size()-nodeid-1;
					// leaf
					if ( cnt[0] == 1 )
						N[insinner].node.I.left = P.first;
					else
					{
						N[insinner].node.I.left = N.size()-(nodeid+Q.size())-1;
						Q.push_back(upair(P.first,P.first+cnt[0]));
					}
					if ( cnt[1] == 1 )
						N[insinner].node.I.right = P.second-1;
					else
					{
						N[insinner].node.I.right = N.size()-(nodeid+Q.size())-1;
						Q.push_back(upair(P.second-cnt[1],P.second));
					}
						
					Q.pop_front();
					nodeid++;
				}

				assert ( nodeid == N.size()/2 );

				// set depth of tree root
				if ( nodeid )
					N[N.size()-1].node.I.cnt = 0;
				else if ( syms.size() )
					N[N.size()-1].node.L.cnt = 0;
			
				for ( uint64_t i = 0; i < nodeid; ++i )
				{
					uint64_t const j = N.size()-i-1;
					
					if ( N[j].node.I.left < syms.size() )
						N [ N[j].node.I.left ] . node . L . cnt = N[j].node.I.cnt+1;
					else
						N [ N[j].node.I.left ] . node . I . cnt = N[j].node.I.cnt+1;

					if ( N[j].node.I.right < syms.size() )
						N [ N[j].node.I.right ] . node . L . cnt = N[j].node.I.cnt+1;
					else
						N [ N[j].node.I.right ] . node . I . cnt = N[j].node.I.cnt+1;
				}
				
				uint64_t maxdepth = 0;
				for ( uint64_t i = 0; i < leafs(); ++i )
					maxdepth = std::max(maxdepth,N[i].node.L.cnt);

				if ( maxdepth > 58 )
				{
					libmaus::exception::LibMausException se;
					se.getStream() << "HuffmanTree: cannot store code in tree for maximal depth " << maxdepth << " exceeding 58" << std::endl;
					se.finish();
					throw se;
				}
			
				for ( uint64_t i = 0; i < inner(); ++i )
				{
					uint64_t const j = N.size()-i-1;

					uint64_t const depth = (N[j].node.I.cnt) & 0x3F;
					uint64_t const code  = (N[j].node.I.cnt) >> 6;
					uint64_t const leftword  = (depth+1) | (((code << 1) | 0) << 6);
					uint64_t const rightword = (depth+1) | (((code << 1) | 1) << 6);
					
					if ( N[j].node.I.left < leafs() )
						N [ N[j].node.I.left ] . node . L . cnt = leftword;
					else
						N [ N[j].node.I.left ] . node . I . cnt = leftword;

					if ( N[j].node.I.right < leafs() )
						N [ N[j].node.I.right ] . node . L . cnt = rightword;
					else
						N [ N[j].node.I.right ] . node . I . cnt = rightword;
				}

				reorderByDfs();
			}
			
			// construct tree from array of pairs (sym,freq)
			template<typename iterator>
			HuffmanTree(iterator F, uint64_t const s, bool const sortbydepth = false, bool const rsetcode = false, bool const rdfsorder = false)
			: N(s ? (2*s-1) : 0 , false), setcode(rsetcode), treeroot(s ? (N.size()-1) : 0)
			{
				for ( uint64_t i = 0; i < s; ++i, ++F )
				{
					N[i].node.L.sym = F->first;
					N[i].node.L.cnt = F->second;
				}
				// sort leafs by freqs
				std::stable_sort(N.begin(),N.begin()+s,HuffmanNodeLeafComparator());
				
				uint64_t procleafs = 0;
				uint64_t procinner = 0;
				uint64_t insinner = s;
				uint64_t unprocleafs = s;
				uint64_t unprocinner = 0;
				
				while ( unprocleafs + unprocinner > 1 )
				{
					uint64_t n_a[2];
					uint64_t c = 0;
					for ( uint64_t j = 0; j < 2; ++j )
					{
						if ( unprocleafs && unprocinner )
						{
							if ( N[procleafs].node.L.cnt <= N[s+procinner].node.I.cnt )
							{
								c += N[procleafs].node.L.cnt;
								n_a[j] = procleafs++; unprocleafs--;
							}
							else
							{
								c += N[s+procinner].node.I.cnt;
								n_a[j] = s+procinner++; unprocinner--;
							}
						}
						else if ( unprocleafs )
						{				
							c+= N[procleafs].node.L.cnt;
							n_a[j] = procleafs++; unprocleafs--;
						}
						else
						{
							c += N[s+procinner].node.I.cnt;
							n_a[j] = s+procinner++; unprocinner--;
						}
					}
					
					N[insinner].node.I.left = n_a[0];
					N[insinner].node.I.right = n_a[1];
					N[insinner].node.I.cnt = c;
					insinner++;
					unprocinner++;
				}
				
				// mark root as processed
				if ( unprocinner )
				{
					unprocinner--;
					procinner++;
				}
			
				// replace symbol counts by node depth
				
				// set depth of tree root
				if (	 procinner )
					N[N.size()-1].node.I.cnt = 0;
				else if ( s )
					N[N.size()-1].node.L.cnt = 0;
			
				for ( uint64_t i = 0; i < procinner; ++i )
				{
					uint64_t const j = N.size()-i-1;
					
					if ( N[j].node.I.left < s )
						N [ N[j].node.I.left ] . node . L . cnt = N[j].node.I.cnt+1;
					else
						N [ N[j].node.I.left ] . node . I . cnt = N[j].node.I.cnt+1;

					if ( N[j].node.I.right < s )
						N [ N[j].node.I.right ] . node . L . cnt = N[j].node.I.cnt+1;
					else
						N [ N[j].node.I.right ] . node . I . cnt = N[j].node.I.cnt+1;
				}
				
				uint64_t maxdepth = 0;
				for ( uint64_t i = 0; i < leafs(); ++i )
					maxdepth = std::max(maxdepth,N[i].node.L.cnt);

				if ( setcode )
				{
					if ( maxdepth > 58 )
					{
						libmaus::exception::LibMausException se;
						se.getStream() << "HuffmanTree: cannot store code in tree for maximal depth " << maxdepth << " exceeding 58" << std::endl;
						se.finish();
						throw se;
					}
				
					for ( uint64_t i = 0; i < procinner; ++i )
					{
						uint64_t const j = N.size()-i-1;

						uint64_t const depth = (N[j].node.I.cnt) & 0x3F;
						uint64_t const code  = (N[j].node.I.cnt) >> 6;
						uint64_t const leftword  = (depth+1) | (((code << 1) | 0) << 6);
						uint64_t const rightword = (depth+1) | (((code << 1) | 1) << 6);
						
						if ( N[j].node.I.left < s )
							N [ N[j].node.I.left ] . node . L . cnt = leftword;
						else
							N [ N[j].node.I.left ] . node . I . cnt = leftword;

						if ( N[j].node.I.right < s )
							N [ N[j].node.I.right ] . node . L . cnt = rightword;
						else
							N [ N[j].node.I.right ] . node . I . cnt = rightword;
					}
				}
				
				if ( sortbydepth )
					sortSymbols();
			
				if ( rdfsorder )
					reorderByDfs();
			}

			// assignment operator
			HuffmanTree & operator=(HuffmanTree const & o)
			{
				if ( this != &o )
				{
					uint64_t const l = o.leafs();
					uint64_t const i = o.inner();
					uint64_t p = 0;
				
					N = libmaus::autoarray::AutoArray<HuffmanNode>(o.N.size(),false);
					for ( uint64_t j = 0; j < l; ++j, ++p ) N[p].node.L = o.N[p].node.L;
					for ( uint64_t j = 0; j < i; ++j, ++p ) N[p].node.I = o.N[p].node.I;
					
					setcode = o.setcode;
					treeroot = o.treeroot;
				}
				
				return *this;
			}
			
			unique_ptr_type uclone() const
			{
				unique_ptr_type ptr(new this_type(*this));
				return UNIQUE_PTR_MOVE(ptr);
			}
			
			void printRec(std::ostream & out, uint64_t const node, uint64_t const indent = 0) const
			{
				out << std::string(indent,' ');
				
				if ( node < leafs() )
					out << "leaf(" << N[node].node.L.sym << "," << N[node].node.L.cnt << ")" << std::endl;
				else
				{
					out << "inner(" << N[node].node.I.cnt << ")" << std::endl;
					
					printRec(out,N[node].node.I.left,indent+1);
					printRec(out,N[node].node.I.right,indent+1);
				}
			}
			
			std::string toString() const
			{
				std::ostringstream ostr;
				printRec(ostr,root());
				return ostr.str();
			}
			
			uint64_t root() const
			{
				return treeroot;
			}
			
			bool isLeaf(uint64_t const i) const
			{
				return i < leafs();
			}
			
			int64_t getSymbol(uint64_t const i) const
			{
				assert ( isLeaf(i) );
				return N[i].node.L.sym;
			}
			
			uint64_t leftChild(uint64_t const i) const
			{
				return N[i].node.I.left;
			}

			uint64_t rightChild(uint64_t const i) const
			{
				return N[i].node.I.right;
			}
			
			static std::ostream & printCode(std::ostream & out, uint64_t const c, unsigned int const b)
			{
				for ( uint64_t i = 0; i < b; ++i )
					out << ((c & (1ull << (b-i-1))) != 0);
				return out;
			}

			static std::ostream & printCode(std::ostream & out, uint64_t const c)
			{
				return printCode(out,c >> 6,c & 0x3F);
			}
			
			std::ostream & printLeafCodes(std::ostream & out)
			{
				for ( uint64_t i = 0; i < leafs(); ++i )
				{
					out << N[i].node.L.sym << "\t";
					printCode(out,N[i].node.L.cnt);
					out << std::endl;
				}
				return out;
			}
			
			bool operator==(HuffmanTree const & o) const
			{
				if ( N.size() != o.N.size() )
					return false;
				
				for ( uint64_t i = 0; i < leafs(); ++i )
					if ( N[i].node.L != o.N[i].node.L )
						return false;
				
				for ( uint64_t i = 0; i < inner(); ++i )
					if ( N[leafs()+i].node.I != o.N[leafs()+i].node.I )
						return false;
						
				if ( setcode != o.setcode )
					return false;
				if ( treeroot != o.treeroot )
					return false;
						
				return true;
			}
			
			bool operator!=(HuffmanTree const & o) const
			{
				return ! ((*this) == o);
			}
			
			template<typename stream_type>
			int64_t decodeSlow(stream_type & stream) const
			{
				uint64_t cur = root();
				
				while ( !isLeaf(cur) )
				{
					bool const b = stream.readBit();
					
					if ( b )
						cur = rightChild(cur);
					else
						cur = leftChild(cur);
				}
				
				return N[cur].node.L.sym;
			}
			
			libmaus::autoarray::AutoArray<int64_t> symbolArray() const
			{
				libmaus::autoarray::AutoArray<int64_t> A(leafs(),false);
				for ( uint64_t i = 0; i < leafs(); ++i )
					A[i] = N[i].node.L.sym;
				std::sort(A.begin(),A.end());
				return A;
			}
			
			uint64_t maxDepth() const
			{
				if ( ! setcode )
				{
					libmaus::exception::LibMausException se;
					se.getStream() << "HuffmanTree::maxDepth: cannot compute depth for object constructed with setcode option unset" << std::endl;
					se.finish();
					throw se;					
				}
			
				uint64_t maxdepth = 0;
				for ( uint64_t i = 0; i < leafs(); ++i )
					maxdepth = std::max(maxdepth,N[i].node.L.cnt & 0x3F);
					
				return maxdepth;
			}

			int64_t maxSymbol() const
			{
				int64_t maxsym = std::numeric_limits<int64_t>::min();
				for ( uint64_t i = 0; i < leafs(); ++i )
					maxsym = std::max(maxsym,N[i].node.L.sym);
					
				return maxsym;
			}
			
			struct EncodeTable
			{
				typedef EncodeTable this_type;
				typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
				libmaus::autoarray::AutoArray<uint64_t>	C;
				libmaus::bitio::BitVector::unique_ptr_type B;
				int64_t minsym;
				int64_t maxsym;
				
				EncodeTable(HuffmanTree const & H)
				{
					if ( ! H.setcode )
					{
						libmaus::exception::LibMausException se;
						se.getStream() << "HuffmanTree::EncodeTable: cannot construct table for object constructed with setcode option unset" << std::endl;
						se.finish();
						throw se;					
					}
				
					if ( H.leafs() )
						minsym = maxsym = H.N[0].node.L.sym;
						
					for ( uint64_t i = 1; i < H.leafs(); ++i )
					{
						minsym = std::min(minsym,H.N[i].node.L.sym);
						maxsym = std::max(maxsym,H.N[i].node.L.sym);
					}
					
					libmaus::bitio::BitVector::unique_ptr_type tB(new libmaus::bitio::BitVector(maxsym-minsym+1));
					B = UNIQUE_PTR_MOVE(tB);
					
					// set up table
					C = libmaus::autoarray::AutoArray<uint64_t>((maxsym-minsym+1),false);
					
					// copy codes
					for ( uint64_t i = 0; i < H.leafs(); ++i )
					{
						C [ H.N[i].node.L.sym - minsym ] = H.N[i].node.L.cnt;
						B->set(H.N[i].node.L.sym - minsym,true);
					}
				}
				
				bool hasSymbol(int64_t const i) const
				{
					return i >= minsym && i <= maxsym && B->get(i-minsym);
				}
				
				uint64_t getCode(int64_t const i) const
				{
					return C[i-minsym] >> 6;
				}
				
				unsigned int getCodeLength(int64_t const i) const
				{
					return C[i-minsym] & 0x3F;
				}
				
				bool getBitFromTop(int64_t const sym, unsigned int const level) const
				{
					assert ( hasSymbol(sym) );
					uint64_t const code = getCode(sym);
					unsigned int const len = getCodeLength(sym);
					return code & (1ull << (len-level-1));
				}
			};
		};

		::std::ostream & operator<<(::std::ostream & out, HuffmanTree const & H);
		::std::ostream & operator<<(::std::ostream & out, HuffmanTree::EncodeTable const & E);
	}
}
#endif
