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

#if ! defined(IMPHUFFMANWAVELETTREE_HPP)
#define IMPHUFFMANWAVELETTREE_HPP

#include <libmaus2/rank/ImpCacheLineRank.hpp>
#include <libmaus2/util/NumberSerialisation.hpp>
#include <libmaus2/huffman/HuffmanTreeNode.hpp>
#include <libmaus2/huffman/HuffmanTreeInnerNode.hpp>
#include <libmaus2/huffman/EncodeTable.hpp>
#include <libmaus2/math/numbits.hpp>
#include <libmaus2/aio/InputStreamFactoryContainer.hpp>

namespace libmaus2
{
	namespace wavelet
	{
		struct ImpHuffmanWaveletTree
		{
			typedef ImpHuffmanWaveletTree this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			typedef ::libmaus2::rank::ImpCacheLineRank rank_type;
			typedef rank_type::unique_ptr_type rank_ptr_type;
			typedef ::libmaus2::autoarray::AutoArray<rank_ptr_type> rank_array_type;

			uint64_t const n;
			::libmaus2::util::shared_ptr < ::libmaus2::huffman::HuffmanTreeNode >::type const sroot;
			rank_array_type dicts;
			rank_type const * root;
			::libmaus2::huffman::EncodeTable<1> enctable;
			uint64_t maxdepth;

			std::vector<uint64_t> nodepos;

			uint64_t byteSize() const
			{
				uint64_t s = 0;

				s += sizeof(uint64_t);
				s += sroot->byteSize();

				s += dicts.byteSize();
				for ( uint64_t i = 0; i < dicts.size(); ++i )
					if ( dicts[i] )
						s += dicts[i]->byteSize();

				s += enctable.byteSize();
				s += sizeof(uint64_t);

				return s;
			}

			bool haveSymbol(::libmaus2::huffman::HuffmanTreeNode const * node, int64_t const sym) const
			{
				if ( node->isLeaf() )
				{
					::libmaus2::huffman::HuffmanTreeLeaf const * leaf =
						dynamic_cast< ::libmaus2::huffman::HuffmanTreeLeaf const *>(node);

					return leaf->symbol == sym;
				}
				else
				{
					::libmaus2::huffman::HuffmanTreeInnerNode const * inode =
						dynamic_cast< ::libmaus2::huffman::HuffmanTreeInnerNode const *>(node);

					return haveSymbol(inode->left,sym) || haveSymbol(inode->right,sym);
				}
			}

			bool haveSymbol(int64_t const sym) const
			{
				return haveSymbol(sroot.get(),sym);
			}

			::libmaus2::autoarray::AutoArray<int64_t> getSymbolArray() const
			{
				return sroot->symbolArray();
			}

			uint64_t getB() const
			{
				::libmaus2::autoarray::AutoArray<int64_t> syms = getSymbolArray();
				std::sort(syms.begin(),syms.end());
				if ( ! syms.size() )
				{
					return 0;
				}
				else if ( syms[0] < 0 )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "ImpHuffmanWaveletTree::getB() called on tree containing negative symbols in alphabet." << std::endl;
					se.finish();
					throw se;
				}
				else
				{
					return ::libmaus2::math::numbits(static_cast<uint64_t>(syms[syms.size()-1]));
				}
			}

			uint64_t serialise(std::ostream & out) const
			{
				uint64_t p = 0;
				p += ::libmaus2::util::NumberSerialisation::serialiseNumber(out,n);
				p += sroot->serialize(out);
				p += ::libmaus2::util::NumberSerialisation::serialiseNumber(out,dicts.size());
				for ( uint64_t i = 0; i < dicts.size(); ++i )
					p += dicts[i]->serialise(out);
				uint64_t const indexpos = p;
				p += ::libmaus2::util::NumberSerialisation::serialiseNumberVector(out,nodepos);
				p += ::libmaus2::util::NumberSerialisation::serialiseNumber(out,indexpos);
				out.flush();

				return p;
			}

			uint64_t serialize(std::ostream & out) const
			{
				return serialise(out);
			}

			static std::vector<uint64_t> loadIndex(std::string const & filename)
			{
				libmaus2::aio::InputStream::unique_ptr_type Pistr(libmaus2::aio::InputStreamFactoryContainer::constructUnique(filename));
				Pistr->seekg(-8,std::ios::end);
				uint64_t const indexpos = ::libmaus2::util::NumberSerialisation::deserialiseNumber(*Pistr);
				Pistr->clear();
				Pistr->seekg(indexpos,std::ios::beg);
				return ::libmaus2::util::NumberSerialisation::deserialiseNumberVector<uint64_t>(*Pistr);
			}

