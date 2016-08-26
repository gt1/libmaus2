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

#if ! defined(WAVELETTREE_HPP)
#define WAVELETTREE_HPP

#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/serialize/Serialize.hpp>
#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/util/BitsPerNum.hpp>
#include <libmaus2/bitio/getBit.hpp>
#include <libmaus2/bitio/BitWriter.hpp>

#if defined(_MSC_VER) || defined(__MINGW32__)
#include "malloc.h"
#endif

#include <iterator>
#include <limits>
#include <stdexcept>
#include <deque>
#include <stack>
#include <cassert>
#include <queue>

#include <iostream>

namespace libmaus2
{
	namespace wavelet
	{
		template<typename symbol_type, bool issigned>
		struct GetNumBits
		{
			private:
			template<typename it>
			static uint64_t getNumBits(it /*a */, uint64_t const /* n */) { return 0; }
		};
		template<typename symbol_type>
		struct GetNumBits<symbol_type, false>
		{
			/**
			 * return maximal number of bits required to store a number in a
			 *
			 * @param a sequence of numbers
			 * @param n length of sequence a
			 * @return maximal number of bits required
			 **/
			template<typename it>
			static uint64_t getNumBits(it a, uint64_t const n)
			{
				if ( n )
				{
					// typedef typename std::iterator_traits<it>::value_type symbol_type;
					symbol_type maxi = ::std::numeric_limits< symbol_type > :: min();
					symbol_type mini = ::std::numeric_limits< symbol_type > :: max();

					for ( uint64_t i = 0; i < n; ++i )
					{
						symbol_type const v = *(a++);
						maxi = ::std::max(v,maxi),
						mini = ::std::min(v,mini);
					}

					uint64_t const bits = ::libmaus2::util::BitsPerNum::bitsPerNum(maxi);

					// ::std::cerr << "mini=" << mini << " maxi=" << maxi << " bits=" << bits << ::std::endl;

					return bits;
				}
				else
				{
					return 0;
				}
			}
		};
		template<typename symbol_type>
		struct GetNumBits<symbol_type, true>
		{
			/**
			 * return maximal number of bits required to store a number in a
			 *
			 * @param a sequence of numbers
			 * @param n length of sequence a
			 * @return maximal number of bits required
			 **/
			template<typename it>
			static uint64_t getNumBits(it a, uint64_t const n)
			{
				if ( n )
				{
					// typedef typename std::iterator_traits<it>::value_type symbol_type;
					symbol_type maxi = ::std::numeric_limits< symbol_type > :: min();
					symbol_type mini = ::std::numeric_limits< symbol_type > :: max();

					for ( uint64_t i = 0; i < n; ++i )
					{
						symbol_type const v = *(a++);
						maxi = ::std::max(v,maxi),
						mini = ::std::min(v,mini);
					}

					if ( ::std::numeric_limits<symbol_type>::is_signed && mini < symbol_type() )
						throw ::std::runtime_error("::libmaus2::wavelet::WaveletTree: cannot handle negative values.");

					uint64_t const bits = ::libmaus2::util::BitsPerNum::bitsPerNum(maxi);

					// ::std::cerr << "mini=" << mini << " maxi=" << maxi << " bits=" << bits << ::std::endl;

					return bits;
				}
				else
				{
					return 0;
				}
			}
		};

		/**
		 * wavelet tree allowing rank/select on non-binary sequences in log(Sigma) time
		 **/
		template<typename _rank_type, typename _symbol_type>
		struct WaveletTree
		{
			public:
			typedef _rank_type rank_type;
			typedef _symbol_type symbol_type;

			typedef WaveletTree<rank_type,symbol_type> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			private:
			typedef typename rank_type::writer_type writer_type;
			typedef typename writer_type::data_type data_type;

			/**
			 * number of symbols in stream
			 **/
			uint64_t const n;
			/**
			 * bits per symbol
			 **/
			uint64_t const b;

			/**
			 * wavelet tree bit layers (n log S) bits
			 **/
			::libmaus2::autoarray::AutoArray< data_type > AW;
			data_type const * const W;
			/**
			 * rank dictionary on W
			 **/
			rank_type R;


			bool operator()(uint64_t const l, uint64_t const i) const
			{
				return ::libmaus2::bitio::getBit(W, l*n+i);
			}

			/**
			 * align number a of bits to 64 by rounding up
			 *
			 * @param a
			 * @return ((a+63)/64)*64
			 *
			 **/
			static inline uint64_t align64(uint64_t const a)
			{
				return ((a+63)/64)*64;
			}
			/**
			 * return ceil(a/b)
			 *
			 * @param a
			 * @param b
			 * @return ceil(a/b)
			 **/
			static inline uint64_t divup(uint64_t const a, uint64_t const b)
			{
				return (a+(b-1))/b;
			}
			/**
			 * return number of bits required to store number k
			 *
			 * @param k
			 * @return number of bits required to store k
			 **/
			static inline uint64_t bitsPerNum(uint64_t k)
			{
				uint64_t c = 0;

				while ( k & (~0xFFFF) ) { k >>= 16; c += 16; }
				if    ( k & (~0x00FF) ) { k >>=  8; c +=  8; }
				if    ( k & (~0x000F) ) { k >>=  4; c +=  4; }
				if    ( k & (~0x0003) ) { k >>=  2; c +=  2; }
				if    ( k & (~0x0001) ) { k >>=  1; c +=  1; }
				if    ( k             ) { k >>=  1; c +=  1; }

				return c;
			}
			/**
			 * return maximal number of bits required to store a number in a
			 *
			 * @param a sequence of numbers
			 * @param n length of sequence a
			 * @return maximal number of bits required
			 **/
			template<typename it>
			static uint64_t getNumBits(it a, uint64_t const n)
			{
				return ::libmaus2::wavelet::GetNumBits<
					symbol_type,
					::std::numeric_limits<symbol_type>::is_signed > ::getNumBits(a,n);

				#if 0
				if ( n )
				{
					// typedef typename std::iterator_traits<it>::value_type symbol_type;
					symbol_type maxi = ::std::numeric_limits< symbol_type > :: min();
					symbol_type mini = ::std::numeric_limits< symbol_type > :: max();

					for ( uint64_t i = 0; i < n; ++i )
					{
						symbol_type const v = *(a++);
						maxi = ::std::max(v,maxi),
						mini = ::std::min(v,mini);
					}

					if ( ::std::numeric_limits<symbol_type>::is_signed && mini < symbol_type() )
						throw ::std::runtime_error("wavelet::WaveletTree::getNumBits(): cannot handle negative values.");

					uint64_t const bits = bitsPerNum(maxi);

					// ::std::cerr << "mini=" << mini << " maxi=" << maxi << " bits=" << bits << ::std::endl;

					return bits;
				}
				else
				{
					return 0;
				}
				#endif
			}

			/**
			 * produce wavelet tree bits with b layers from sequence a of length n.
			 * uses ~ 2n * sizeof(symbol_type) + (1<<b) * sizeof(uint64_t) bytes
			 * of memory in addition to output of size n*b bits
			 *
			 * @param a
			 * @param n
			 * @param b
			 * @return bits
			 **/
			template<typename it>
			static ::libmaus2::autoarray::AutoArray< data_type > produceBits(it a, uint64_t const n, uint64_t const b)
			{
				::libmaus2::autoarray::AutoArray< data_type > W( divup( align64(n*b), 8*sizeof(data_type) ), false );

				// ::std::cerr << "n=" << n << " b=" << b << " t=" << t << " w=" << w << ::std::endl;

				::libmaus2::autoarray::AutoArray<symbol_type> AA0(n,false), AA1(n,false);
				symbol_type * A0 = AA0.get(), * A1 = AA1.get();

				for ( uint64_t i = 0; i < n; ++i )
					A0[i] = *(a++);

				writer_type writer(W.get());

				::std::deque< uint64_t > T;
				T.push_back( n );

				uint64_t m = (1ull<<(b-1));

				for ( uint64_t ib = 0; (ib+1) < b; ++ib, m>>=1 )
				{
					::std::deque< uint64_t > Tn;

					uint64_t left = 0;

					while ( T.size() )
					{
						uint64_t const right = left + T.front(); T.pop_front();

						uint64_t pch = left;

						for ( uint64_t i = left; i < right; ++i )
							if ( A0[ i ] & m )
							{
								writer.writeBit(1);
							}
							else
							{
								writer.writeBit(0);
								pch++;
							}

						uint64_t pcl = left;

						if ( pch - left )
							Tn.push_back( pch-left );
						if ( right - pch )
							Tn.push_back( right-pch );

						for ( uint64_t i = left; i < right; ++i )
							if ( A0[ i ] & m )
								A1[pch++] = A0[i];
							else
								A1[pcl++] = A0[i];

						left = right;
					}

					T.swap(Tn);
					::std::swap(A0,A1);
				}

				uint64_t left = 0;

				while ( T.size() )
				{
					uint64_t const right = left + T.front(); T.pop_front();

					for ( uint64_t i = left; i < right; ++i )
						writer.writeBit( (A0[ i ] & m) != 0 );

					left = right;
				}

				if ( n * b )
					writer.flush();

				#if 0
				for ( uint64_t i = 0; i < n; ++i )
					::std::cerr << i % 10;
				::std::cerr << ::std::endl;
				for ( uint64_t ib = 0; ib < b; ++ib )
				{
					for ( uint64_t i = 0; i < n; ++i )
						::std::cerr << ::libmaus2::bitio::getBit(W.get(),ib*n+i);
					::std::cerr << ::std::endl;
				}
				#endif

				return W;
			}

			struct Node
			{
				uint64_t level;
				uint64_t left;
				uint64_t right;

				Node() : level(0), left(0), right(0) {}
				Node(uint64_t const rlevel, uint64_t const rleft, uint64_t const rright)
				: level(rlevel), left(rleft), right(rright) {}
			};

			/**
			 * rank0 on node
			 **/
			uint64_t rank0(Node const & node, uint64_t const i) const
			{
				// zeroes up to node
				uint64_t const o0 = node.level * n + node.left ? R.rank0(node.level * n + node.left - 1ull) : 0ull;
				// zeroes up to position i in node
				uint64_t const n0 = R.rank0(node.level * n + node.left + i);
				// return difference
				return n0-o0;
			}
			/**
			 * rank1 on node
			 **/
			uint64_t rank1(Node const & node, uint64_t const i) const
			{
				// ones up to node
				uint64_t const o1 = node.level * n + node.left ? R.rank1(node.level * n + node.left - 1ull) : 0ull;
				// ones up to position i in node
				uint64_t const n1 = R.rank1(node.level * n + node.left + i);
				// return difference
				return n1-o1;
			}
			/**
			 * bit access on node
			 **/
			uint64_t operator()(Node const & node, uint64_t i) const
			{
				return (*this)( node.level, node.left + i );
			}

			struct NodePortion
			{
				Node node;
				uint64_t left;
				uint64_t right;

				NodePortion() : node(), left(0), right(0) {}
				NodePortion(Node const & rnode, uint64_t const rleft, uint64_t const rright)
				: node(rnode), left(rleft), right(rright) {}
			};



			/**
			 * rank0 on node portion
			 **/
			uint64_t rank0(NodePortion const & node, uint64_t const i) const
			{
				uint64_t const lb = node.node.level * n + node.node.left + node.left;

				// zeroes up to node portion
				uint64_t const o0 =  lb ? R.rank0(lb - 1ull) : 0ull;
				// zeroes up to position i in node
				uint64_t const n0 = R.rank0(lb + i);

				// ::std::cerr << "r0: lb=" << lb << " o0=" << o0 << " n0=" << n0 << " i=" << i << ::std::endl;

				// return difference
				return n0-o0;
			}

			/**
			 * number of 0 bits in node portion
			 **/
			uint64_t rank0(NodePortion const & node) const
			{
				if ( node.right-node.left )
				{
					return rank0(node, node.right-node.left-1);
				}
				else
				{
					return 0;
				}
			}

			/**
			 * rank1 on node portion
			 **/
			uint64_t rank1(NodePortion const & node, uint64_t i) const
			{
				uint64_t const lb = node.node.level * n + node.node.left + node.left;

				// zeroes up to node portion
				uint64_t const o1 =  lb ? R.rank1(lb - 1ull) : 0ull;
				// zeroes up to position i in node
				uint64_t const n1 = R.rank1(lb + i);

				// ::std::cerr << "r1: lb=" << lb << " o1=" << o1 << " n1=" << n1 << " i=" << i << ::std::endl;

				// return difference
				return n1-o1;
			}