			void init()
			{
				std::map < ::libmaus2::huffman::HuffmanTreeNode const * , ::libmaus2::huffman::HuffmanTreeInnerNode const * > parentMap;
				std::map < ::libmaus2::huffman::HuffmanTreeInnerNode const *, uint64_t > nodeToId;

				std::stack < ::libmaus2::huffman::HuffmanTreeNode const * > S;
				S.push(sroot.get());

				while ( ! S.empty() )
				{
					::libmaus2::huffman::HuffmanTreeNode const * cur = S.top();
					S.pop();

					if ( !cur->isLeaf() )
					{
						::libmaus2::huffman::HuffmanTreeInnerNode const * node = dynamic_cast< ::libmaus2::huffman::HuffmanTreeInnerNode const *>(cur);
						uint64_t const id = nodeToId.size();
						nodeToId [ node ] = id;

						parentMap [ node->left ] = node;
						parentMap [ node->right ] = node;

						// push children
						S.push ( node->right );
						S.push ( node->left );
					}
				}

				for ( std::map < ::libmaus2::huffman::HuffmanTreeInnerNode const *, uint64_t >::const_iterator ita = nodeToId.begin();
					ita != nodeToId.end(); ++ita )
				{
					::libmaus2::huffman::HuffmanTreeInnerNode const * node = ita->first;
					uint64_t const nodeid = ita->second;

					if ( parentMap.find(node) != parentMap.end() )
					{
						::libmaus2::huffman::HuffmanTreeInnerNode const * parent = parentMap.find(node)->second;
						uint64_t const parentid = nodeToId.find(parent)->second;

						dicts [ nodeid ] -> parent = dicts [ parentid ].get();

						assert ( node == parent->left || node == parent->right );

						if ( node == parent->left )
						{
							dicts [ parentid ] -> left = dicts [ nodeid ].get();
						}
						else
						{
							dicts [ parentid ] -> right = dicts [ nodeid ] .get();
						}
					}

					#if 0
					if ( node->left->isLeaf() )
					{
						dicts[nodeid]->left = dynamic_cast< ::libmaus2::huffman::HuffmanTreeLeaf const * >(node->left)->symbol;
					}
					if ( node->right->isLeaf() )
					{
						dicts[nodeid]->right = dynamic_cast< ::libmaus2::huffman::HuffmanTreeLeaf const * >(node->right)->symbol;
					}
					#endif
				}

				if ( dicts.size() )
				{
					root = dicts[0].get();
				}

				::libmaus2::autoarray::AutoArray<uint64_t> depth = sroot->depthArray();
				maxdepth = 0;
				for ( uint64_t i = 0; i < depth.size(); ++i )
					maxdepth = std::max(maxdepth,depth[i]);
			}

			template<typename stream_type>
			ImpHuffmanWaveletTree(stream_type & in)
			:
				// number of symbols
				n(::libmaus2::util::NumberSerialisation::deserialiseNumber(in)),
				// root of huffman tree
				sroot ( ::libmaus2::huffman::HuffmanTreeNode::deserialize(in) ),
				// number of contexts = number of bit vectors
				dicts( ::libmaus2::util::NumberSerialisation::deserialiseNumber(in) ),
				// root of dictionary tree
				root(0),
				//
				enctable(sroot.get()),
				//
				maxdepth(0)
			{
				// std::cerr << "** n=" << n << std::endl;

				for ( uint64_t i = 0; i < dicts.size(); ++i )
				{
					rank_ptr_type tdictsi(new rank_type(in));
					dicts [ i ] = UNIQUE_PTR_MOVE(tdictsi);
					dicts [ i ] -> checkSanity();
				}

				nodepos = ::libmaus2::util::NumberSerialisation::deserialiseNumberVector<uint64_t>(in);
				::libmaus2::util::NumberSerialisation::deserialiseNumber(in); // index position

				init();
			}

			template<typename stream_type>
			ImpHuffmanWaveletTree(stream_type & in, uint64_t & s)
			:
				// number of symbols
				n(::libmaus2::util::NumberSerialisation::deserialiseNumber(in)),
				// root of huffman tree
				sroot ( ::libmaus2::huffman::HuffmanTreeNode::deserialize(in,s) ),
				// number of contexts = number of bit vectors
				dicts( ::libmaus2::util::NumberSerialisation::deserialiseNumber(in) ),
				// root of dictionary tree
				root(0),
				//
				enctable(sroot.get()),
				//
				maxdepth(0)
			{
				s += 2*sizeof(uint64_t);

				for ( uint64_t i = 0; i < dicts.size(); ++i )
				{
					rank_ptr_type tdictsi(new rank_type(in,s));
					dicts [ i ] = UNIQUE_PTR_MOVE(tdictsi);
				}

				nodepos = ::libmaus2::util::NumberSerialisation::deserialiseNumberVector<uint64_t>(in);
				::libmaus2::util::NumberSerialisation::deserialiseNumber(in); // index position

				s += (1+nodepos.size()+1)*sizeof(uint64_t);

				init();
			}

			private:
			ImpHuffmanWaveletTree(
				std::string const & filename,
				uint64_t const rn,
				::libmaus2::util::shared_ptr< ::libmaus2::huffman::HuffmanTreeNode >::type rsroot,
				uint64_t const numnodes,
				std::vector<uint64_t> const & rnodepos
			)
			: n(rn), sroot(rsroot), dicts(numnodes), root(0), enctable(sroot.get()), maxdepth(0), nodepos(rnodepos)
			{
				#if defined(_OPENMP)
				#pragma omp parallel for schedule(dynamic,1)
				#endif
				for ( int64_t i = 0; i < static_cast<int64_t>(dicts.size()); ++i )
				{
					libmaus2::aio::InputStream::unique_ptr_type Pistr(libmaus2::aio::InputStreamFactoryContainer::constructUnique(filename));
					std::istream & istr = *Pistr;
					istr.seekg(nodepos[i],std::ios::beg);
					rank_ptr_type tdictsi(new rank_type(istr));
					dicts[i] = UNIQUE_PTR_MOVE(tdictsi);
				}

				init();
			}

			public:
			static unique_ptr_type load(std::string const & filename)
			{
				libmaus2::aio::InputStream::unique_ptr_type Pistr(libmaus2::aio::InputStreamFactoryContainer::constructUnique(filename));
				std::istream & in = *Pistr;
				uint64_t const n = ::libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				::libmaus2::util::shared_ptr< ::libmaus2::huffman::HuffmanTreeNode >::type const sroot = ::libmaus2::huffman::HuffmanTreeNode::deserialize(in);
				uint64_t const numnodes = ::libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				Pistr.reset();

				std::vector<uint64_t> const nodepos = loadIndex(filename);

				unique_ptr_type p ( new this_type(filename,n,sroot,numnodes,nodepos) );

				return UNIQUE_PTR_MOVE(p);
			}

			uint64_t getN() const
			{
				return n;
			}

			uint64_t size() const
			{
				return n;
			}

			int64_t operator[](uint64_t i) const
			{
				rank_type const * node = root;
				::libmaus2::huffman::HuffmanTreeNode const * hnode = sroot.get();
				unsigned int sym;

				#if 0
				if ( i % (32*1024) == 0 )
					std::cerr << "i=" << i << std::endl;
				#endif

				while ( node )
				{
					uint64_t const r1 = node->inverseSelect1(i,sym);

					if ( sym )
					{
						i = r1-1;
						node = node->right;
						hnode = dynamic_cast< ::libmaus2::huffman::HuffmanTreeInnerNode const *>(hnode)->right;
					}
					else
					{
						i = i-r1;
						node = node->left;
						hnode = dynamic_cast< ::libmaus2::huffman::HuffmanTreeInnerNode const *>(hnode)->left;
					}
				}

				assert ( hnode->isLeaf() );

				return dynamic_cast< ::libmaus2::huffman::HuffmanTreeLeaf const *>(hnode)->symbol;
			}

			std::pair<int64_t,uint64_t> inverseSelect(uint64_t i) const
			{
				rank_type const * node = root;
				::libmaus2::huffman::HuffmanTreeNode const * hnode = sroot.get();
				unsigned int sym;

				while ( node )
				{
					uint64_t const r1 = node->inverseSelect1(i,sym);

					if ( sym )
					{
						i = r1-1;
						node = node->right;
						hnode = dynamic_cast< ::libmaus2::huffman::HuffmanTreeInnerNode const *>(hnode)->right;
					}
					else
					{
						i = i-r1;
						node = node->left;
						hnode = dynamic_cast< ::libmaus2::huffman::HuffmanTreeInnerNode const *>(hnode)->left;
					}
				}

				assert ( hnode->isLeaf() );

				return std::pair<uint64_t,uint64_t>(
					dynamic_cast< ::libmaus2::huffman::HuffmanTreeLeaf const *>(hnode)->symbol,
					i);
			}

			uint64_t rank(int64_t const sym, uint64_t i) const
			{
				if ( !enctable.checkSymbol(sym) )
					return 0;

				unsigned int const b = enctable[sym].second;
				uint64_t const s = enctable[sym].first.A[0];
				rank_type const * node = root;

				// #define HDEBUG
				#if defined(HDEBUG)
				::libmaus2::huffman::HuffmanTreeNode const * hnode = sroot.get();
				std::cerr << "code is " << s << " length "<< b << std::endl;
				#endif


				for ( uint64_t mask = 1ull << (b-1); mask; mask >>= 1 )
				{
					#if defined(HDEBUG)
					bool const bit = s & mask;
					std::cerr << "bit=" << bit << " mask=" << mask << std::endl;
					#endif

					if ( s & mask )
					{
						uint64_t const r1 = node->rank1(i);
						if ( ! r1 )
							return 0;
						i = r1-1;
						node = node->right;

						#if defined(HDEBUG)
						hnode = dynamic_cast< ::libmaus2::huffman::HuffmanTreeInnerNode const *>(hnode)->right;
						#endif
					}
					else
					{
						uint64_t const r0 = node->rank0(i);
						if ( ! r0 )
							return 0;
						i = r0-1;
						node = node->left;

						#if defined(HDEBUG)
						hnode = dynamic_cast< ::libmaus2::huffman::HuffmanTreeInnerNode const *>(hnode)->left;
						#endif
					}
				}

				#if defined(HDEBUG)
				assert ( hnode->isLeaf() );

				assert (
					dynamic_cast< ::libmaus2::huffman::HuffmanTreeLeaf const *>(hnode)->symbol
					== sym );
				std::cerr << "here." << std::endl;
				#endif


				return i+1;
			}