			/**
			 * number of 1 bits in node portion
			 **/
			uint64_t rank1(NodePortion const & node) const
			{
				if ( node.right-node.left )
				{
					return rank1(node, node.right-node.left-1);
				}
				else
				{
					return 0;
				}
			}


			Node root() const
			{
				return Node(0,0,n);
			}

			bool getNodeBit(Node const & node, uint64_t i) const
			{
				return ::libmaus2::bitio::getBit( W, node.level*n + node.left + i );
			}

			/**
			 * get left child
			 **/
			Node leftChild(Node const & node) const
			{
				// if ( node.level + 1 < b )
				if ( node.level < b )
				{
					// zero bits up to node
					uint64_t const p0 = (node.level * n + node.left) ?
						R.rank0( node.level * n + node.left - 1ull ) : 0ull;
					// zero bits up to end of node
					uint64_t const n0 = (node.level * n + node.right) ?
						R.rank0( node.level * n + node.right - 1ull) : 0ull;
					//
					return Node(node.level+1ull, node.left, node.left + n0 - p0);
				}
				else
				{
					return Node(node.level+1ull, 0ull, 0ull);
				}
			}
			/**
			 * get right child
			 **/
			Node rightChild(Node const & node) const
			{
				// if ( node.level + 1 < b )
				if ( node.level < b )
				{
					// one bits up to node
					uint64_t const p0 = (node.level * n + node.left) ?
						R.rank1(node.level * n + node.left - 1ull) : 0ull;
					// one bits up to end of node
					uint64_t const n0 = (node.level * n + node.right) ?
						R.rank1(node.level * n + node.right - 1ull) : 0ull;
					//
					return Node(node.level+1ull, node.right- (n0-p0), node.right);
				}
				else
				{
					return Node(node.level+1ull, 0ull, 0ull);
				}
			}

			NodePortion root(uint64_t const l, uint64_t const r) const
			{
				return NodePortion(Node(0ull,0ull,n),l,r);
			}

			/**
			 * left child:
			 * left =
			 *	v[left] == 0: rank0(v,left) - 1
			 *	v[left] != 0: rank0(v,left)
			 * right =
			 *	v[right-1] == 0: rank0 ( v, right-1 )
			 *	v[right-1] != 0: rank0 ( v, right-1 )
			 **/
			NodePortion leftChild(NodePortion const & node) const
			{
				NodePortion child;
				child.node = leftChild(node.node);

				if (
					child.node.right-child.node.left
					&&
					node.right - node.left
				)
				{
					child.left = ((*this)(node.node,node.left) == 0) ? (rank0(node.node,node.left)-1) :
						rank0(node.node,node.left);
					child.right = rank0(node.node,node.right-1);

					if ( child.left == child.right )
						child.left = child.right = 0;
				}
				else
				{
					child.left = child.right = 0;
				}

				return child;
			}
			/**
			 * right child:
			 * left =
			 *	v[left] == 1: rank1(v,left) - 1
			 *	v[left] != 1: rank1(v,left)
			 * right =
			 *	v[right-1] == 1: rank1 ( v, right-1 )
			 *	v[right-1] != 1: rank1 ( v, right-1 )
			 **/
			NodePortion rightChild(NodePortion const & node) const
			{
				NodePortion child;
				child.node = rightChild(node.node);

				if (
					child.node.right-child.node.left
					&&
					node.right - node.left
				)
				{
					child.left = ((*this)(node.node,node.left) == 1) ? (rank1(node.node,node.left)-1) :
						rank1(node.node,node.left);
					child.right = rank1(node.node,node.right-1);

					if ( child.left == child.right )
						child.left = child.right = 0;
				}
				else
				{
					child.left = child.right = 0;
				}

				return child;
			}

			struct RangeCountStackElement : public NodePortion
			{
				symbol_type sym;
				unsigned int visit;

				RangeCountStackElement() : NodePortion(), sym(0), visit(0) {}
				RangeCountStackElement(NodePortion const portion) : NodePortion(portion), sym(0), visit(0) {}
				RangeCountStackElement(NodePortion const portion, symbol_type rsym) : NodePortion(portion), sym(rsym), visit(0) {}

				unsigned int depth() const
				{
					return NodePortion::node.level;
				}

				uint64_t range() const
				{
					return NodePortion::right - NodePortion::left;
				}
				bool empty() const
				{
					return range() == 0;
				}

				bool operator<(RangeCountStackElement const & o) const
				{
					return this->range() < o.range();
				}
			};

			public:
			/**
			 * construct wavelet tree from sequence a of length rn
			 *
			 * @param a
			 * @param rn
			 **/
			template<typename it>
			WaveletTree(it a, uint64_t const rn) : n(rn), b(getNumBits(a,n)), AW( produceBits(a, n, b) ), W(AW.get()), R(W,align64(n*b)) {}

			WaveletTree(::libmaus2::autoarray::AutoArray<uint64_t> & rW,
				    uint64_t const rn,
				    uint64_t const rb)
			: n(rn), b(rb), AW( rW ), W( AW.get() ), R(W,align64(n*b)) {}

			WaveletTree(uint64_t const * const rW,
				    uint64_t const rn,
				    uint64_t const rb)
			: n(rn), b(rb), AW( ), W( rW ), R(W,align64(n*b)) {}

			static uint64_t readUnsignedInt(::std::istream & in)
			{
				uint64_t i;
				::libmaus2::serialize::Serialize<uint64_t>::deserialize(in,&i);
				return i;
			}

			static uint64_t readUnsignedInt(::std::istream & in, uint64_t & s)
			{
				uint64_t i;
				s += ::libmaus2::serialize::Serialize<uint64_t>::deserialize(in,&i);
				return i;
			}

			static ::libmaus2::autoarray::AutoArray<uint64_t> readArray(::std::istream & in)
			{
				::libmaus2::autoarray::AutoArray<uint64_t> A;
				A.deserialize(in);
				return A;
			}

			static ::libmaus2::autoarray::AutoArray<uint64_t> readArray(::std::istream & in, uint64_t & s)
			{
				::libmaus2::autoarray::AutoArray<uint64_t> A;
				s += A.deserialize(in);
				return A;
			}

			WaveletTree(::std::istream & in)
			: n(readUnsignedInt(in)), b(readUnsignedInt(in)), AW ( readArray(in) ), W(AW.get()), R(W,align64(n*b)) {}

			WaveletTree(::std::istream & in, uint64_t & s)
			: n(readUnsignedInt(in,s)), b(readUnsignedInt(in,s)), AW ( readArray(in,s) ), W(AW.get()), R(AW.get(),align64(n*b)) {}

			uint64_t serialize(::std::ostream & out)
			{
				uint64_t s = 0;
				s += ::libmaus2::serialize::Serialize<uint64_t>::serialize(out,n);
				s += ::libmaus2::serialize::Serialize<uint64_t>::serialize(out,b);
				s += AW.serialize(out);
				return s;
			}

			uint64_t getN() const
			{
				return n;
			}
			uint64_t getB() const
			{
				return b;
			}
			uint64_t const * getW() const
			{
				return W;
			}
			uint64_t size() const
			{
				return n;
			}

			::libmaus2::autoarray::AutoArray<int64_t> getSymbolArray() const
			{
				::libmaus2::autoarray::AutoArray<int64_t> A(1ull << getB());
				for ( uint64_t i = 0; i < A.size(); ++i )
					A[i] = i;
				return A;
			}

			/**
			 * return the index of the i'th occurence of k, or ::std::numeric_limits<uint64_t>::max(), if no such index exists
			 *
			 * @param k
			 * @param i
			 * @return index of the i'th occurence of k
			 **/
			uint64_t select(uint64_t const k, uint64_t i) const
			{
				// check whether value k requires too many bits (i.e. is too large)
				if ( k & (~((1ull<<b) - 1)) )
					return ::std::numeric_limits<uint64_t>::max();

				uint64_t left = 0, right = n, offset = 0, m = (1ull<<(b-1));

				// stack for b = log( Sigma ) pairs of numbers allocated on the runtime stack
	#if defined(_MSC_VER) || defined(__MINGW32__)
				uint64_t * S = reinterpret_cast<uint64_t *>(_alloca( b * sizeof(uint64_t) ));
	#else
				uint64_t * S = reinterpret_cast<uint64_t *>(alloca( b * sizeof(uint64_t) ));
	#endif

				// go downward
				for ( uint64_t ib = 0; ib < b; ++ib, m>>=1, offset += n )
				{
					*(S++) = left;

					if ( k & m )
						// adjust left
						left = left + R.rank0(offset+right-1) - ((offset+left) ? R.rank0(offset+left-1) : 0);
					else
						// adjust right
						right = right - (R.rank1(offset+right-1) - ((offset+left) ? R.rank1(offset+left-1) : 0));

					// interval too small?
					if ( i >= ( right - left ) )
						return ::std::numeric_limits<uint64_t>::max();
				}

				// go upwards
				m = 1;
				offset -= n;

				for ( uint64_t ib = 0; ib < b; ++ib, m <<= 1, offset -= n )
				{
					left = *(--S);

					if ( k & m )
						i = R.select1( ((offset + left) ? R.rank1(offset+left-1) : 0) + i ) - (offset+left);
					else
						i = R.select0( ((offset + left) ? R.rank0(offset+left-1) : 0) + i ) - (offset+left);

					// ::std::cerr << "key " << k << " left=" << left << " right=" << right << " m=" << m << " i=" << i << ::std::endl;
				}

				return i;
			}

			/**
			 * print tree on stream for debugging
			 * @param out stream
			 **/
			void print(::std::ostream & out) const
			{
				::std::stack<Node> S;
				S.push(root());

				while ( ! S.empty() )
				{
					Node node = S.top(); S.pop();

					if ( node.right - node.left )
					{
						out << "Node(" << node.level << "," <<
							node.left << "," <<
							node.right << ")" << ::std::endl;

						S.push(rightChild(node));
						S.push(leftChild(node));
					}
				}
			}

			/**
			 * return the i'th symbol in the input sequence
			 *
			 * @param i index
			 * @return i'th symbol
			 **/
			symbol_type operator[](size_t i) const
			{
				// return rmq(i,i+1);

				symbol_type v = 0;
				Node node = root();

				for ( unsigned int ib = 0; ib < b; ++ib )
				{
					v <<= 1;

					if ( getNodeBit(node, i) )
					{
						i -= rank0(node, i);
						node = rightChild(node);
						v |= 1;
					}
					else
					{
						i -= rank1(node, i);
						node = leftChild(node);
					}
				}

				return v;
			}

			/**
			 * return the i'th symbol in the input sequence
			 *
			 * @param i index
			 * @return i'th symbol
			 **/
			symbol_type sortedSymbol(uint64_t i) const
			{
				symbol_type v = 0;
				Node node = root();

				for ( unsigned int ib = 0; ib < b; ++ib )
				{
					uint64_t const width = node.right-node.left;
					uint64_t const zrank = rank0(node,width-1);

					// ::std::cerr << "Node(" << node.level << "," << node.left << "," << node.right << ") width=" << width << " zrank=" << zrank << ::std::endl;

					v <<= 1;

					if ( i < zrank )
					{
						// ::std::cerr << "i=" << i << " < rank0()=" << zrank << ::std::endl;
						node = leftChild(node);
					}
					else
					{
						// ::std::cerr << "i=" << i << " >= rank0()=" << zrank << ::std::endl;
						i -= zrank;
						// ::std::cerr << "setting i=" << i << ::std::endl;
						node = rightChild(node);
						v |= 1;
					}
				}

				return v;
			}

			/**
			 * range minimum value query
			 **/
			symbol_type rmq(uint64_t const l, uint64_t const r) const
			{
				assert ( r > l );
				assert ( r <= n );

				NodePortion P = root(l,r);

				symbol_type v = 0;
				for ( uint64_t ib = 0; ib < b; ++ib )
				{
					v <<= 1;

					if ( rank0(P,P.right-P.left-1) )
					{
						P = leftChild(P);
					}
					else
					{
						P = rightChild(P);
						v |= 1;
					}
				}

				return v;
			}

			/**
			 * range maximum value query
			 **/
			symbol_type rmqi(uint64_t const l, uint64_t const r) const
			{
				assert ( r > l );
				assert ( r <= n );

				NodePortion P = root(l,r);

				symbol_type v = 0;
				for ( uint64_t ib = 0; ib < b; ++ib )
				{
					v <<= 1;

					if ( rank1(P,P.right-P.left-1) )
					{
						P = rightChild(P);
						v |= 1;
					}
					else
					{
						P = leftChild(P);
					}
				}

				return v;
			}