			uint64_t rankm(int64_t const sym, uint64_t i) const
			{
				if ( !enctable.checkSymbol(sym) )
					return 0;

				unsigned int const b = enctable[sym].second;
				uint64_t const s = enctable[sym].first.A[0];
				rank_type const * node = root;

				for ( uint64_t mask = 1ull << (b-1); mask; mask >>= 1 )
				{
					if ( s & mask )
					{
						i = node->rankm1(i);
						node = node->right;
					}
					else
					{
						i = node->rankm0(i);
						node = node->left;
					}
				}

				return i;
			}

			std::pair<uint64_t,uint64_t> rankm(int64_t const sym, uint64_t l, uint64_t r) const
			{
				if ( !enctable.checkSymbol(sym) )
					return std::pair<uint64_t,uint64_t>(0,0);

				unsigned int const b = enctable[sym].second;
				uint64_t const s = enctable[sym].first.A[0];
				rank_type const * node = root;

				for ( uint64_t mask = 1ull << (b-1); mask; mask >>= 1 )
				{
					if ( s & mask )
					{
						l = node->rankm1(l);
						r = node->rankm1(r);
						node = node->right;
					}
					else
					{
						l = node->rankm0(l);
						r = node->rankm0(r);
						node = node->left;
					}
				}

				return std::pair<uint64_t,uint64_t>(l,r);
			}

			template<typename iterator>
			std::pair<uint64_t,uint64_t> rankm(int64_t const sym, uint64_t l, uint64_t r, iterator D) const
			{
				if ( !enctable.checkSymbol(sym) )
					return std::pair<uint64_t,uint64_t>(0,0);

				unsigned int const b = enctable[sym].second;
				uint64_t const s = enctable[sym].first.A[0];
				rank_type const * node = root;

				for ( uint64_t mask = 1ull << (b-1); mask; mask >>= 1 )
				{
					if ( s & mask )
					{
						l = node->rankm1(l);
						r = node->rankm1(r);
						node = node->right;
					}
					else
					{
						l = node->rankm0(l);
						r = node->rankm0(r);
						node = node->left;
					}
				}

				return std::pair<uint64_t,uint64_t>(D[sym]+l,D[sym]+r);
			}

			uint64_t select(int64_t const sym, uint64_t i) const
			{
				if ( !enctable.checkSymbol(sym) )
					return std::numeric_limits<uint64_t>::max();

				unsigned int const b = enctable[sym].second;
				uint64_t const s = enctable[sym].first.A[0];
				rank_type const * node = root;

				uint64_t mask;
				for ( mask = (1ull<<(b-1)); mask > 1; mask >>= 1 )
					if ( mask & s )
						node = node->right;
					else
						node = node->left;

				assert ( mask == 1 );

				do
				{
					if ( s & mask )
						i = node->select1(i);
					else
						i = node->select0(i);

					node = node->parent;
					mask <<= 1;

				} while ( mask != (1ull<<b) );

				return i;
			}

			std::map < int64_t, uint64_t > enumerateSymbolsInRangeSlow(uint64_t const l, uint64_t const r) const
			{
				std::map < int64_t, uint64_t > M;
				for ( uint64_t i = l; i < r; ++i )
					M [ (*this)[i] ] ++;
				return M;
			}

			std::map < int64_t, uint64_t > enumerateSymbolsInRange(uint64_t const l, uint64_t const r) const
			{
				uint64_t const stackdepth = maxdepth+1;
				uint64_t const stacksize = stackdepth * sizeof(EnumerateRangeSymbolsStackElement);

#if defined(_MSC_VER) || defined(__MINGW32__)
				EnumerateRangeSymbolsStackElement * const S = reinterpret_cast<EnumerateRangeSymbolsStackElement *>(_alloca( stacksize ));
#else
				EnumerateRangeSymbolsStackElement * const S = reinterpret_cast<EnumerateRangeSymbolsStackElement *>(alloca( stacksize ));
#endif

				EnumerateRangeSymbolsStackElement * SP = S;
				std::map < int64_t, uint64_t > M;

				// uint64_t cnt = 0;

				*(SP++) = EnumerateRangeSymbolsStackElement(l,r,EnumerateRangeSymbolsStackElement::first,root,sroot.get());
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

					if ( E.r-E.l )
					{
						if ( E.hnode->isLeaf() )
						{
							M[dynamic_cast< ::libmaus2::huffman::HuffmanTreeLeaf const * >(E.hnode)->symbol] = E.r-E.l;
						}
						else
						{
							switch ( E.v )
							{
								case EnumerateRangeSymbolsStackElement::first:
								{
									// register next visit of E
									E.v = EnumerateRangeSymbolsStackElement::second; ++SP;
									EnumerateRangeSymbolsStackElement SE = E.left();
									if ( SE.r-SE.l )
										*(SP++) = SE;
								}
									break;
								case EnumerateRangeSymbolsStackElement::second:
								{
									// register next visit of E
									E.v = EnumerateRangeSymbolsStackElement::third; ++SP;
									EnumerateRangeSymbolsStackElement SE = E.right();
									if ( SE.r-SE.l )
										*(SP++) = SE;
								}
									break;
								case EnumerateRangeSymbolsStackElement::third:
									break;
							}
						}
					}
				}

				return M;

			}