			/**
			 * print portion of tree
			 *
			 * @param out stream
			 * @param l left
			 * @param r right
			 **/
			void print(::std::ostream & out, uint64_t const l, uint64_t const r) const
			{
				::std::stack<NodePortion> S;
				S.push(root(l,r));

				while ( ! S.empty() )
				{
					NodePortion const node = S.top(); S.pop();

					if ( node.node.right - node.node.left )
					{
						out << "NodePortion(" << node.node.level << "," <<
							node.node.left << "," <<
							node.node.right << "," <<
							node.left << "," <<
							node.right << ")" << ::std::endl;

						NodePortion const rc = rightChild(node);
						if ( rc.right - rc.left )
							S.push(rc);
						NodePortion const lc = leftChild(node);
						if ( lc.right - lc.left )
							S.push(lc);
					}
				}
			}

			/**
			 * print portion of tree
			 *
			 * @param out stream
			 * @param l left
			 * @param r right
			 * @param k key
			 **/
			void print(::std::ostream & out, uint64_t const l, uint64_t const r, uint64_t const k) const
			{
				::std::stack<NodePortion> S;
				S.push(root(l,r));

				while ( ! S.empty() )
				{
					NodePortion const node = S.top(); S.pop();

					if ( (node.node.right - node.node.left) && (node.node.right) > k )
					{
						out << "NodePortion(" << node.node.level << "," <<
							node.node.left << "," <<
							node.node.right << "," <<
							node.left << "," <<
							node.right << ")" <<
							((node.node.level == b) ? "*" : "") << ::std::endl;

						NodePortion const rc = rightChild(node);
						if ( rc.right - rc.left )
							S.push(rc);
						NodePortion const lc = leftChild(node);
						if ( lc.right - lc.left )
							S.push(lc);
					}
				}
			}

			/**
			 * range next value, return smallest value >= k in A[l,r-1], or ::std::numeric_limits<uint64_t>::max()
			 * if no such value exists
			 *
			 * @param l
			 * @param r
			 * @param k
			 * @return rpv(l,r,k)
			 **/
			uint64_t rnv(uint64_t const l, uint64_t const r, uint64_t const k) const
			{
				// maximum depth of stack is b+1

	#if defined(_MSC_VER) || defined(__MINGW32__)
				::std::pair < NodePortion, bool > * const ST = reinterpret_cast< ::std::pair < NodePortion, bool > *>(_alloca( (b+1)*sizeof(::std::pair < NodePortion, bool >) ));
	#else
				::std::pair < NodePortion, bool > * const ST =
					reinterpret_cast< ::std::pair < NodePortion, bool > *>(alloca( (b+1)*sizeof( ::std::pair < NodePortion, bool >) ));
	#endif

				// stack pointer
				::std::pair < NodePortion, bool > * SP = ST;
				// push root
				*(SP++) = ::std::pair < NodePortion, bool > (root(l,r), false);

				while ( SP != ST )
				{
					std::pair < NodePortion, bool > & P = *(SP-1);

					NodePortion const & node = P.first;
					bool const seenbefore = P.second;

					if ( seenbefore )
					{
						NodePortion const rc = rightChild(node);

						--SP;

						if ( rc.right - rc.left )
							*(SP++) = ::std::pair<NodePortion,bool> (rc,false);
					}
					else if ( (node.node.right - node.node.left) && (node.node.right) > k )
					{
						if ( node.node.level == b )
							return node.node.left;

						NodePortion const lc = leftChild(node);

						P.second = true;

						if ( lc.right - lc.left )
							*(SP++) = ::std::pair<NodePortion,bool> (lc,false);
					}
					else
					{
						--SP;
					}
				}

				return ::std::numeric_limits<uint64_t>::max();
			}

			/**
			 * range previous value, return largest value <= k in A[l,r-1], or ::std::numeric_limits<uint64_t>::max()
			 * if no such value exists
			 *
			 * @param l
			 * @param r
			 * @param k
			 * @return rpv(l,r,k)
			 **/
			uint64_t rpv(uint64_t const l, uint64_t const r, uint64_t const k) const
			{
				// maximum depth of stack is b+1

	#if defined(_MSC_VER) || defined(__MINGW32__)
				::std::pair < NodePortion, bool > * const ST = reinterpret_cast< ::std::pair < NodePortion, bool > *>(_alloca( (b+1)*sizeof(::std::pair < NodePortion, bool >) ));
	#else
				::std::pair < NodePortion, bool > * const ST = reinterpret_cast< ::std::pair < NodePortion, bool > *>(alloca( (b+1)*sizeof(::std::pair < NodePortion, bool >) ));
	#endif

				// stack pointer
				::std::pair < NodePortion, bool > * SP = ST;
				// push root
				*(SP++) = ::std::pair < NodePortion, bool > (root(l,r), false);

				while ( SP != ST )
				{
					::std::pair < NodePortion, bool > & P = *(SP-1);

					NodePortion const & node = P.first;
					bool const seenbefore = P.second;

					if ( seenbefore )
					{
						NodePortion const lc = leftChild(node);

						--SP;

						if ( lc.right - lc.left )
							*(SP++) = ::std::pair<NodePortion,bool> (lc,false);
					}
					else if ( (node.node.right - node.node.left) && (node.node.left) <= k )
					{
						if ( node.node.level == b )
							return node.node.left;

						NodePortion const rc = rightChild(node);

						P.second = true;

						if ( rc.right - rc.left )
							*(SP++) = ::std::pair<NodePortion,bool> (rc,false);
					}
					else
					{
						--SP;
					}
				}

				return ::std::numeric_limits<uint64_t>::max();
			}

			void smallerLarger(
				symbol_type const sym,
				uint64_t const left, uint64_t const right,
				uint64_t & smaller, uint64_t & larger
			) const
			{
				assert ( sym < (1ull << b) );

				NodePortion node = root(left, right);
				smaller = 0;
				larger = 0;

				for ( symbol_type mask = (1ull << (b-1)); mask && (node.right-node.left); mask >>= 1 )
					if ( sym & mask )
					{
						smaller += rank0(node,node.right-node.left-1);
						node = rightChild(node);
					}
					else
					{
						larger += rank1(node,node.right-node.left-1);
						node = leftChild(node);
					}
			}

			uint64_t smaller(
				symbol_type const sym, uint64_t const left, uint64_t const right
			) const
			{
				assert ( sym < (1ull << b) );

				NodePortion node = root(left, right);
				uint64_t smaller = 0;

				for ( symbol_type mask = (1ull << (b-1)); mask && (node.right-node.left); mask >>= 1 )
					if ( sym & mask )
					{
						smaller += rank0(node,node.right-node.left-1);
						node = rightChild(node);
					}
					else
					{
						node = leftChild(node);
					}

				return smaller;
			}

			uint64_t smaller(
				symbol_type const sym,
				uint64_t const left, uint64_t const right,
				unsigned int cntdepth
			) const
			{
				assert ( sym < (1ull << b) );

				NodePortion node = root(left, right);
				uint64_t smaller = 0;

				for ( symbol_type mask = (1ull << (b-1)); mask && (node.right-node.left); mask >>= 1 )
					if ( sym & mask )
					{
						if ( node.node.level >= cntdepth )
							smaller += rank0(node,node.right-node.left-1);
						node = rightChild(node);
					}
					else
					{
						node = leftChild(node);
					}

				return smaller;
			}

			/**
			 * return number of indices j <= i such that a[j] == k, i.e. the rank of index i concerning symbol k
			 *
			 * @param k
			 * @param i
			 * @return rank of index i concerning symbol k
			 **/
			uint64_t rank(uint64_t const k, uint64_t i) const
			{
				// check whether value k requires too many bits (i.e. is too large)
				if ( k & (~((1ull<<b) - 1)) )
					return 0;

				uint64_t left = 0, right = n, offset = 0, m = (1ull<<(b-1));

				for ( uint64_t ib = 0; ib < b; ++ib, m>>=1, offset += n )
				{
					// number of 0 bits up to node
					uint64_t const o0 = (offset+left) ? R.rank0(offset+left-1) : 0;
					// number of 1 bits up to node
					uint64_t const o1 = (offset+left) ? R.rank1(offset+left-1) : 0;

					if ( k & m )
					{
						// number of 1 bits in node up to position i
						uint64_t const n1 = R.rank1(offset+left+i) - o1;

						// no symbols
						if ( ! n1 )
							return 0;

						// total number of 0 bits in node
						uint64_t const n0 = R.rank0(offset+right-1) - o0;

						// adjust left, right, i
						left = left + n0;
						i = n1 - 1;
					}
					else
					{
						// number of 0 bits in node up to position i
						uint64_t const n0 = R.rank0(offset+left+i) - o0;

						// no symbols
						if ( ! n0 )
							return 0;

						// total number of 1 bits in node
						uint64_t const n1 = R.rank1(offset+right-1) - o1;

						// adjust left, right, i
						right = right - n1;
						i = n0 - 1;
					}
				}

				return i+1;
			}

			/**
			 *
			 **/
			std::pair<symbol_type,uint64_t> inverseSelect(uint64_t i) const
			{
				// node info
				uint64_t left = 0, right = n, offset = 0, m = (1ull<<(b-1));
				// symbol constructed
				symbol_type sym = 0;

				for ( uint64_t ib = 0; ib < b; ++ib, m>>=1, offset += n )
				{
					symbol_type const bit = ::libmaus2::bitio::getBit(W, offset+left+i);
					sym <<= 1;
					sym |= bit;

					// number of 0 bits up to node
					uint64_t const o0 = (offset+left) ? R.rank0(offset+left-1) : 0;
					// number of 1 bits up to node
					uint64_t const o1 = (offset+left) ? R.rank1(offset+left-1) : 0;

					if ( bit )
					{
						// number of 1 bits in node up to position i
						uint64_t const n1 = R.rank1(offset+left+i) - o1;

						// total number of 0 bits in node
						uint64_t const n0 = R.rank0(offset+right-1) - o0;

						// adjust left, right, i
						left = left + n0;
						i = n1 - 1;
					}
					else
					{
						// number of 0 bits in node up to position i
						uint64_t const n0 = R.rank0(offset+left+i) - o0;

						// total number of 1 bits in node
						uint64_t const n1 = R.rank1(offset+right-1) - o1;

						// adjust left, right, i
						right = right - n1;
						i = n0 - 1;
					}
				}

				return std::pair<symbol_type,uint64_t>(sym,i);
			}

			uint64_t rankm1(uint64_t const k, uint64_t const i) const
			{
				return i ? rank(k,i-1) : 0;
			}

			uint64_t rankm(uint64_t const k, uint64_t const i) const
			{
				return rankm1(k,i);
			}

			std::pair<uint64_t,uint64_t> rankm1(uint64_t const k, uint64_t const l, uint64_t const r) const
			{
				return std::pair<uint64_t,uint64_t>(rankm1(k,l),rankm1(k,r));
			}

			std::pair<uint64_t,uint64_t> rankm(uint64_t const k, uint64_t const l, uint64_t const r) const
			{
				return rankm1(k,l,r);
			}

			/**
			 * return number of indices j <= i such that a[j] == k, i.e. the rank of index i concerning symbol k
			 *
			 * @param k
			 * @param i
			 * @param rb depth
			 * @return rank of index i concerning symbol k
			 **/
			uint64_t rank(uint64_t const k, uint64_t i, unsigned int const rb) const
			{
				// check whether value k requires too many bits (i.e. is too large)
				if ( k & (~((1ull<<b) - 1)) )
					return 0;

				uint64_t left = 0, right = n, offset = 0, m = (1ull<<(b-1));

				for ( uint64_t ib = 0; ib < rb; ++ib, m>>=1, offset += n )
				{
					// number of 0 bits up to node
					uint64_t const o0 = (offset+left) ? R.rank0(offset+left-1) : 0;
					// number of 1 bits up to node
					uint64_t const o1 = (offset+left) ? R.rank1(offset+left-1) : 0;

					if ( k & m )
					{
						// number of 1 bits in node up to position i
						uint64_t const n1 = R.rank1(offset+left+i) - o1;

						// no symbols
						if ( ! n1 )
							return 0;

						// total number of 0 bits in node
						uint64_t const n0 = R.rank0(offset+right-1) - o0;

						// adjust left, right, i
						left = left + n0;
						i = n1 - 1;
					}
					else
					{
						// number of 0 bits in node up to position i
						uint64_t const n0 = R.rank0(offset+left+i) - o0;

						// no symbols
						if ( ! n0 )
							return 0;

						// total number of 1 bits in node
						uint64_t const n1 = R.rank1(offset+right-1) - o1;

						// adjust left, right, i
						right = right - n1;
						i = n0 - 1;
					}
				}

				return i+1;
			}