			template<typename iterator, bool sort /* = true */>
			uint64_t enumerateSymbolsInRange(uint64_t const l, uint64_t const r, iterator P) const
			{
				uint64_t const stackdepth = maxdepth+1;
				uint64_t const stacksize = stackdepth * sizeof(EnumerateRangeSymbolsStackElement);

#if defined(_MSC_VER) || defined(__MINGW32__)
				EnumerateRangeSymbolsStackElement * const S = reinterpret_cast<EnumerateRangeSymbolsStackElement *>(_alloca( stacksize ));
#else
				EnumerateRangeSymbolsStackElement * const S = reinterpret_cast<EnumerateRangeSymbolsStackElement *>(alloca( stacksize ));
#endif

				EnumerateRangeSymbolsStackElement * SP = S;

				iterator PP = P;
				*(SP++) = EnumerateRangeSymbolsStackElement(l,r,EnumerateRangeSymbolsStackElement::first,root,sroot.get());
				while ( SP != S )
				{
					EnumerateRangeSymbolsStackElement & E = *(--SP);

					if ( E.r-E.l )
					{
						if ( E.hnode->isLeaf() )
						{
							*(PP++) = std::pair<int64_t,uint64_t>(
								dynamic_cast< ::libmaus2::huffman::HuffmanTreeLeaf const * >(E.hnode)->symbol,
								E.r-E.l
							);
						}
						else
						{
							switch ( E.v )
							{
								case EnumerateRangeSymbolsStackElement::first:
								{
									// register next visit of E
									E.v = EnumerateRangeSymbolsStackElement::second; ++SP;
									EnumerateRangeSymbolsStackElement SE = E.left();
									if ( SE.r-SE.l )
										*(SP++) = SE;
								}
									break;
								case EnumerateRangeSymbolsStackElement::second:
								{
									// register next visit of E
									E.v = EnumerateRangeSymbolsStackElement::third; ++SP;
									EnumerateRangeSymbolsStackElement SE = E.right();
									if ( SE.r-SE.l )
										*(SP++) = SE;
								}
									break;
								case EnumerateRangeSymbolsStackElement::third:
									break;
							}
						}
					}
				}

				if ( sort )
					std::sort ( P,PP );
				return PP-P;
			}

			template<typename iterator>
			void enumerateSymbolsInRangeArray(uint64_t const l, uint64_t const r, iterator P) const
			{
				uint64_t const stackdepth = maxdepth+1;
				uint64_t const stacksize = stackdepth * sizeof(EnumerateRangeSymbolsStackElement);

#if defined(_MSC_VER) || defined(__MINGW32__)
				EnumerateRangeSymbolsStackElement * const S = reinterpret_cast<EnumerateRangeSymbolsStackElement *>(_alloca( stacksize ));
#else
				EnumerateRangeSymbolsStackElement * const S = reinterpret_cast<EnumerateRangeSymbolsStackElement *>(alloca( stacksize ));
#endif

				EnumerateRangeSymbolsStackElement * SP = S;

				*(SP++) = EnumerateRangeSymbolsStackElement(l,r,EnumerateRangeSymbolsStackElement::first,root,sroot.get());
				while ( SP != S )
				{
					EnumerateRangeSymbolsStackElement & E = *(--SP);

					if ( E.r-E.l )
					{
						if ( E.hnode->isLeaf() )
						{
							P [ dynamic_cast< ::libmaus2::huffman::HuffmanTreeLeaf const * >(E.hnode)->symbol ] = E.r-E.l;
						}
						else
						{
							switch ( E.v )
							{
								case EnumerateRangeSymbolsStackElement::first:
								{
									// register next visit of E
									E.v = EnumerateRangeSymbolsStackElement::second; ++SP;
									EnumerateRangeSymbolsStackElement SE = E.left();
									if ( SE.r-SE.l )
										*(SP++) = SE;
								}
									break;
								case EnumerateRangeSymbolsStackElement::second:
								{
									// register next visit of E
									E.v = EnumerateRangeSymbolsStackElement::third; ++SP;
									EnumerateRangeSymbolsStackElement SE = E.right();
									if ( SE.r-SE.l )
										*(SP++) = SE;
								}
									break;
								case EnumerateRangeSymbolsStackElement::third:
									break;
							}
						}
					}
				}
			}

			struct StepElement
			{
				int64_t sym;
				uint64_t base;
				uint64_t size;

				StepElement() {}
				StepElement(int64_t const rsym, uint64_t rbase, uint64_t rsize)
				: sym(rsym), base(rbase), size(rsize) {}
			};