			uint64_t rangeQuantile(uint64_t l, uint64_t r, uint64_t i) const
			{
				NodePortion node = root(l,r);

				uint64_t k = 0;

				symbol_type mask = (1ull << (b-1));

				for ( uint64_t ib = 0; ib < b; ++ib, mask >>= 1 )
				{
					uint64_t zrank = (node.right-node.left) ? rank0( node, node.right - node.left - 1 ) : 0;

					#if 0
					::std::cerr
						<< "node.left=" << node.left << " node.right=" << node.right
						<< " node.node.left=" << node.node.left
						<< " node.node.right=" << node.node.right
						<< " zrank=" << zrank << " i=" << i << ::std::endl;
					#endif

					if ( i < zrank )
					{
						node = leftChild(node);
					}
					else
					{
						k |= mask;
						i -= zrank;
						node = rightChild(node);
					}
				}

				return k;
			}

			::std::vector< ::std::pair < symbol_type, uint64_t > > rangeCountMax(uint64_t const l, uint64_t const r, uint64_t const s) const
			{
				::std::vector< ::std::pair < symbol_type, uint64_t > > V;

				if ( r > l )
				{
					::std::priority_queue<RangeCountStackElement, ::std::vector<RangeCountStackElement>, ::std::less<RangeCountStackElement> > Q;

					Q.push( RangeCountStackElement(root(l,r)) );

					while ( Q.size() && V.size() < s )
					{
						RangeCountStackElement const E = Q.top();
						Q.pop();

						if ( E.depth() == b )
						{
							V.push_back ( ::std::pair<symbol_type,uint64_t>(E.sym,E.range()) );
						}
						else
						{
							RangeCountStackElement const L(leftChild(E), (E.sym<<1));
							if ( L.range() )
								Q.push(L);

							RangeCountStackElement const R(rightChild(E), (E.sym<<1)|1 );
							if ( R.range() )
								Q.push(R);
						}
					}
				}

				return V;
			}

			::libmaus2::autoarray::AutoArray< ::std::pair < symbol_type, uint64_t > > rangeCountMaxArray(uint64_t const l, uint64_t const r, uint64_t const s) const
			{
				::std::vector< ::std::pair < symbol_type, uint64_t > > V;

				if ( r > l )
				{
					::std::priority_queue<RangeCountStackElement, ::std::vector<RangeCountStackElement>, ::std::less<RangeCountStackElement> > Q;

					Q.push( RangeCountStackElement(root(l,r)) );

					while ( Q.size() && V.size() < s )
					{
						RangeCountStackElement const E = Q.top();
						Q.pop();

						if ( E.depth() == b )
						{
							V.push_back ( ::std::pair<symbol_type,uint64_t>(E.sym,E.range()) );
						}
						else
						{
							RangeCountStackElement const L(leftChild(E), (E.sym<<1));
							if ( L.range() )
								Q.push(L);

							RangeCountStackElement const R(rightChild(E), (E.sym<<1)|1 );
							if ( R.range() )
								Q.push(R);
						}
					}
				}

				::libmaus2::autoarray::AutoArray < ::std::pair < symbol_type, uint64_t > > A(V.size());
				::std::copy ( V.begin(), V.end(),  A.get() );

				return A;
			}

			::libmaus2::autoarray::AutoArray< ::std::pair < symbol_type, uint64_t > > rangeCount(uint64_t const l, uint64_t const r) const
			{
				uint64_t const stackdepth = b+1;
				uint64_t const stacksize = stackdepth * sizeof(RangeCountStackElement);

	#if defined(_MSC_VER) || defined(__MINGW32__)
				RangeCountStackElement * const S = reinterpret_cast<RangeCountStackElement *>(_alloca( stacksize ));
	#else
				RangeCountStackElement * const S = reinterpret_cast<RangeCountStackElement *>(alloca( stacksize ));
	#endif

				RangeCountStackElement * SP = S;


				uint64_t cnt = 0;

				*(SP++) = RangeCountStackElement(root(l,r));
				while ( SP != S )
				{
					RangeCountStackElement & E = *(--SP);

					/*
					::std::cerr << "NodePortion(" << E.node.level << "," <<
							E.node.left << "," <<
							E.node.right << "," <<
							E.left << "," <<
							E.right << "," <<
							E.node.level << ")" <<
							((E.node.level == b) ? "*" : "")
							<< " :: " << SP-S
							<< ::std::endl;
					*/

					if ( ! E.empty() )
					{
						if ( E.depth() == b )
						{
							++cnt;
						}
						else
						{
							switch ( E.visit )
							{
								case 0:
									E.visit++;
									*(++SP) = RangeCountStackElement ( leftChild(E), (E.sym<<1) );
									++SP;
									break;
								case 1:
									E.visit++;
									*(++SP) = RangeCountStackElement ( rightChild(E), (E.sym<<1)|1 );
									++SP;
									break;
								case 2:
									break;
							}
						}
					}
				}

				::libmaus2::autoarray::AutoArray< ::std::pair < symbol_type, uint64_t > > A(cnt,false);

				*(SP++) = RangeCountStackElement(root(l,r));
				cnt = 0;
				while ( SP != S )
				{
					RangeCountStackElement & E = *(--SP);

					if ( ! E.empty() )
					{
						if ( E.depth() == b )
						{
							A[cnt++] = ::std::pair<symbol_type,uint64_t>(E.sym,E.range());
						}
						else
						{
							switch ( E.visit )
							{
								case 0:
									E.visit++;
									*(++SP) = RangeCountStackElement ( leftChild(E), (E.sym<<1) );
									++SP;
									break;
								case 1:
									E.visit++;
									*(++SP) = RangeCountStackElement ( rightChild(E), (E.sym<<1)|1 );
									++SP;
									break;
								case 2:
									break;
							}
						}
					}
				}

				return A;
			}


			struct EnumerateRangeSymbolsStackElement : public NodePortion
			{
				unsigned int visit;
				symbol_type sym;

				EnumerateRangeSymbolsStackElement() : NodePortion(), visit(0), sym(0) {}
				EnumerateRangeSymbolsStackElement(NodePortion const & portion) : NodePortion(portion), visit(0), sym(0) {}
				EnumerateRangeSymbolsStackElement(NodePortion const & portion, symbol_type const rsym) : NodePortion(portion), visit(0), sym(rsym) {}

				unsigned int depth() const
				{
					return NodePortion::node.level;
				}
				uint64_t range() const
				{
					return NodePortion::right - NodePortion::left;
				}
				bool empty() const
				{
					return range() == 0;
				}
			};

			std::map < symbol_type, uint64_t > enumerateSymbolsInRangeSlow(uint64_t const l, uint64_t const r) const
			{
				std::map < symbol_type, uint64_t > M;
				for ( uint64_t i = l; i < r; ++i )
					M [ (*this)[i] ] ++;
				return M;
			}

			std::map < symbol_type, uint64_t > enumerateSymbolsInRange(uint64_t const l, uint64_t const r) const
			{
				uint64_t const stackdepth = b+1;
				uint64_t const stacksize = stackdepth * sizeof(EnumerateRangeSymbolsStackElement);

#if defined(_MSC_VER) || defined(__MINGW32__)
				EnumerateRangeSymbolsStackElement * const S = reinterpret_cast<EnumerateRangeSymbolsStackElement *>(_alloca( stacksize ));
#else
				EnumerateRangeSymbolsStackElement * const S = reinterpret_cast<EnumerateRangeSymbolsStackElement *>(alloca( stacksize ));
#endif

				EnumerateRangeSymbolsStackElement * SP = S;
				std::map < symbol_type, uint64_t > M;

				// uint64_t cnt = 0;

				*(SP++) = EnumerateRangeSymbolsStackElement(root(l,r));
				while ( SP != S )
				{
					EnumerateRangeSymbolsStackElement & E = *(--SP);

					/*
					::std::cerr << "NodePortion(" << E.node.level << "," <<
							E.node.left << "," <<
							E.node.right << "," <<
							E.left << "," <<
							E.right << "," <<
							E.node.level << ")" <<
							((E.node.level == b) ? "*" : "")
							<< " :: " << SP-S
							<< " offset=" << offset
							<< ::std::endl;
					*/

					if ( ! E.empty() )
					{
						if ( E.depth() == b )
						{
							 M[E.sym] = E.range();
						}
						else
						{
							switch ( E.visit )
							{
								case 0:
									// register next visit of E
									E.visit = 1; ++SP;
									if ( rank0(E) )
										*(SP++) = EnumerateRangeSymbolsStackElement ( leftChild(E), (E.sym<<1) );
									break;
								case 1:
									// register next visit of E
									E.visit = 2; ++SP;
									if ( rank1(E) )
										*(SP++) = EnumerateRangeSymbolsStackElement ( rightChild(E), (E.sym<<1)|1 );
									break;
								case 2:
									break;
							}
						}
					}
				}

				return M;

			}

			struct GenericRNVStackElement : public NodePortion
			{
				symbol_type sym;
				uint64_t offset;
				unsigned int visit;

				GenericRNVStackElement() : NodePortion(), sym(0), offset(0), visit(0) {}
				GenericRNVStackElement(NodePortion const portion) : NodePortion(portion), sym(0), offset(0), visit(0) {}
				GenericRNVStackElement(NodePortion const portion, symbol_type rsym) : NodePortion(portion), sym(rsym), offset(0), visit(0) {}
				GenericRNVStackElement(NodePortion const portion, symbol_type rsym, uint64_t const roffset) : NodePortion(portion), sym(rsym), offset(roffset), visit(0) {}

				unsigned int depth() const
				{
					return NodePortion::node.level;
				}

				uint64_t range() const
				{
					return NodePortion::right - NodePortion::left;
				}
				bool empty() const
				{
					return range() == 0;
				}
			};

			symbol_type rnvGeneric(uint64_t const l, uint64_t const r, symbol_type const k) const
			{
				uint64_t const stackdepth = b+1;
				uint64_t const stacksize = stackdepth * sizeof(GenericRNVStackElement);

	#if defined(_MSC_VER) || defined(__MINGW32__)
				GenericRNVStackElement * const S = reinterpret_cast<GenericRNVStackElement *>(_alloca( stacksize ));
	#else
				GenericRNVStackElement * const S = reinterpret_cast<GenericRNVStackElement *>(alloca( stacksize ));
	#endif

				GenericRNVStackElement * SP = S;

				// uint64_t cnt = 0;

				*(SP++) = GenericRNVStackElement(root(l,r));
				while ( SP != S )
				{
					GenericRNVStackElement & E = *(--SP);
					uint64_t const offset = 1ull<<(b-E.node.level-1);

					/*
					::std::cerr << "NodePortion(" << E.node.level << "," <<
							E.node.left << "," <<
							E.node.right << "," <<
							E.left << "," <<
							E.right << "," <<
							E.node.level << ")" <<
							((E.node.level == b) ? "*" : "")
							<< " :: " << SP-S
							<< " offset=" << offset
							<< ::std::endl;
					*/

					if ( ! E.empty() )
					{
						if ( E.depth() == b )
						{
							return E.sym;
						}
						else
						{
							switch ( E.visit )
							{
								case 0:
									// register next visit of E
									E.visit = 1; ++SP;
									if ( rank0(E) && k < E.offset+offset )
										*(SP++) = GenericRNVStackElement ( leftChild(E), (E.sym<<1), E.offset );
									break;
								case 1:
									// register next visit of E
									E.visit = 2; ++SP;
									if ( rank1(E) )
										*(SP++) = GenericRNVStackElement ( rightChild(E), (E.sym<<1)|1, E.offset + offset );
									break;
								case 2:
									break;
							}
						}
					}
				}

				return ::std::numeric_limits<symbol_type>::max();
			}

		};


		/**
		 * wavelet tree allowing rank/select on non-binary sequences in log(Sigma) time
		 **/
		template<typename _rank_type, typename _symbol_type>
		struct QuickWaveletTree
		{
			public:
			typedef _rank_type rank_type;
			typedef _symbol_type symbol_type;
			typedef QuickWaveletTree<rank_type,symbol_type> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			private:
			typedef typename rank_type::writer_type writer_type;
			typedef typename writer_type::data_type data_type;

			struct NodeInfo
			{
				private:
				uint64_t o0;
				uint64_t o1;
				uint64_t left;
				uint64_t right;
				uint64_t absleft;
				uint64_t absright;
				uint64_t b;