			template<typename iterator>
			uint64_t stepDiv(uint64_t const l, uint64_t const r, StepElement * const P) const
			{
				uint64_t const stackdepth = maxdepth+1;
				uint64_t const stacksize = stackdepth * sizeof(EnumerateRangeSymbolsStackElement);

#if defined(_MSC_VER) || defined(__MINGW32__)
				EnumerateRangeSymbolsStackElement * const S = reinterpret_cast<EnumerateRangeSymbolsStackElement *>(_alloca( stacksize ));
#else
				EnumerateRangeSymbolsStackElement * const S = reinterpret_cast<EnumerateRangeSymbolsStackElement *>(alloca( stacksize ));
#endif

				StepElement * PP = P;
				EnumerateRangeSymbolsStackElement * SP = S;
				*(SP++) = EnumerateRangeSymbolsStackElement(l,r,EnumerateRangeSymbolsStackElement::first,root,sroot.get());
				while ( SP != S )
				{
					EnumerateRangeSymbolsStackElement & E = *(--SP);

					if ( E.r-E.l )
					{
						if ( E.hnode->isLeaf() )
							*(PP++) = StepElement(dynamic_cast< ::libmaus2::huffman::HuffmanTreeLeaf const * >(E.hnode)->symbol, E.l, E.l-E.r);
						else
						{
							switch ( E.v )
							{
								case EnumerateRangeSymbolsStackElement::first:
								{
									// register next visit of E
									E.v = EnumerateRangeSymbolsStackElement::second; ++SP;
									EnumerateRangeSymbolsStackElement SE = E.left();
									if ( SE.r-SE.l )
										*(SP++) = SE;
								}
									break;
								case EnumerateRangeSymbolsStackElement::second:
								{
									// register next visit of E
									E.v = EnumerateRangeSymbolsStackElement::third; ++SP;
									EnumerateRangeSymbolsStackElement SE = E.right();
									if ( SE.r-SE.l )
										*(SP++) = SE;
								}
									break;
								case EnumerateRangeSymbolsStackElement::third:
									break;
							}
						}
					}
				}

				return PP-P;
			}

			template<typename iterator>
			void stepDivArray(uint64_t const l, uint64_t const r, iterator P) const
			{
				uint64_t const stackdepth = maxdepth+1;
				uint64_t const stacksize = stackdepth * sizeof(EnumerateRangeSymbolsStackElement);

#if defined(_MSC_VER) || defined(__MINGW32__)
				EnumerateRangeSymbolsStackElement * const S = reinterpret_cast<EnumerateRangeSymbolsStackElement *>(_alloca( stacksize ));
#else
				EnumerateRangeSymbolsStackElement * const S = reinterpret_cast<EnumerateRangeSymbolsStackElement *>(alloca( stacksize ));
#endif

				EnumerateRangeSymbolsStackElement * SP = S;

				*(SP++) = EnumerateRangeSymbolsStackElement(l,r,EnumerateRangeSymbolsStackElement::first,root,sroot.get());
				while ( SP != S )
				{
					EnumerateRangeSymbolsStackElement & E = *(--SP);

					if ( E.r-E.l )
					{
						if ( E.hnode->isLeaf() )
						{
							P [ dynamic_cast< ::libmaus2::huffman::HuffmanTreeLeaf const * >(E.hnode)->symbol ] = std::pair<uint64_t,uint64_t>(E.l,E.r-E.l);
						}
						else
						{
							switch ( E.v )
							{
								case EnumerateRangeSymbolsStackElement::first:
								{
									// register next visit of E
									E.v = EnumerateRangeSymbolsStackElement::second; ++SP;
									EnumerateRangeSymbolsStackElement SE = E.left();
									if ( SE.r-SE.l )
										*(SP++) = SE;
								}
									break;
								case EnumerateRangeSymbolsStackElement::second:
								{
									// register next visit of E
									E.v = EnumerateRangeSymbolsStackElement::third; ++SP;
									EnumerateRangeSymbolsStackElement SE = E.right();
									if ( SE.r-SE.l )
										*(SP++) = SE;
								}
									break;
								case EnumerateRangeSymbolsStackElement::third:
									break;
							}
						}
					}
				}
			}

			struct EnumerateRangeSymbolsStackElement
			{
				enum visit { first, second, third };

				uint64_t l;
				uint64_t r;
				visit v;
				rank_type const * node;
				::libmaus2::huffman::HuffmanTreeNode const * hnode;

				EnumerateRangeSymbolsStackElement() : l(0), r(0), v(first), node(0), hnode(0) {}
				EnumerateRangeSymbolsStackElement(
					uint64_t const rl,
					uint64_t const rr,
					visit const rv,
					rank_type const * const rnode,
					::libmaus2::huffman::HuffmanTreeNode const * const rhnode
				) : l(rl), r(rr), v(rv), node(rnode), hnode(rhnode) {}

				EnumerateRangeSymbolsStackElement left() const
				{
					assert ( ! hnode->isLeaf() );

					return
						EnumerateRangeSymbolsStackElement(node->rankm0(l),node->rankm0(r),first,node->left, dynamic_cast< ::libmaus2::huffman::HuffmanTreeInnerNode const * >(hnode)->left);
				}
				EnumerateRangeSymbolsStackElement right() const
				{
					assert ( ! hnode->isLeaf() );

					return
						EnumerateRangeSymbolsStackElement(node->rankm1(l),node->rankm1(r),first,node->right, dynamic_cast< ::libmaus2::huffman::HuffmanTreeInnerNode const * >(hnode)->right);
				}
			};

			template<typename iterator_D, typename iterator_P>
			uint64_t multiStepSortedNoEmpty(uint64_t const l, uint64_t const r, iterator_D D, iterator_P P) const
			{
				return multiStep<iterator_D,iterator_P,true,false>(l,r,D,P);
			}