				public:
				uint64_t getO0() const { return o0; }
				uint64_t getO1() const { return o1; }
				uint64_t getLeft() const { return left; }
				uint64_t getRight() const { return right; }
				uint64_t getAbsLeft() const { return absleft; }
				uint64_t getAbsRight() const { return absright; }
				uint64_t getDiam() const { return right-left; }
				uint64_t getB() const { return b; }

				~NodeInfo()
				{
					#if 0
					std::cerr << "deconstructing NodeInfo(" << o0 << "," << o1 << "," << left << "," << right
					<< "," << absleft << "," << absright << "), " << this << std::endl;
					#endif
				}

				NodeInfo()
				: o0(0), o1(0), left(0), right(0), absleft(0), absright(0), b(0)
				{
					#if 0
					std::cerr << "constructed (0) NodeInfo(" << o0 << "," << o1 << "," << left << "," << right
					<< "," << absleft << "," << absright << "), " << this << std::endl;
					#endif
				}

				NodeInfo(
					uint64_t const ro0,
					uint64_t const ro1,
					uint64_t const rleft,
					uint64_t const rright,
					uint64_t const rabsleft,
					uint64_t const rabsright,
					uint64_t const rb)
				: o0(ro0), o1(ro1), left(rleft), right(rright), absleft(rabsleft), absright(rabsright), b(rb)
				{
					#if 0
					std::cerr << "constructed (1) NodeInfo(" << o0 << "," << o1 << "," << left << "," << right
					<< "," << absleft << "," << absright << "), " << this << std::endl;
					#endif
				}

				NodeInfo(NodeInfo const & o)
				: o0(o.o0), o1(o.o1), left(o.left), right(o.right), absleft(o.absleft), absright(o.absright), b(o.b)
				{
					#if 0
					std::cerr << "constructed (2) NodeInfo(" << o0 << "," << o1 << "," << left << "," << right
					<< "," << absleft << "," << absright << "), " << this << std::endl;
					#endif
				}

				NodeInfo & operator=(NodeInfo const & o)
				{
					o0 = o.o0;
					o1 = o.o1;
					left = o.left;
					right = o.right;
					absleft = o.absleft;
					absright = o.absright;
					b = o.b;

					#if 0
					std::cerr << "copied NodeInfo(" << o0 << "," << o1 << "," << left << "," << right
					<< "," << absleft << "," << absright << "), " << this << std::endl;
					#endif

					return *this;
				}
			};

			/**
			 * number of symbols in stream
			 **/
			uint64_t const n;
			/**
			 * bits per symbol
			 **/
			uint64_t const b;

			/**
			 * wavelet tree bit layers (n log S) bits
			 **/
			::libmaus2::autoarray::AutoArray< data_type > const AW;
			data_type const * const W;

			/**
			 * rank dictionary on W
			 **/
			rank_type R;
			/**
			 * offsets dictionary
			 **/
			::libmaus2::autoarray::AutoArray< data_type > const O0;
			::libmaus2::autoarray::AutoArray< data_type > const O1;
			::libmaus2::autoarray::AutoArray< NodeInfo > const NI;

			#if 0
			bool operator[](uint64_t const i) const
			{
				return ::libmaus2::bitio::getBit(W.get(),i);
			}
			#endif

			bool operator()(uint64_t const l, uint64_t const i) const
			{
				return ::libmaus2::bitio::getBit(AW.get(), l*n+i);
			}

			bool accessNodeBit(NodeInfo const & nodeinfo, uint64_t const i) const
			{
				return ::libmaus2::bitio::getBit(AW.get(), nodeinfo.getAbsLeft() + i);
			}

			/**
			 * align number a of bits to 64 by rounding up
			 *
			 * @param a
			 * @return ((a+63)/64)*64
			 *
			 **/
			static inline uint64_t align64(uint64_t const a)
			{
				return ((a+63)/64)*64;
			}
			/**
			 * return ceil(a/b)
			 *
			 * @param a
			 * @param b
			 * @return ceil(a/b)
			 **/
			static inline uint64_t divup(uint64_t const a, uint64_t const b)
			{
				return (a+(b-1))/b;
			}
			/**
			 * return number of bits required to store number k
			 *
			 * @param k
			 * @return number of bits required to store k
			 **/
			static inline uint64_t bitsPerNum(uint64_t k)
			{
				uint64_t c = 0;

				while ( k & (~0xFFFF) ) { k >>= 16; c += 16; }
				if    ( k & (~0x00FF) ) { k >>=  8; c +=  8; }
				if    ( k & (~0x000F) ) { k >>=  4; c +=  4; }
				if    ( k & (~0x0003) ) { k >>=  2; c +=  2; }
				if    ( k & (~0x0001) ) { k >>=  1; c +=  1; }
				if    ( k             ) { k >>=  1; c +=  1; }

				return c;
			}
			/**
			 * return maximal number of bits required to store a number in a
			 *
			 * @param a sequence of numbers
			 * @param n length of sequence a
			 * @return maximal number of bits required
			 **/
			template<typename it>
			static uint64_t getNumBits(it a, uint64_t const n)
			{
				return ::libmaus2::wavelet::GetNumBits<
					symbol_type,
					::std::numeric_limits<symbol_type>::is_signed > ::getNumBits(a,n);
			}

			/**
			 * produce wavelet tree bits with b layers from sequence a of length n.
			 * uses ~ 2n * sizeof(symbol_type) + (1<<b) * sizeof(uint64_t) bytes
			 * of memory in addition to output of size n*b bits
			 *
			 * @param a
			 * @param n
			 * @param b
			 * @return bits
			 **/
			template<typename it>
			static ::libmaus2::autoarray::AutoArray< data_type > produceBits(it a, uint64_t const n, uint64_t const b)
			{
				::libmaus2::autoarray::AutoArray< data_type > W( divup( align64(n*b), 8*sizeof(data_type) ), false );

				// ::std::cerr << "n=" << n << " b=" << b << " t=" << t << " w=" << w << ::std::endl;

				::libmaus2::autoarray::AutoArray<symbol_type> AA0(n,false), AA1(n,false);
				symbol_type * A0 = AA0.get(), * A1 = AA1.get();

				for ( uint64_t i = 0; i < n; ++i )
					A0[i] = *(a++);

				writer_type writer(W.get());

				::std::deque< uint64_t > T;
				T.push_back( n );

				uint64_t m = (1ull<<(b-1));

				for ( uint64_t ib = 0; (ib+1) < b; ++ib, m>>=1 )
				{
					::std::deque< uint64_t > Tn;

					uint64_t left = 0;

					while ( T.size() )
					{
						uint64_t const right = left + T.front(); T.pop_front();

						uint64_t pch = left;

						for ( uint64_t i = left; i < right; ++i )
							if ( A0[ i ] & m )
							{
								writer.writeBit(1);
							}
							else
							{
								writer.writeBit(0);
								pch++;
							}

						uint64_t pcl = left;

						if ( pch - left )
							Tn.push_back( pch-left );
						if ( right - pch )
							Tn.push_back( right-pch );

						for ( uint64_t i = left; i < right; ++i )
							if ( A0[ i ] & m )
								A1[pch++] = A0[i];
							else
								A1[pcl++] = A0[i];

						left = right;
					}

					T.swap(Tn);
					::std::swap(A0,A1);
				}

				uint64_t left = 0;

				while ( T.size() )
				{
					uint64_t const right = left + T.front(); T.pop_front();

					for ( uint64_t i = left; i < right; ++i )
						writer.writeBit( (A0[ i ] & m) != 0 );

					left = right;
				}

				if ( n * b )
					writer.flush();

				#if 0
				for ( uint64_t i = 0; i < n; ++i )
					::std::cerr << i % 10;
				::std::cerr << ::std::endl;
				for ( uint64_t ib = 0; ib < b; ++ib )
				{
					for ( uint64_t i = 0; i < n; ++i )
						::std::cerr << bitio::getBit(W.get(),ib*n+i);
					::std::cerr << ::std::endl;
				}
				#endif

				return W;
			}

			/**
			 * rank0 on node
			 **/
			uint64_t rank0(NodeInfo const & nodeinfo, uint64_t const i) const
			{
				return R.rank0(nodeinfo.getAbsLeft() + i)-nodeinfo.getO0();
			}
			/**
			 * rank1 on node
			 **/
			uint64_t rank1(NodeInfo const & nodeinfo, uint64_t const i) const
			{
				return R.rank1(nodeinfo.getAbsLeft() + i)-nodeinfo.getO1();
			}

			struct NodePortion
			{
				uint64_t q;
				uint64_t left;
				uint64_t right;

				NodePortion() : q(0), left(0), right(0) {}
				NodePortion(uint64_t const rq, uint64_t const rleft, uint64_t const rright)
				: q(rq), left(rleft), right(rright) {}
			};

			/**
			 * rank0 on node portion
			 **/
			uint64_t rank0(NodePortion const & node, uint64_t i) const
			{
				uint64_t const lb = NI[node.q].getAbsLeft() + node.left;

				// zeroes up to node portion
				uint64_t o0 =  lb ? R.rank0(lb - 1) : 0;
				// zeroes up to position i in node
				uint64_t n0 = R.rank0(lb + i);

				// ::std::cerr << "r0: lb=" << lb << " o0=" << o0 << " n0=" << n0 << " i=" << i << ::std::endl;

				// return difference
				return n0-o0;
			}

			/**
			 * number of 0 bits in node portion
			 **/
			uint64_t rank0(NodePortion const & node) const
			{
				if ( node.right-node.left )
				{
					return rank0(node, node.right-node.left-1);
				}
				else
				{
					return 0;
				}
			}

			/**
			 * rank1 on node portion
			 **/
			uint64_t rank1(NodePortion const & node, uint64_t i) const
			{
				uint64_t const lb = NI[node.q].getAbsLeft() + node.left;

				// zeroes up to node portion
				uint64_t o1 =  lb ? R.rank1(lb - 1) : 0;
				// zeroes up to position i in node
				uint64_t n1 = R.rank1(lb + i);

				// ::std::cerr << "r1: lb=" << lb << " o1=" << o1 << " n1=" << n1 << " i=" << i << ::std::endl;

				// return difference
				return n1-o1;
			}

			/**
			 * number of 1 bits in node portion
			 **/
			uint64_t rank1(NodePortion const & node) const
			{
				if ( node.right-node.left )
				{
					return rank1(node, node.right-node.left-1);
				}
				else
				{
					return 0;
				}
			}



			NodePortion root(uint64_t const l, uint64_t const r) const
			{
				return NodePortion(0,l,r);
			}

			/**
			 * left child:
			 * left =
			 *	v[left] == 0: rank0(v,left) - 1
			 *	v[left] != 0: rank0(v,left)
			 * right =
			 *	v[right-1] == 0: rank0 ( v, right-1 )
			 *	v[right-1] != 0: rank0 ( v, right-1 )
			 **/
			NodePortion leftChild(NodePortion const & node) const
			{
				NodePortion child;
				child.q = 2*node.q+1; // leftChild(node.node);

				NodeInfo const & childnodeinfo = NI[child.q];

				if (
					childnodeinfo.getRight()-childnodeinfo.getLeft()
					&&
					node.right - node.left
				)
				{
					NodeInfo const & nodeinfo = NI[node.q];

					child.left = (accessNodeBit(nodeinfo,node.left) == 0) ? (rank0(nodeinfo,node.left)-1) : rank0(nodeinfo,node.left);
					child.right = rank0(nodeinfo,node.right-1);

					if ( child.left == child.right )
						child.left = child.right = 0;
				}
				else
				{
					child.left = child.right = 0;
				}

				return child;
			}
			/**
			 * right child:
			 * left =
			 *	v[left] == 1: rank1(v,left) - 1
			 *	v[left] != 1: rank1(v,left)
			 * right =
			 *	v[right-1] == 1: rank1 ( v, right-1 )
			 *	v[right-1] != 1: rank1 ( v, right-1 )
			 **/
			NodePortion rightChild(NodePortion const & node) const
			{
				NodePortion child;
				child.q = 2*node.q+2;

				NodeInfo const & childnodeinfo = NI[child.q];

				if (
					childnodeinfo.getRight()-childnodeinfo.getLeft()
					&&
					node.right - node.left
				)
				{
					NodeInfo const & nodeinfo = NI[node.q];

					child.left = (accessNodeBit(nodeinfo,node.left) == 1) ? (rank1(nodeinfo,node.left)-1) : rank1(nodeinfo,node.left);
					child.right = rank1(nodeinfo,node.right-1);

					if ( child.left == child.right )
						child.left = child.right = 0;
				}
				else
				{
					child.left = child.right = 0;
				}

				return child;
			}

			// 1+2+4+8+... = 1(0) + 2(1) + ... + .(b-1)
			// b = 1 : 1 = (1ull<<b)-1
			// b = 2 : 3 = (1ull<<b)-1

			void fillO0(
				uint64_t * O0,
				uint64_t const left,
				uint64_t const right,
				uint64_t const i,
				uint64_t const ib
			)
			{
				if ( ib < b )
				{
					uint64_t const zbef = (ib*n+left)  ? R.rank0( ib*n + left -  1) : 0;
					uint64_t const zup  = (ib*n+right) ? R.rank0( ib*n + right -  1) : 0;
					uint64_t const zn = zup-zbef;

					// number of zeroes up to node
					assert ( i < (1ull<<b) );
					O0[i] = zbef;

					fillO0(O0, left, left + zn , 2*i + 1, ib + 1);
					fillO0(O0, left + zn, right, 2*i + 2, ib + 1);
				}
			}

			void fillO1(
				uint64_t * O1,
				uint64_t const left,
				uint64_t const right,
				uint64_t const i,
				uint64_t const ib
			)
			{
				if ( ib < b )
				{
					uint64_t const zbef = (ib*n+left)  ? R.rank0( ib*n + left -  1) : 0;
					uint64_t const zup  = (ib*n+right) ? R.rank0( ib*n + right -  1) : 0;
					uint64_t const zn = zup-zbef;

					uint64_t const obef = (ib*n+left)  ? R.rank1( ib*n + left -  1) : 0;

					// number of zeroes up to node
					assert ( i < (1ull<<b) );
					O1[i] = obef;

					fillO1(O1, left, left + zn , 2*i + 1, ib + 1);
					fillO1(O1, left + zn, right, 2*i + 2, ib + 1);
				}
			}

			void fillNodeInfo(
				NodeInfo * const NI,
				uint64_t const left,
				uint64_t const right,
				uint64_t const i,
				uint64_t const ib
			)
			{
				if ( ib < b )
				{
					// zeroes before node
					uint64_t const zbef = (ib*n+left)  ? R.rank0( ib*n + left -  1) : 0;
					// zeroes up to end of node
					uint64_t const zup  = (ib*n+right) ? R.rank0( ib*n + right -  1) : 0;

					// zeroes in node
					uint64_t const zn = zup-zbef;

					// number of ones before node
					uint64_t const obef = (ib*n+left)  ? R.rank1( ib*n + left -  1) : 0;

					#if 0
					// ones up to end of node
					uint64_t const oup  = (ib*n+right) ? R.rank1( ib*n + right -  1) : 0;
					// ones in node
					uint64_t const on = oup-obef;
					#endif

					// number of zeroes up to node
					assert ( i < (1ull<<b) );
					NI[i] = NodeInfo(
						zbef,obef,
						left,
						right,
						left + ib * n,
						right + ib * n,
						ib
					);

					fillNodeInfo(NI, left, left + zn , 2*i + 1, ib + 1);
					fillNodeInfo(NI, left + zn, right, 2*i + 2, ib + 1);
				}
				else if ( ib == b )
				{
					NI[i] = NodeInfo(0,0,left,right,left+ib*n,right+ib*n,ib);
				}
			}

			::libmaus2::autoarray::AutoArray<uint64_t> produceO0()
			{
				if ( n )
				{
					uint64_t numnodes1 = static_cast<uint64_t>(1) << b;
					::libmaus2::autoarray::AutoArray<uint64_t> AO0( numnodes1, false );

					fillO0(AO0.get(),0,n,0,0);

					if ( b )
						AO0[(1ull<<b)-1] = R.rank0(b*n-1);

					return AO0;
				}
				else
				{
					return ::libmaus2::autoarray::AutoArray<uint64_t>(0);
				}
			}
			::libmaus2::autoarray::AutoArray<uint64_t> produceO1()
			{
				if ( n )
				{
					uint64_t numnodes1 = static_cast<uint64_t>(1) << b;
					::libmaus2::autoarray::AutoArray<uint64_t> AO1( numnodes1, false );

					fillO1(AO1.get(),0,n,0,0);

					if ( b )
						AO1[(1ull<<b)-1] = R.rank1(b*n-1);

					return AO1;
				}
				else
				{
					return ::libmaus2::autoarray::AutoArray<uint64_t>(0);
				}
			}

			::libmaus2::autoarray::AutoArray<NodeInfo> produceNI()
			{
				if ( n )
				{
					uint64_t numnodes = (static_cast<uint64_t>(1ull) << (b+1))-1;
					::libmaus2::autoarray::AutoArray<NodeInfo> ANI( numnodes, true );

					fillNodeInfo(ANI.get(),0,n,0,0);

					return ANI;
				}
				else
				{
					return ::libmaus2::autoarray::AutoArray<NodeInfo>(0);
				}
			}

			public:
			/**
			 * construct wavelet tree from sequence a of length rn
			 *
			 * @param a
			 * @param rn
			 **/
			template<typename it>
			QuickWaveletTree(it a, uint64_t const rn)
			: n(rn), b(getNumBits(a,n)), AW( produceBits(a, n, b) ), W(AW.get()), R(W,align64(n*b)),
			  O0(produceO0()), O1(produceO1()), NI(produceNI())
			{
				#if 0
				std::cerr << "Constructing (0) QuickWaveletTree(it,uint64_t), " << this << std::endl;
				#endif
			}

			QuickWaveletTree(::libmaus2::autoarray::AutoArray<uint64_t> & rW,
				    uint64_t const rn,
				    uint64_t const rb)
			: n(rn), b(rb), AW( rW ), W( AW.get() ), R(W,align64(n*b)),
			  O0(produceO0()), O1(produceO1()), NI(produceNI())
			{
				#if 0
				std::cerr << "Constructing (1) QuickWaveletTree(AutoArray &, uint64_t, uint64_t), " << this << std::endl;
				#endif
			}

			QuickWaveletTree(uint64_t const * const rW,
				    uint64_t const rn,
				    uint64_t const rb)
			: n(rn), b(rb), AW( ), W( rW ), R(W,align64(n*b)),
			  O0(produceO0()), O1(produceO1()), NI(produceNI())
			{
				#if 0
				std::cerr << "Constructing (2) QuickWaveletTree(uint64_t const * const, uint64_t, uint64_t), " << this << std::endl;
				#endif
			}

			static uint64_t readUnsignedInt(::std::istream & in)
			{
				uint64_t i;
				::libmaus2::serialize::Serialize<uint64_t>::deserialize(in,&i);
				return i;
			}

			static uint64_t readUnsignedInt(::std::istream & in, uint64_t & s)
			{
				uint64_t i;
				s += ::libmaus2::serialize::Serialize<uint64_t>::deserialize(in,&i);
				return i;
			}

			static ::libmaus2::autoarray::AutoArray<uint64_t> readArray(::std::istream & in)
			{
				::libmaus2::autoarray::AutoArray<uint64_t> A;
				A.deserialize(in);
				return A;
			}

			static ::libmaus2::autoarray::AutoArray<uint64_t> readArray(::std::istream & in, uint64_t & s)
			{
				::libmaus2::autoarray::AutoArray<uint64_t> A;
				s += A.deserialize(in);
				return A;
			}

			QuickWaveletTree(::std::istream & in)
			: n(readUnsignedInt(in)), b(readUnsignedInt(in)), AW ( readArray(in) ), W(AW.get()), R(W,align64(n*b))
			, O0(produceO0()), O1(produceO1()), NI(produceNI())
			{
				#if 0
				std::cerr << "Constructing (3) QuickWaveletTree(std::istream &), " << this << std::endl;
				#endif
			}

			QuickWaveletTree(::std::istream & in, uint64_t & s)
			: n(readUnsignedInt(in,s)), b(readUnsignedInt(in,s)), AW ( readArray(in,s) ), W(AW.get()), R(AW.get(),align64(n*b))
			, O0(produceO0()), O1(produceO1()), NI(produceNI())
			{
				#if 0
				std::cerr << "Constructing (4) QuickWaveletTree(std::istream &, uint64_t &), " << this << std::endl;
				#endif
			}

			~QuickWaveletTree()
			{
				#if 0
				std::cerr << "Deconstructing QuickWaveletTree, " << this << std::endl;
				#endif
			}

			uint64_t serialize(::std::ostream & out)
			{
				uint64_t s = 0;
				s += ::libmaus2::serialize::Serialize<uint64_t>::serialize(out,n);
				s += ::libmaus2::serialize::Serialize<uint64_t>::serialize(out,b);
				s += AW.serialize(out);
				return s;
			}

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

			::libmaus2::autoarray::AutoArray<int64_t> getSymbolArray() const
			{
				::libmaus2::autoarray::AutoArray<int64_t> A(1ull << getB());
				for ( uint64_t i = 0; i < A.size(); ++i )
					A[i] = i;
				return A;
			}

			/**
			 * return the index of the i'th occurence of k, or ::std::numeric_limits<uint64_t>::max(), if no such index exists
			 *
			 * @param k
			 * @param i
			 * @return index of the i'th occurence of k
			 **/
			uint64_t select(uint64_t const k, uint64_t i) const
			{
				// check whether value k requires too many bits (i.e. is too large)
				if ( k & (~((1ull<<b) - 1)) )
					return ::std::numeric_limits<uint64_t>::max();

				uint64_t m = 1;
				uint64_t q = ((((1ull<<b)-1)+k)-1) >> 1;

				uint64_t const range = (k & 1) ?
					((((O0[q+1]+O1[q+1]))) - (((O0[q]+O1[q]) + (O0[q+1] - O0[q])))) :
					(((O0[q+1]+O1[q+1]) - (O1[q+1] - O1[q])) - ((O0[q]+O1[q])));

				if ( i >= range )
					return ::std::numeric_limits<uint64_t>::max();

				for ( uint64_t ib = 0; ib < b; ++ib, m <<= 1, q = (q-1)>>1 )
				{
					uint64_t const left = (O0[q]+O1[q]);

					if ( k & m )
						i = R.select1( left - O0[q] + i ) - left;
					else
						i = R.select0( O0[q] + i ) - left;
				}

				return i;
			}

			/**
			 * print tree on stream for debugging
			 * @param out stream
			 **/
			void print(::std::ostream & out) const
			{
				::std::stack< uint64_t > S;
				S.push(0);

				while ( ! S.empty() )
				{
					uint64_t const q = S.top(); S.pop();

					if ( NI[q].getB() < b )
					{
						out << "Node(" << NI[q].getB() << "," <<
							NI[q].getLeft() << "," <<
							NI[q].getRight() << ")" << ::std::endl;

						S.push(2*q+2);
						S.push(2*q+1);
					}
				}
			}


			std::pair<symbol_type,uint64_t> inverseSelect(uint64_t i) const
			{
				if ( i >= n )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "QuickWaveletTree<>::inverseSelect() called for i=" << i << " >= n=" << n;
					se.finish();
					throw se;
				}

				uint64_t q = 0;
				symbol_type s = 0;

				for ( uint64_t ib = 0; ib < b; ++ib )
				{
					NodeInfo const & I = NI[q];
					uint64_t const abspos = I.getAbsLeft() + i;
					bool const bit = ::libmaus2::bitio::getBit(AW.get(), abspos);

					if ( bit )
					{
						i = R.rank1(abspos)-I.getO1()-1;
						s = (s<<1)|1;
						q = 2*q+2;
					}
					else
					{
						i = R.rank0(abspos)-I.getO0()-1;
						s = (s<<1);
						q = 2*q+1;
					}
				}

				return std::pair<symbol_type,uint64_t>(s,i);
			}

			/**
			 * return the i'th symbol in the input sequence
			 *
			 * @param i index
			 * @return i'th symbol
			 **/
			symbol_type operator[](uint64_t const i) const
			{
				return access(i);
				// return rmq(i,i+1);
			}

			/**
			 * range minimum value query
			 **/
			symbol_type rmq(uint64_t const l, uint64_t const r) const
			{
				assert ( r > l );
				assert ( r <= n );

				NodePortion P = root(l,r);

				symbol_type v = 0;
				for ( uint64_t ib = 0; ib < b; ++ib )
				{
					v <<= 1;

					if ( rank0(P,P.right-P.left-1) )
					{
						P = leftChild(P);
					}
					else
					{
						P = rightChild(P);
						v |= 1;
					}
				}

				return v;
			}