			template<typename iterator_D, typename iterator_P, bool const sort /* = true */, bool includeEmpty /* = false */>
			uint64_t multiStep(uint64_t const l, uint64_t const r, iterator_D D, iterator_P P) const
			{
				uint64_t const stackdepth = maxdepth+1;
				uint64_t const stacksize = stackdepth * sizeof(EnumerateRangeSymbolsStackElement);

#if defined(_MSC_VER) || defined(__MINGW32__)
				EnumerateRangeSymbolsStackElement * const S = reinterpret_cast<EnumerateRangeSymbolsStackElement *>(_alloca( stacksize ));
#else
				EnumerateRangeSymbolsStackElement * const S = reinterpret_cast<EnumerateRangeSymbolsStackElement *>(alloca( stacksize ));
#endif

				EnumerateRangeSymbolsStackElement * SP = S;

				iterator_P PP = P;
				*(SP++) = EnumerateRangeSymbolsStackElement(l,r,EnumerateRangeSymbolsStackElement::first,root,sroot.get());
				while ( SP != S )
				{
					EnumerateRangeSymbolsStackElement & E = *(--SP);

					if ( includeEmpty || (E.r-E.l) )
					{
						if ( E.hnode->isLeaf() )
						{
							int64_t const sym = dynamic_cast< ::libmaus2::huffman::HuffmanTreeLeaf const * >(E.hnode)->symbol;
							*(PP++) =
								std::pair< int64_t,std::pair<uint64_t,uint64_t> > (
									sym,
									std::pair<uint64_t,uint64_t>(
										D[sym] + E.l,
										D[sym] + E.r
									)
								);
						}
						else
						{
							switch ( E.v )
							{
								case EnumerateRangeSymbolsStackElement::first:
								{
									// register next visit of E
									E.v = EnumerateRangeSymbolsStackElement::second; ++SP;
									EnumerateRangeSymbolsStackElement SE = E.left();
									if ( includeEmpty || (SE.r-SE.l) )
										*(SP++) = SE;
								}
									break;
								case EnumerateRangeSymbolsStackElement::second:
								{
									// register next visit of E
									E.v = EnumerateRangeSymbolsStackElement::third; ++SP;
									EnumerateRangeSymbolsStackElement SE = E.right();
									if ( includeEmpty || (SE.r-SE.l) )
										*(SP++) = SE;
								}
									break;
								case EnumerateRangeSymbolsStackElement::third:
									break;
							}
						}
					}
				}

				if ( sort )
					std::sort ( P,PP );
				return PP-P;
			}

			template<typename iterator_D, typename callback_type>
			void multiRankCallBack(
				uint64_t const l,
				uint64_t const r,
				iterator_D D,
				callback_type & cb) const
			{
				multiRankCallBackTemplate<iterator_D,callback_type,false>(l,r,D,cb);
			}

			template<typename iterator_D, typename callback_type,
				bool includeEmpty /* = false */>
			void multiRankCallBackTemplate(
				uint64_t const l,
				uint64_t const r,
				iterator_D D,
				callback_type & cb) const
			{
				uint64_t const stackdepth = maxdepth+1;
				uint64_t const stacksize = stackdepth * sizeof(EnumerateRangeSymbolsStackElement);

#if defined(_MSC_VER) || defined(__MINGW32__)
				EnumerateRangeSymbolsStackElement * const S = reinterpret_cast<EnumerateRangeSymbolsStackElement *>(_alloca( stacksize ));
#else
				EnumerateRangeSymbolsStackElement * const S = reinterpret_cast<EnumerateRangeSymbolsStackElement *>(alloca( stacksize ));
#endif

				EnumerateRangeSymbolsStackElement * SP = S;

				*(SP++) = EnumerateRangeSymbolsStackElement(l,r,EnumerateRangeSymbolsStackElement::first,root,sroot.get());
				while ( SP != S )
				{
					EnumerateRangeSymbolsStackElement & E = *(--SP);

					if ( includeEmpty || (E.r-E.l) )
					{
						if ( E.hnode->isLeaf() )
						{
							int64_t const sym = dynamic_cast< ::libmaus2::huffman::HuffmanTreeLeaf const * >(E.hnode)->symbol;
							cb ( sym, D[sym] + E.l, D[sym] + E.r );
						}
						else
						{
							switch ( E.v )
							{
								case EnumerateRangeSymbolsStackElement::first:
								{
									// register next visit of E
									E.v = EnumerateRangeSymbolsStackElement::second; ++SP;
									EnumerateRangeSymbolsStackElement SE = E.left();
									if ( includeEmpty || (SE.r-SE.l) )
										*(SP++) = SE;
								}
									break;
								case EnumerateRangeSymbolsStackElement::second:
								{
									// register next visit of E
									E.v = EnumerateRangeSymbolsStackElement::third; ++SP;
									EnumerateRangeSymbolsStackElement SE = E.right();
									if ( includeEmpty || (SE.r-SE.l) )
										*(SP++) = SE;
								}
									break;
								case EnumerateRangeSymbolsStackElement::third:
									break;
							}
						}
					}
				}
			}