			/**
			 * range maximum value query
			 **/
			symbol_type rmqi(uint64_t const l, uint64_t const r) const
			{
				assert ( r > l );
				assert ( r <= n );

				NodePortion P = root(l,r);

				symbol_type v = 0;
				for ( uint64_t ib = 0; ib < b; ++ib )
				{
					v <<= 1;

					if ( rank1(P,P.right-P.left-1) )
					{
						P = rightChild(P);
						v |= 1;
					}
					else
					{
						P = leftChild(P);
					}
				}

				return v;
			}

			/**
			 * print portion of tree
			 *
			 * @param out stream
			 * @param l left
			 * @param r right
			 **/
			void print(::std::ostream & out, uint64_t const l, uint64_t const r) const
			{
				::std::stack<NodePortion> S;
				S.push(root(l,r));

				while ( ! S.empty() )
				{
					NodePortion const node = S.top(); S.pop();

					if ( NI[node.q].getB() <= b )
					{
						out << "NodePortion(" << NI[node.q].getB() << "," <<
							NI[node.q].getLeft() << "," <<
							NI[node.q].getRight() << "," <<
							node.left << "," <<
							node.right << ")" << ::std::endl;

						if ( NI[node.q].getB()+1 <= b )
						{
							NodePortion const rc = rightChild(node);
							S.push(rc);
						}
						if ( NI[node.q].getB()+1 <= b )
						{
							NodePortion const lc = leftChild(node);
							S.push(lc);
						}
					}
				}
			}

			/**
			 * print portion of tree
			 *
			 * @param out stream
			 * @param l left
			 * @param r right
			 * @param k key
			 **/
			void print(::std::ostream & out, uint64_t const l, uint64_t const r, uint64_t const k) const
			{
				::std::stack<NodePortion> S;
				S.push(root(l,r));

				while ( ! S.empty() )
				{
					NodePortion const node = S.top(); S.pop();
					NodeInfo const & nodeinfo = NI[node.q];

					if ( (nodeinfo.getDiam()) && (nodeinfo.getRight()) > k )
					{
						out << "NodePortion(" << nodeinfo.getB() << "," <<
							nodeinfo.getLeft() << "," <<
							nodeinfo.getRight() << "," <<
							node.left << "," <<
							node.right << ")" <<
							((nodeinfo.getB() == b) ? "*" : "") << ::std::endl;

						if ( nodeinfo.getB() + 1 <= b )
						{
							NodePortion const rc = rightChild(node);
							S.push(rc);
						}
						if ( nodeinfo.getB() + 1 <= b )
						{
							NodePortion const lc = leftChild(node);
							S.push(lc);
						}
					}
				}
			}

			struct MultiRankStackElement
			{
				uint64_t m;
				uint64_t q;
				uint64_t v;
				uint64_t l;
				uint64_t r;
				symbol_type s;

				MultiRankStackElement() : m(0), q(0), v(0), l(0), r(0), s(0) {}
				MultiRankStackElement(uint64_t const rm, uint64_t const rq, uint64_t const rl, uint64_t const rr, symbol_type const rs)
				: m(rm), q(rq), v(0), l(rl), r(rr), s(rs)
				{}

				::std::string toString() const
				{
					std::ostringstream ostr;
					ostr << "MultiRankStackElement("
						<< "m=" << m << ","
						<< "q=" << q << ","
						<< "v=" << v << ","
						<< "l=" << l << ","
						<< "r=" << r << ","
						<< "s=" << s << ")";
					return ostr.str();
				}
			};

			void multiRank(uint64_t const l, uint64_t const r, uint64_t const * D) const
			{
#if defined(_MSC_VER) || defined(__MINGW32__)
				MultiRankStackElement * const ST = reinterpret_cast< MultiRankStackElement *>(_alloca( (b+1)*sizeof(MultiRankStackElement) ));
#else
				MultiRankStackElement * const ST = reinterpret_cast< MultiRankStackElement *>(alloca( (b+1)*sizeof(MultiRankStackElement) ));
#endif

				// stack pointer
				MultiRankStackElement * SP = ST;

				if ( l != r )
				{
					// push root
					*(SP++) = MultiRankStackElement((1ull)<<(b-1),0,l,r,0);
				}

				while ( SP != ST )
				{
					MultiRankStackElement & M = *(--SP);

					// std::cerr << M.toString() << std::endl;

					M.v += 1;

					if ( M.m )
					{
						if ( M.v < 3 )
							++SP;

						NodeInfo const & I = NI[M.q];

						switch ( M.v )
						{
							// consider left
							case 1:
							{
								uint64_t const subl = M.l ? (R.rank0(I.getAbsLeft()+M.l-1) - I.getO0()) : 0;
								uint64_t const subr = M.r ? (R.rank0(I.getAbsLeft()+M.r-1) - I.getO0()) : 0;
								if ( subr-subl )
									*(SP++) = MultiRankStackElement(
										M.m>>1,
										2*M.q+1,
										subl,subr,
										(M.s<<1)
									);
								break;
							}
							// consider right
							case 2:
							{
								uint64_t const subl = M.l ? (R.rank1(I.getAbsLeft()+M.l-1) - I.getO1()) : 0;
								uint64_t const subr = M.r ? (R.rank1(I.getAbsLeft()+M.r-1) - I.getO1()) : 0;
								if ( subr-subl )
									*(SP++) = MultiRankStackElement(M.m>>1,2*M.q+2,subl,subr,(M.s<<1)|1);
								break;
							}
						}
					}
					else if ( M.r != M.l )
					{
						std::cerr << "sym " << M.s << " l " << D[M.s] + M.l << " r " << D[M.s] + M.r << std::endl;
					}
				}
			}

			template<typename lcp_iterator, typename queue_type>
			inline void multiRankLCPSet(
				uint64_t const l,
				uint64_t const r,
				uint64_t const * D,
				lcp_iterator WLCP,
				typename std::iterator_traits<lcp_iterator>::value_type const unset,
				typename std::iterator_traits<lcp_iterator>::value_type const cur_l,
				queue_type * PQ1
				) const
			{
	#if defined(_MSC_VER) || defined(__MINGW32__)
				MultiRankStackElement * const ST = reinterpret_cast< MultiRankStackElement *>(_alloca( (b+1)*sizeof(MultiRankStackElement) ));
	#else
				MultiRankStackElement * const ST = reinterpret_cast< MultiRankStackElement *>(alloca( (b+1)*sizeof(MultiRankStackElement) ));
	#endif

				// stack pointer
				MultiRankStackElement * SP = ST;

				if ( l != r )
				{
					// push root
					*(SP++) = MultiRankStackElement((1ull)<<(b-1),0,l,r,0);
				}

				while ( SP != ST )
				{
					MultiRankStackElement & M = *(--SP);

					// std::cerr << M.toString() << std::endl;

					M.v += 1;

					if ( M.m )
					{
						if ( M.v < 3 )
							++SP;

						NodeInfo const & I = NI[M.q];

						switch ( M.v )
						{
							// consider left
							case 1:
							{
								uint64_t const subl = M.l ? (R.rank0(I.getAbsLeft()+M.l-1) - I.getO0()) : 0;
								uint64_t const subr = M.r ? (R.rank0(I.getAbsLeft()+M.r-1) - I.getO0()) : 0;
								if ( subr-subl )
									*(SP++) = MultiRankStackElement(
										M.m>>1,
										2*M.q+1,
										subl,subr,
										(M.s<<1)
									);
								break;
							}
							// consider right
							case 2:
							{
								uint64_t const subl = M.l ? (R.rank1(I.getAbsLeft()+M.l-1) - I.getO1()) : 0;
								uint64_t const subr = M.r ? (R.rank1(I.getAbsLeft()+M.r-1) - I.getO1()) : 0;
								if ( subr-subl )
									*(SP++) = MultiRankStackElement(M.m>>1,2*M.q+2,subl,subr,(M.s<<1)|1);
								break;
							}
						}
					}
					else if ( M.r != M.l )
					{
						// std::cerr << "sym " << M.s << " l " << D[M.s] + M.l << " r " << D[M.s] + M.r << std::endl;

						uint64_t const sp = D[M.s] + M.l;
						uint64_t const ep = D[M.s] + M.r;

						if ( WLCP[ep] == unset )
	                                        {
        	                                        WLCP[ep] = cur_l;
                	                                PQ1->push_back( std::pair<uint64_t,uint64_t>(sp,ep) );
                        	                }
					}
				}
			}

			template<typename lcp_iterator, typename queue_type>
			inline void multiRankLCPSetLarge(
				uint64_t const l,
				uint64_t const r,
				uint64_t const * D,
				lcp_iterator & WLCP,
				uint64_t const cur_l,
				queue_type * PQ1
				) const
			{
	#if defined(_MSC_VER) || defined(__MINGW32__)
				MultiRankStackElement * const ST = reinterpret_cast< MultiRankStackElement *>(_alloca( (b+1)*sizeof(MultiRankStackElement) ));
	#else
				MultiRankStackElement * const ST = reinterpret_cast< MultiRankStackElement *>(alloca( (b+1)*sizeof(MultiRankStackElement) ));
	#endif

				// stack pointer
				MultiRankStackElement * SP = ST;

				if ( l != r )
				{
					// push root
					*(SP++) = MultiRankStackElement((1ull)<<(b-1),0,l,r,0);
				}

				while ( SP != ST )
				{
					MultiRankStackElement & M = *(--SP);

					// std::cerr << M.toString() << std::endl;

					M.v += 1;

					if ( M.m )
					{
						if ( M.v < 3 )
							++SP;

						NodeInfo const & I = NI[M.q];

						switch ( M.v )
						{
							// consider left
							case 1:
							{
								uint64_t const subl = M.l ? (R.rank0(I.getAbsLeft()+M.l-1) - I.getO0()) : 0;
								uint64_t const subr = M.r ? (R.rank0(I.getAbsLeft()+M.r-1) - I.getO0()) : 0;
								if ( subr-subl )
									*(SP++) = MultiRankStackElement(
										M.m>>1,
										2*M.q+1,
										subl,subr,
										(M.s<<1)
									);
								break;
							}
							// consider right
							case 2:
							{
								uint64_t const subl = M.l ? (R.rank1(I.getAbsLeft()+M.l-1) - I.getO1()) : 0;
								uint64_t const subr = M.r ? (R.rank1(I.getAbsLeft()+M.r-1) - I.getO1()) : 0;
								if ( subr-subl )
									*(SP++) = MultiRankStackElement(M.m>>1,2*M.q+2,subl,subr,(M.s<<1)|1);
								break;
							}
						}
					}
					else if ( M.r != M.l )
					{
						// std::cerr << "sym " << M.s << " l " << D[M.s] + M.l << " r " << D[M.s] + M.r << std::endl;

						uint64_t const sp = D[M.s] + M.l;
						uint64_t const ep = D[M.s] + M.r;

						if ( WLCP.isUnset(ep) )
	                                        {
        	                                        WLCP.set(ep, cur_l);
                	                                PQ1->push_back( std::pair<uint64_t,uint64_t>(sp,ep) );
                        	                }
					}
				}
			}

			template<typename callback>
			inline void multiRankCallBack(
				uint64_t const l,
				uint64_t const r,
				uint64_t const * D,
				callback & cb
				) const
			{
	#if defined(_MSC_VER) || defined(__MINGW32__)
				MultiRankStackElement * const ST = reinterpret_cast< MultiRankStackElement *>(_alloca( (b+1)*sizeof(MultiRankStackElement) ));
	#else
				MultiRankStackElement * const ST = reinterpret_cast< MultiRankStackElement *>(alloca( (b+1)*sizeof(MultiRankStackElement) ));
	#endif

				// stack pointer
				MultiRankStackElement * SP = ST;

				if ( l != r )
				{
					// push root
					*(SP++) = MultiRankStackElement((1ull)<<(b-1),0,l,r,0);
				}

				while ( SP != ST )
				{
					MultiRankStackElement & M = *(--SP);

					// std::cerr << M.toString() << std::endl;

					M.v += 1;

					if ( M.m )
					{
						if ( M.v < 3 )
							++SP;

						NodeInfo const & I = NI[M.q];

						switch ( M.v )
						{
							// consider left
							case 1:
							{
								uint64_t const subl = M.l ? (R.rank0(I.getAbsLeft()+M.l-1) - I.getO0()) : 0;
								uint64_t const subr = M.r ? (R.rank0(I.getAbsLeft()+M.r-1) - I.getO0()) : 0;
								if ( subr-subl )
									*(SP++) = MultiRankStackElement(
										M.m>>1,
										2*M.q+1,
										subl,subr,
										(M.s<<1)
									);
								break;
							}
							// consider right
							case 2:
							{
								uint64_t const subl = M.l ? (R.rank1(I.getAbsLeft()+M.l-1) - I.getO1()) : 0;
								uint64_t const subr = M.r ? (R.rank1(I.getAbsLeft()+M.r-1) - I.getO1()) : 0;
								if ( subr-subl )
									*(SP++) = MultiRankStackElement(M.m>>1,2*M.q+2,subl,subr,(M.s<<1)|1);
								break;
							}
						}
					}
					else if ( M.r != M.l )
					{
						// std::cerr << "sym " << M.s << " l " << D[M.s] + M.l << " r " << D[M.s] + M.r << std::endl;
						uint64_t const sp = D[M.s] + M.l;
						uint64_t const ep = D[M.s] + M.r;
						cb ( M.s, sp, ep );
					}
				}
			}

			symbol_type access(uint64_t i) const
			{
				assert ( i < n );
				uint64_t q = 0;
				symbol_type s = 0;

				for ( uint64_t ib = 0; ib < b; ++ib )
				{
					NodeInfo const & I = NI[q];
					uint64_t const abspos = I.getAbsLeft() + i;
					bool const bit = ::libmaus2::bitio::getBit(AW.get(), abspos);

					if ( bit )
					{
						i = R.rank1(abspos)-I.getO1()-1;
						s = (s<<1)|1;
						q = 2*q+2;
					}
					else
					{
						i = R.rank0(abspos)-I.getO0()-1;
						s = (s<<1);
						q = 2*q+1;
					}
				}

				return s;
			}


			/**
			 * return number of indices j <= i such that a[j] == k, i.e. the rank of index i concerning symbol k
			 *
			 * @param k < 2^b
			 * @param i
			 * @return rank of index i concerning symbol k
			 **/
			uint64_t rank(uint64_t const k, uint64_t i) const
			{
				uint64_t m = (1ull<<(b-1));
				uint64_t q = 0;

				for ( uint64_t ib = 0; ib < b; ++ib, m>>=1 )
				{
					NodeInfo const & I = NI[q];
					uint64_t const abspos = I.getAbsLeft() + i;

					/*
					uint64_t const subl = M.l ? (R.rank0(I.getAbsLeft()+M.l-1) - I.getO0()) : 0;
					uint64_t const subr = M.r ? (R.rank0(I.getAbsLeft()+M.r-1) - I.getO0()) : 0;
					*/

					if ( k & m )
					{
						// number of 1 bits in node up to position i
						uint64_t const n1 = R.rank1(abspos) - I.getO1();

						// no symbols
						if ( ! n1 )
							return 0;

						i = n1 - 1;

						q = 2*q+2;
					}
					else
					{
						// number of 0 bits in node up to position i
						uint64_t const n0 = R.rank0(abspos) - I.getO0();

						// no symbols
						if ( ! n0 )
							return 0;

						i = n0 - 1;

						q = 2*q+1;
					}
				}

				return i+1;
			}

			uint64_t rankm1(uint64_t const k, uint64_t const i) const
			{
				return i ? rank(k,i-1) : 0;
			}

			/**
			 * return number of indices j < i such that a[j] == k for i=l and i=r
			 *
			 * @param k < 2^b
			 * @param l
			 * @param r
			 * @return rank of index i concerning symbol k
			 **/
			std::pair<uint64_t,uint64_t> rankm1(uint64_t const k, uint64_t l, uint64_t r) const
			{
				uint64_t m = (1ull<<(b-1));
				uint64_t q = 0;

				for ( uint64_t ib = 0; ib < b; ++ib, m>>=1 )
				{
					NodeInfo const & I = NI[q];

					if ( k & m )
					{
						l = l ? (R.rank1(I.getAbsLeft()+l-1) - I.getO1()) : 0;
						r = r ? (R.rank1(I.getAbsLeft()+r-1) - I.getO1()) : 0;
						q = 2*q+2;
					}
					else
					{
						l = l ? (R.rank0(I.getAbsLeft()+l-1) - I.getO0()) : 0;
						r = r ? (R.rank0(I.getAbsLeft()+r-1) - I.getO0()) : 0;
						q = 2*q+1;
					}
				}

				return std::pair<uint64_t,uint64_t>(l,r);
			}

			/**
			 * range next value, return smallest value >= k in A[l,r-1], or ::std::numeric_limits<uint64_t>::max()
			 * if no such value exists
			 *
			 * @param l
			 * @param r
			 * @param k
			 * @return rpv(l,r,k)
			 **/
			uint64_t rnv(uint64_t const l, uint64_t const r, uint64_t const k) const
			{
				// maximum depth of stack is b+1

	#if defined(_MSC_VER) || defined(__MINGW32__)
				::std::pair < NodePortion, bool > * const ST = reinterpret_cast< ::std::pair < NodePortion, bool > *>(_alloca( (b+1)*sizeof(::std::pair < NodePortion, bool >) ));
	#else
				::std::pair < NodePortion, bool > * const ST =
					reinterpret_cast< ::std::pair < NodePortion, bool > *>(alloca( (b+1)*sizeof( ::std::pair < NodePortion, bool >) ));
	#endif

				// stack pointer
				::std::pair < NodePortion, bool > * SP = ST;
				// push root
				*(SP++) = ::std::pair < NodePortion, bool > (root(l,r), false);

				while ( SP != ST )
				{
					std::pair < NodePortion, bool > & P = *(SP-1);

					NodePortion const & node = P.first;
					NodeInfo const & nodeinfo = NI[node.q];
					bool const seenbefore = P.second;

					if ( seenbefore )
					{
						NodePortion const rc = rightChild(node);

						--SP;

						if ( rc.right - rc.left )
							*(SP++) = ::std::pair<NodePortion,bool> (rc,false);
					}
					else if ( (nodeinfo.getDiam()) && (nodeinfo.getRight()) > k )
					{
						if ( nodeinfo.getB() == b )
							return nodeinfo.getLeft();

						NodePortion const lc = leftChild(node);

						P.second = true;

						if ( lc.right - lc.left )
							*(SP++) = ::std::pair<NodePortion,bool> (lc,false);
					}
					else
					{
						--SP;
					}
				}

				return ::std::numeric_limits<uint64_t>::max();
			}

			/**
			 * range previous value, return largest value <= k in A[l,r-1], or ::std::numeric_limits<uint64_t>::max()
			 * if no such value exists
			 *
			 * @param l
			 * @param r
			 * @param k
			 * @return rpv(l,r,k)
			 **/
			uint64_t rpv(uint64_t const l, uint64_t const r, uint64_t const k) const
			{
				// maximum depth of stack is b+1

	#if defined(_MSC_VER) || defined(__MINGW32__)
				::std::pair < NodePortion, bool > * const ST = reinterpret_cast< ::std::pair < NodePortion, bool > *>(_alloca( (b+1)*sizeof(::std::pair < NodePortion, bool >) ));
	#else
				::std::pair < NodePortion, bool > * const ST = reinterpret_cast< ::std::pair < NodePortion, bool > *>(alloca( (b+1)*sizeof(::std::pair < NodePortion, bool >) ));
	#endif

				// stack pointer
				::std::pair < NodePortion, bool > * SP = ST;
				// push root
				*(SP++) = ::std::pair < NodePortion, bool > (root(l,r), false);

				while ( SP != ST )
				{
					::std::pair < NodePortion, bool > & P = *(SP-1);

					NodePortion const & node = P.first;
					bool const seenbefore = P.second;

					if ( seenbefore )
					{
						NodePortion const lc = leftChild(node);

						--SP;

						if ( lc.right - lc.left )
							*(SP++) = ::std::pair<NodePortion,bool> (lc,false);
					}
					else if ( (NI[node.q].getDiam()) && (NI[node.q].getLeft()) <= k )
					{
						if ( NI[node.q].getB() == b )
							return NI[node.q].getLeft();

						NodePortion const rc = rightChild(node);

						P.second = true;

						if ( rc.right - rc.left )
							*(SP++) = ::std::pair<NodePortion,bool> (rc,false);
					}
					else
					{
						--SP;
					}
				}

				return ::std::numeric_limits<uint64_t>::max();
			}

			void smallerLarger(
				symbol_type const sym,
				uint64_t const left, uint64_t const right,
				uint64_t & smaller, uint64_t & larger
			) const
			{
				assert ( sym < (1ull << b) );

				NodePortion node = root(left, right);
				smaller = 0;
				larger = 0;

				for ( symbol_type mask = (1ull << (b-1)); mask && (node.right-node.left); mask >>= 1 )
					if ( sym & mask )
					{
						smaller += rank0(node,node.right-node.left-1);
						node = rightChild(node);
					}
					else
					{
						larger += rank1(node,node.right-node.left-1);
						node = leftChild(node);
					}
			}

			uint64_t smaller(
				symbol_type const sym, uint64_t const left, uint64_t const right
			) const
			{
				assert ( sym < (1ull << b) );

				NodePortion node = root(left, right);
				uint64_t smaller = 0;

				for ( symbol_type mask = (1ull << (b-1)); mask && (node.right-node.left); mask >>= 1 )
					if ( sym & mask )
					{
						smaller += rank0(node,node.right-node.left-1);
						node = rightChild(node);
					}
					else
					{
						node = leftChild(node);
					}

				return smaller;
			}

			uint64_t smaller(
				symbol_type const sym,
				uint64_t const left, uint64_t const right,
				unsigned int cntdepth
			) const
			{
				assert ( sym < (1ull << b) );

				NodePortion node = root(left, right);
				uint64_t smaller = 0;

				for ( symbol_type mask = (1ull << (b-1)); mask && (node.right-node.left); mask >>= 1 )
					if ( sym & mask )
					{
						if ( node.node.level >= cntdepth )
							smaller += rank0(node,node.right-node.left-1);
						node = rightChild(node);
					}
					else
					{
						node = leftChild(node);
					}

				return smaller;
			}

			struct GenericRNVStackElement : public NodePortion
			{
				symbol_type sym;
				uint64_t offset;
				unsigned int visit;

				GenericRNVStackElement() : NodePortion(), sym(0), offset(0), visit(0) {}
				GenericRNVStackElement(NodePortion const portion) : NodePortion(portion), sym(0), offset(0), visit(0) {}
				GenericRNVStackElement(NodePortion const portion, symbol_type rsym) : NodePortion(portion), sym(rsym), offset(0), visit(0) {}
				GenericRNVStackElement(NodePortion const portion, symbol_type rsym, uint64_t const roffset) : NodePortion(portion), sym(rsym), offset(roffset), visit(0) {}

				uint64_t range() const
				{
					return NodePortion::right - NodePortion::left;
				}
				bool empty() const
				{
					return range() == 0;
				}
			};

			symbol_type rnvGeneric(uint64_t const l, uint64_t const r, symbol_type const k) const
			{
				uint64_t const stackdepth = b+1;
				uint64_t const stacksize = stackdepth * sizeof(GenericRNVStackElement);

	#if defined(_MSC_VER) || defined(__MINGW32__)
				GenericRNVStackElement * const S = reinterpret_cast<GenericRNVStackElement *>(_alloca( stacksize ));
	#else
				GenericRNVStackElement * const S = reinterpret_cast<GenericRNVStackElement *>(alloca( stacksize ));
	#endif

				GenericRNVStackElement * SP = S;

				// uint64_t cnt = 0;

				*(SP++) = GenericRNVStackElement(root(l,r));
				while ( SP != S )
				{
					GenericRNVStackElement & E = *(--SP);
					uint64_t const offset = 1ull<<(b-NI[E.q].getB()-1);

					/*
					::std::cerr << "NodePortion(" << E.node.level << "," <<
							E.node.left << "," <<
							E.node.right << "," <<
							E.left << "," <<
							E.right << "," <<
							E.node.level << ")" <<
							((E.node.level == b) ? "*" : "")
							<< " :: " << SP-S
							<< " offset=" << offset
							<< ::std::endl;
					*/

					if ( ! E.empty() )
					{
						if ( NI[E.q].getB() == b )
						{
							return E.sym;
						}
						else
						{
							switch ( E.visit )
							{
								case 0:
									// register next visit of E
									E.visit = 1; ++SP;
									if ( rank0(E) && k < E.offset+offset )
										*(SP++) = GenericRNVStackElement ( leftChild(E), (E.sym<<1), E.offset );
									break;
								case 1:
									// register next visit of E
									E.visit = 2; ++SP;
									if ( rank1(E) )
										*(SP++) = GenericRNVStackElement ( rightChild(E), (E.sym<<1)|1, E.offset + offset );
									break;
								case 2:
									break;
							}
						}
					}
				}

				return ::std::numeric_limits<symbol_type>::max();
			}


		};
	}
}
#endif