			template<typename lcp_iterator, typename queue_type, typename iterator_D>
			inline uint64_t multiRankLCPSet(
				uint64_t const l,
				uint64_t const r,
				iterator_D D,
				lcp_iterator WLCP,
				typename std::iterator_traits<lcp_iterator>::value_type const unset,
				typename std::iterator_traits<lcp_iterator>::value_type const cur_l,
				queue_type * PQ1
				) const
			{
				uint64_t const stackdepth = maxdepth+1;
				uint64_t const stacksize = stackdepth * sizeof(EnumerateRangeSymbolsStackElement);

#if defined(_MSC_VER) || defined(__MINGW32__)
				EnumerateRangeSymbolsStackElement * const S = reinterpret_cast<EnumerateRangeSymbolsStackElement *>(_alloca( stacksize ));
#else
				EnumerateRangeSymbolsStackElement * const S = reinterpret_cast<EnumerateRangeSymbolsStackElement *>(alloca( stacksize ));
#endif

				EnumerateRangeSymbolsStackElement * SP = S;
				uint64_t s = 0;

				*(SP++) = EnumerateRangeSymbolsStackElement(l,r,EnumerateRangeSymbolsStackElement::first,root,sroot.get());
				while ( SP != S )
				{
					EnumerateRangeSymbolsStackElement & E = *(--SP);

					if ( (E.r-E.l) )
					{
						if ( E.hnode->isLeaf() )
						{
							int64_t const sym = dynamic_cast< ::libmaus2::huffman::HuffmanTreeLeaf const * >(E.hnode)->symbol;
							uint64_t const sp = D[sym] + E.l;
							uint64_t const ep = D[sym] + E.r;

							if ( WLCP[ep] == unset )
	                	                        {
	                	                        	WLCP[ep] = cur_l;
	                	                        	PQ1->push_back( std::pair<uint64_t,uint64_t>(sp,ep) );
	                	                        	s++;
							}
						}
						else
						{
							switch ( E.v )
							{
								case EnumerateRangeSymbolsStackElement::first:
								{
									// register next visit of E
									E.v = EnumerateRangeSymbolsStackElement::second; ++SP;
									EnumerateRangeSymbolsStackElement SE = E.left();
									if ( (SE.r-SE.l) )
										*(SP++) = SE;
								}
									break;
								case EnumerateRangeSymbolsStackElement::second:
								{
									// register next visit of E
									E.v = EnumerateRangeSymbolsStackElement::third; ++SP;
									EnumerateRangeSymbolsStackElement SE = E.right();
									if ( (SE.r-SE.l) )
										*(SP++) = SE;
								}
									break;
								case EnumerateRangeSymbolsStackElement::third:
									break;
							}
						}
					}
				}

				return s;
			}

			template<typename lcp_iterator, typename queue_type, typename iterator_D>
			inline uint64_t multiRankLCPSetLarge(
				uint64_t const l,
				uint64_t const r,
				iterator_D D,
				lcp_iterator & WLCP,
				uint64_t const cur_l,
				queue_type * PQ1
				) const
			{
				uint64_t const stackdepth = maxdepth+1;
				uint64_t const stacksize = stackdepth * sizeof(EnumerateRangeSymbolsStackElement);

#if defined(_MSC_VER) || defined(__MINGW32__)
				EnumerateRangeSymbolsStackElement * const S = reinterpret_cast<EnumerateRangeSymbolsStackElement *>(_alloca( stacksize ));
#else
				EnumerateRangeSymbolsStackElement * const S = reinterpret_cast<EnumerateRangeSymbolsStackElement *>(alloca( stacksize ));
#endif

				EnumerateRangeSymbolsStackElement * SP = S;
				uint64_t s = 0;

				*(SP++) = EnumerateRangeSymbolsStackElement(l,r,EnumerateRangeSymbolsStackElement::first,root,sroot.get());
				while ( SP != S )
				{
					EnumerateRangeSymbolsStackElement & E = *(--SP);

					if ( (E.r-E.l) )
					{
						if ( E.hnode->isLeaf() )
						{
							int64_t const sym = dynamic_cast< ::libmaus2::huffman::HuffmanTreeLeaf const * >(E.hnode)->symbol;
							uint64_t const sp = D[sym] + E.l;
							uint64_t const ep = D[sym] + E.r;

							if ( WLCP.isUnset(ep) )
		                                        {
        		                                        WLCP.set(ep, cur_l);
                		                                PQ1->push_back( std::pair<uint64_t,uint64_t>(sp,ep) );
                		                                s++;
							}
						}
						else
						{
							switch ( E.v )
							{
								case EnumerateRangeSymbolsStackElement::first:
								{
									// register next visit of E
									E.v = EnumerateRangeSymbolsStackElement::second; ++SP;
									EnumerateRangeSymbolsStackElement SE = E.left();
									if ( (SE.r-SE.l) )
										*(SP++) = SE;
								}
									break;
								case EnumerateRangeSymbolsStackElement::second:
								{
									// register next visit of E
									E.v = EnumerateRangeSymbolsStackElement::third; ++SP;
									EnumerateRangeSymbolsStackElement SE = E.right();
									if ( (SE.r-SE.l) )
										*(SP++) = SE;
								}
									break;
								case EnumerateRangeSymbolsStackElement::third:
									break;
							}
						}
					}
				}

				return s;
			}
		};
	}
}
#endif
