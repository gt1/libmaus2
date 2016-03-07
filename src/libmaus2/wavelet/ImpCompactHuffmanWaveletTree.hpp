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

#if ! defined(LIBMAUS2_WAVELET_IMPCOMPACTHUFFMANHUFFMANWAVELETTREE_HPP)
#define LIBMAUS2_WAVELET_IMPCOMPACTHUFFMANHUFFMANWAVELETTREE_HPP

#include <libmaus2/rank/ImpCacheLineRank.hpp>
#include <libmaus2/rank/RunLengthBitVector.hpp>
#include <libmaus2/util/NumberSerialisation.hpp>
#include <libmaus2/huffman/HuffmanTree.hpp>
#include <libmaus2/math/numbits.hpp>
#include <libmaus2/aio/InputStreamFactoryContainer.hpp>

namespace libmaus2
{
	namespace wavelet
	{
		template<typename _rank_type>
		struct ImpCompactHuffmanWaveletTreeTemplate
		{
			typedef _rank_type rank_type;
			typedef ImpCompactHuffmanWaveletTreeTemplate<rank_type> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			typedef typename rank_type::unique_ptr_type rank_ptr_type;
			typedef ::libmaus2::autoarray::AutoArray<rank_ptr_type> rank_array_type;

			uint64_t const n;
			libmaus2::huffman::HuffmanTree::unique_ptr_type H;
			libmaus2::huffman::HuffmanTree::EncodeTable::unique_ptr_type E;
			rank_array_type dicts;
			uint64_t maxdepth;
			std::vector<uint64_t> nodepos;

			ImpCompactHuffmanWaveletTreeTemplate(
				uint64_t const rn,
				libmaus2::huffman::HuffmanTree const & rH,
				rank_array_type & rdicts
			)
				: n(rn),
				  H(rH.uclone()),
				  E(new libmaus2::huffman::HuffmanTree::EncodeTable(*H)),
				  dicts(rdicts),
				  maxdepth(H->maxDepth()),
				  nodepos(dicts.size())
			{
				uint64_t p = 0;
				p += sizeof(uint64_t);
				p += H->serialisedSize();
				p += sizeof(uint64_t);
				for ( uint64_t i = 0; i < dicts.size(); ++i )
				{
					nodepos[i] = p;
					p += dicts[i]->serialisedSize();
				}
			}

			libmaus2::autoarray::AutoArray<int64_t> symbolArray() const
			{
				return H->symbolArray();
			}

			uint64_t byteSize() const
			{
				uint64_t s = 0;

				s += sizeof(uint64_t);
				// s += sroot->byteSize();

				s += dicts.byteSize();
				for ( uint64_t i = 0; i < dicts.size(); ++i )
					if ( dicts[i] )
						s += dicts[i]->byteSize();

				// s += enctable.byteSize();
				s += sizeof(uint64_t);

				return s;
			}

			bool haveSymbol(int64_t const sym) const
			{
				return E->hasSymbol(sym);
			}

			::libmaus2::autoarray::AutoArray<int64_t> getSymbolArray() const
			{
				::libmaus2::autoarray::AutoArray<int64_t> A(H->leafs());
				for ( uint64_t i = 0; i < H->leafs(); ++i )
					A[i] = H->getSymbol(i);
				return A;
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
					se.getStream() << "ImpCompactHuffmanWaveletTreeTemplate::getB() called on tree containing negative symbols in alphabet." << std::endl;
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
				// p += sroot->serialize(out);
				p += H->serialise(out);
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
				std::istream & istr = *Pistr;
				istr.seekg(-8,std::ios::end);
				uint64_t const indexpos = ::libmaus2::util::NumberSerialisation::deserialiseNumber(istr);
				istr.clear();
				istr.seekg(indexpos,std::ios::beg);
				return ::libmaus2::util::NumberSerialisation::deserialiseNumberVector<uint64_t>(istr);
			}

			void init()
			{
				maxdepth = 0;
				for ( uint64_t i = 0; i < H->leafs(); ++i )
					maxdepth = std::max(maxdepth,static_cast<uint64_t>(E->getCodeLength(H->getSymbol(i))));
			}

			template<typename stream_type>
			ImpCompactHuffmanWaveletTreeTemplate(stream_type & in)
			:
				// number of symbols
				n(::libmaus2::util::NumberSerialisation::deserialiseNumber(in)),
				// huffman tree
				H(new libmaus2::huffman::HuffmanTree(in)),
				E(new libmaus2::huffman::HuffmanTree::EncodeTable(*H)),
				// number of contexts = number of bit vectors
				dicts( ::libmaus2::util::NumberSerialisation::deserialiseNumber(in) ),
				//
				maxdepth(0)
			{
				// std::cerr << "** n=" << n << std::endl;

				for ( uint64_t i = 0; i < dicts.size(); ++i )
				{
					rank_ptr_type tdictsi(new rank_type(in));
					dicts [ i ] = UNIQUE_PTR_MOVE(tdictsi);
					// dicts [ i ] -> checkSanity();
				}

				nodepos = ::libmaus2::util::NumberSerialisation::deserialiseNumberVector<uint64_t>(in);
				::libmaus2::util::NumberSerialisation::deserialiseNumber(in); // index position

				init();
			}

			template<typename stream_type>
			ImpCompactHuffmanWaveletTreeTemplate(stream_type & in, uint64_t & s)
			:
				// number of symbols
				n(::libmaus2::util::NumberSerialisation::deserialiseNumber(in)),
				// root of huffman tree
				H(new libmaus2::huffman::HuffmanTree(in,s)),
				E(new libmaus2::huffman::HuffmanTree::EncodeTable(*H)),
				// number of contexts = number of bit vectors
				dicts( ::libmaus2::util::NumberSerialisation::deserialiseNumber(in) ),
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
			ImpCompactHuffmanWaveletTreeTemplate(
				std::string const & filename,
				uint64_t const rn,
				libmaus2::huffman::HuffmanTree const & rH,
				uint64_t const numnodes,
				std::vector<uint64_t> const & rnodepos
			)
			: n(rn), H(new libmaus2::huffman::HuffmanTree(rH)), E(new libmaus2::huffman::HuffmanTree::EncodeTable(*H)), dicts(numnodes),
			  maxdepth(0), nodepos(rnodepos)
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
				libmaus2::aio::InputStreamInstance::unique_ptr_type in(new libmaus2::aio::InputStreamInstance(filename));

				uint64_t const n = ::libmaus2::util::NumberSerialisation::deserialiseNumber(*in);
				::libmaus2::huffman::HuffmanTree H(*in);
				uint64_t const numnodes = ::libmaus2::util::NumberSerialisation::deserialiseNumber(*in);

				in.reset();

				std::vector<uint64_t> const nodepos = loadIndex(filename);

				unique_ptr_type p ( new this_type(filename,n,H,numnodes,nodepos) );

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
				uint64_t node = H->root();

				while ( !H->isLeaf(node) )
				{
					unsigned int sym;
					uint64_t const r1 = dicts[node-H->leafs()]->inverseSelect1(i,sym);

					if ( sym )
					{
						i = r1-1;
						node = H->rightChild(node);
					}
					else
					{
						i = i-r1;
						node = H->leftChild(node);
					}
				}

				assert ( H->isLeaf(node) );

				return H->getSymbol(node);
			}

			std::pair<int64_t,uint64_t> inverseSelect(uint64_t i) const
			{
				uint64_t node = H->root();

				while ( !H->isLeaf(node) )
				{
					unsigned int sym;
					uint64_t const r1 = dicts[node-H->leafs()]->inverseSelect1(i,sym);

					if ( sym )
					{
						i = r1-1;
						node = H->rightChild(node);
					}
					else
					{
						i = i-r1;
						node = H->leftChild(node);
					}
				}

				assert ( H->isLeaf(node) );

				return std::pair<int64_t,uint64_t>(H->getSymbol(node),i);
			}

			uint64_t rank(int64_t const sym, uint64_t i) const
			{
				if ( !E->hasSymbol(sym) )
					return 0;
				if ( H->leafs() == 1 )
					return i+1;

				unsigned int const b    = E->getCodeLength(sym);
				uint64_t     const s    = E->getCode(sym);
				uint64_t           node = H->root();

				for ( uint64_t mask = 1ull << (b-1); mask; mask >>= 1 )
				{
					if ( s & mask )
					{
						uint64_t const r1 = dicts[node-H->leafs()]->rank1(i);
						if ( ! r1 )
							return 0;
						i = r1-1;
						node = H->rightChild(node);
					}
					else
					{
						uint64_t const r0 = dicts[node-H->leafs()]->rank0(i);
						if ( ! r0 )
							return 0;
						i = r0-1;
						node = H->leftChild(node);
					}
				}

				return i+1;
			}

			uint64_t rankm(int64_t const sym, uint64_t i) const
			{
				if ( !E->hasSymbol(sym) )
					return 0;
				if ( H->leafs() == 1 )
					return i;

				unsigned int const b = E->getCodeLength(sym);
				uint64_t const s = E->getCode(sym);
				uint64_t node = H->root();

				for ( uint64_t mask = 1ull << (b-1); mask; mask >>= 1 )
				{
					if ( s & mask )
					{
						i = dicts[node-H->leafs()]->rankm1(i);
						node = H->rightChild(node);
					}
					else
					{
						i = dicts[node-H->leafs()]->rankm0(i);
						node = H->leftChild(node);
					}
				}

				return i;
			}

			std::pair<uint64_t,uint64_t> rankm(int64_t const sym, uint64_t l, uint64_t r) const
			{
				if ( !E->hasSymbol(sym) )
					return std::pair<uint64_t,uint64_t>(0,0);
				if ( H->leafs() == 1 )
					return std::pair<uint64_t,uint64_t>(rankm(sym,l),rankm(sym,r));

				unsigned int const b = E->getCodeLength(sym);
				uint64_t const s = E->getCode(sym);
				uint64_t node = H->root();

				for ( uint64_t mask = 1ull << (b-1); mask; mask >>= 1 )
				{
					if ( s & mask )
					{
						l = dicts[node-H->leafs()]->rankm1(l);
						r = dicts[node-H->leafs()]->rankm1(r);
						node = H->rightChild(node);
					}
					else
					{
						l = dicts[node-H->leafs()]->rankm0(l);
						r = dicts[node-H->leafs()]->rankm0(r);
						node = H->leftChild(node);
					}
				}

				return std::pair<uint64_t,uint64_t>(l,r);
			}

			uint64_t count(int64_t const sym, uint64_t const l, uint64_t const r) const
			{
				std::pair<uint64_t,uint64_t> const P = rankm(sym,l,r);
				return P.second-P.first;
			}

			template<typename iterator>
			std::pair<uint64_t,uint64_t> rankm(int64_t const sym, uint64_t l, uint64_t r, iterator D) const
			{
				if ( !E->hasSymbol(sym) )
					return std::pair<uint64_t,uint64_t>(0,0);
				if ( H->leafs() == 1 )
					return std::pair<uint64_t,uint64_t>(D[sym]+rankm(sym,l),D[sym]+rankm(sym,r));

				unsigned int const b = E->getCodeLength(sym);
				uint64_t const s = E->getCode(sym);
				uint64_t node = H->root();

				for ( uint64_t mask = 1ull << (b-1); mask; mask >>= 1 )
				{
					if ( s & mask )
					{
						l = dicts[node-H->leafs()]->rankm1(l);
						r = dicts[node-H->leafs()]->rankm1(r);
						node = H->rightChild(node);
					}
					else
					{
						l = dicts[node-H->leafs()]->rankm0(l);
						r = dicts[node-H->leafs()]->rankm0(r);
						node = H->leftChild(node);
					}
				}

				return std::pair<uint64_t,uint64_t>(D[sym]+l,D[sym]+r);
			}

			uint64_t select(int64_t const sym, uint64_t i) const
			{
				if ( !E->hasSymbol(sym) )
					return std::numeric_limits<uint64_t>::max();
				if ( H->isLeaf(H->root()) )
					return i;

				unsigned int const b = E->getCodeLength(sym);
				uint64_t const s = E->getCode(sym);
				uint64_t node = H->root();

				// parent links (+1 is because we call operator-- once more than operator++ below
				// actual depth we use is maxdepth-1
				#if defined(_MSC_VER) || defined(__MINGW32__)
				uint32_t * S = (reinterpret_cast<uint32_t *>(_alloca( maxdepth * sizeof(uint32_t) )))+1;
				#else
				uint32_t * S = (reinterpret_cast<uint32_t *>(alloca( maxdepth * sizeof(uint32_t) )))+1;
				#endif

				uint64_t mask;
				for ( mask = (1ull<<(b-1)); mask > 1; mask >>= 1 )
				{
					*(S++) = node;
					if ( mask & s )
						node = H->rightChild(node);
					else
						node = H->leftChild(node);
				}

				assert ( mask == 1 );

				do
				{
					if ( s & mask )
						i = dicts[node-H->leafs()]->select1(i);
					else
						i = dicts[node-H->leafs()]->select0(i);

					node = *(--S);
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

			struct EnumerateRangeSymbolsStackElement
			{
				enum visit { first, second, third };

				uint64_t l;
				uint64_t r;
				visit v;
				uint32_t node;

				EnumerateRangeSymbolsStackElement() : l(0), r(0), v(first), node(0) {}
				EnumerateRangeSymbolsStackElement(
					uint64_t const rl,
					uint64_t const rr,
					visit const rv,
					uint32_t const rnode
				) : l(rl), r(rr), v(rv), node(rnode) {}

				EnumerateRangeSymbolsStackElement left(libmaus2::huffman::HuffmanTree const & H, rank_array_type const & dicts) const
				{
					assert ( ! H.isLeaf(node) );

					return
						EnumerateRangeSymbolsStackElement(
							dicts[node-H.leafs()]->rankm0(l),
							dicts[node-H.leafs()]->rankm0(r),
							first,
							H.leftChild(node)
						);
				}
				EnumerateRangeSymbolsStackElement right(libmaus2::huffman::HuffmanTree const & H, rank_array_type const & dicts) const
				{
					assert ( ! H.isLeaf(node) );

					return
						EnumerateRangeSymbolsStackElement(
							dicts[node-H.leafs()]->rankm1(l),
							dicts[node-H.leafs()]->rankm1(r),
							first,
							H.rightChild(node)
						);
				}
			};

			struct EnumerateSymbolsRankStackElement
			{
				enum visit { first, second, third };

				uint64_t l;
				visit v;
				uint32_t node;

				EnumerateSymbolsRankStackElement() : l(0), v(first), node(0) {}
				EnumerateSymbolsRankStackElement(
					uint64_t const rl,
					visit const rv,
					uint32_t const rnode
				) : l(rl), v(rv), node(rnode) {}

				EnumerateSymbolsRankStackElement left(libmaus2::huffman::HuffmanTree const & H, rank_array_type const & dicts) const
				{
					assert ( ! H.isLeaf(node) );

					return
						EnumerateSymbolsRankStackElement(
							dicts[node-H.leafs()]->rankm0(l),
							first,
							H.leftChild(node)
						);
				}
				EnumerateSymbolsRankStackElement right(libmaus2::huffman::HuffmanTree const & H, rank_array_type const & dicts) const
				{
					assert ( ! H.isLeaf(node) );

					return
						EnumerateSymbolsRankStackElement(
							dicts[node-H.leafs()]->rankm1(l),
							first,
							H.rightChild(node)
						);
				}
			};

			template<typename iterator>
			void enumerateSymbolsRank(uint64_t const l, iterator P) const
			{
				uint64_t const stackdepth = maxdepth+1;
				uint64_t const stacksize = stackdepth * sizeof(EnumerateRangeSymbolsStackElement);

#if defined(_MSC_VER) || defined(__MINGW32__)
				EnumerateSymbolsRankStackElement * const S = reinterpret_cast<EnumerateSymbolsRankStackElement *>(_alloca( stacksize ));
#else
				EnumerateSymbolsRankStackElement * const S = reinterpret_cast<EnumerateSymbolsRankStackElement *>(alloca( stacksize ));
#endif

				EnumerateSymbolsRankStackElement * SP = S;

				*(SP++) = EnumerateSymbolsRankStackElement(l,EnumerateSymbolsRankStackElement::first,H->root());
				while ( SP != S )
				{
					EnumerateSymbolsRankStackElement & E = *(--SP);

					if ( H->isLeaf(E.node) )
					{
						P[H->getSymbol(E.node)] = E.l;
					}
					else
					{
						switch ( E.v )
						{
							case EnumerateSymbolsRankStackElement::first:
							{
								// register next visit of E
								E.v = EnumerateSymbolsRankStackElement::second; ++SP;
								EnumerateSymbolsRankStackElement SE = E.left(*H,dicts);
								*(SP++) = SE;
							}
								break;
							case EnumerateSymbolsRankStackElement::second:
							{
								// register next visit of E
								E.v = EnumerateSymbolsRankStackElement::third; ++SP;
								EnumerateSymbolsRankStackElement SE = E.right(*H,dicts);
								*(SP++) = SE;
							}
								break;
							case EnumerateSymbolsRankStackElement::third:
								break;
						}
					}
				}
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

				*(SP++) = EnumerateRangeSymbolsStackElement(l,r,EnumerateRangeSymbolsStackElement::first,H->root());
				while ( SP != S )
				{
					EnumerateRangeSymbolsStackElement & E = *(--SP);

					if ( E.r-E.l )
					{
						if ( H->isLeaf(E.node) )
						{
							M[H->getSymbol(E.node)] = E.r-E.l;
						}
						else
						{
							switch ( E.v )
							{
								case EnumerateRangeSymbolsStackElement::first:
								{
									// register next visit of E
									E.v = EnumerateRangeSymbolsStackElement::second; ++SP;
									EnumerateRangeSymbolsStackElement SE = E.left(*H,dicts);
									if ( SE.r-SE.l )
										*(SP++) = SE;
								}
									break;
								case EnumerateRangeSymbolsStackElement::second:
								{
									// register next visit of E
									E.v = EnumerateRangeSymbolsStackElement::third; ++SP;
									EnumerateRangeSymbolsStackElement SE = E.right(*H,dicts);
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

			template<typename iterator>
			uint64_t enumerateSymbolsInRangeSorted(uint64_t const l, uint64_t const r, iterator P) const
			{
				return enumerateSymbolsInRange<iterator,true>(l,r,P);
			}

			template<typename iterator>
			uint64_t enumerateSymbolsInRangeUnsorted(uint64_t const l, uint64_t const r, iterator P) const
			{
				return enumerateSymbolsInRange<iterator,false>(l,r,P);
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
				*(SP++) = EnumerateRangeSymbolsStackElement(l,r,EnumerateRangeSymbolsStackElement::first,H->root());
				while ( SP != S )
				{
					EnumerateRangeSymbolsStackElement & E = *(--SP);

					if ( E.r-E.l )
					{
						if ( H->isLeaf(E.node) )
						{
							*(PP++) = std::pair<int64_t,uint64_t>(
								H->getSymbol(E.node),
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
									EnumerateRangeSymbolsStackElement SE = E.left(*H,dicts);
									if ( SE.r-SE.l )
										*(SP++) = SE;
								}
									break;
								case EnumerateRangeSymbolsStackElement::second:
								{
									// register next visit of E
									E.v = EnumerateRangeSymbolsStackElement::third; ++SP;
									EnumerateRangeSymbolsStackElement SE = E.right(*H,dicts);
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

				*(SP++) = EnumerateRangeSymbolsStackElement(l,r,EnumerateRangeSymbolsStackElement::first,H->root());
				while ( SP != S )
				{
					EnumerateRangeSymbolsStackElement & E = *(--SP);

					if ( E.r-E.l )
					{
						if ( H->isLeaf(E.node) )
						{
							P [ H->getSymbol(E.node) ] = E.r-E.l;
						}
						else
						{
							switch ( E.v )
							{
								case EnumerateRangeSymbolsStackElement::first:
								{
									// register next visit of E
									E.v = EnumerateRangeSymbolsStackElement::second; ++SP;
									EnumerateRangeSymbolsStackElement SE = E.left(*H,dicts);
									if ( SE.r-SE.l )
										*(SP++) = SE;
								}
									break;
								case EnumerateRangeSymbolsStackElement::second:
								{
									// register next visit of E
									E.v = EnumerateRangeSymbolsStackElement::third; ++SP;
									EnumerateRangeSymbolsStackElement SE = E.right(*H,dicts);
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
				*(SP++) = EnumerateRangeSymbolsStackElement(l,r,EnumerateRangeSymbolsStackElement::first,H->root());
				while ( SP != S )
				{
					EnumerateRangeSymbolsStackElement & E = *(--SP);

					if ( E.r-E.l )
					{
						if ( H->isLeaf(E.node) )
							*(PP++) = StepElement(H->getSymbol(E.node), E.l, E.l-E.r);
						else
						{
							switch ( E.v )
							{
								case EnumerateRangeSymbolsStackElement::first:
								{
									// register next visit of E
									E.v = EnumerateRangeSymbolsStackElement::second; ++SP;
									EnumerateRangeSymbolsStackElement SE = E.left(*H,dicts);
									if ( SE.r-SE.l )
										*(SP++) = SE;
								}
									break;
								case EnumerateRangeSymbolsStackElement::second:
								{
									// register next visit of E
									E.v = EnumerateRangeSymbolsStackElement::third; ++SP;
									EnumerateRangeSymbolsStackElement SE = E.right(*H,dicts);
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

				*(SP++) = EnumerateRangeSymbolsStackElement(l,r,EnumerateRangeSymbolsStackElement::first,H->root());
				while ( SP != S )
				{
					EnumerateRangeSymbolsStackElement & E = *(--SP);

					if ( E.r-E.l )
					{
						if ( H->isLeaf(E.node) )
						{
							P [ H->getSymbol(E.node) ] = std::pair<uint64_t,uint64_t>(E.l,E.r-E.l);
						}
						else
						{
							switch ( E.v )
							{
								case EnumerateRangeSymbolsStackElement::first:
								{
									// register next visit of E
									E.v = EnumerateRangeSymbolsStackElement::second; ++SP;
									EnumerateRangeSymbolsStackElement SE = E.left(*H,dicts);
									if ( SE.r-SE.l )
										*(SP++) = SE;
								}
									break;
								case EnumerateRangeSymbolsStackElement::second:
								{
									// register next visit of E
									E.v = EnumerateRangeSymbolsStackElement::third; ++SP;
									EnumerateRangeSymbolsStackElement SE = E.right(*H,dicts);
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
				*(SP++) = EnumerateRangeSymbolsStackElement(l,r,EnumerateRangeSymbolsStackElement::first,H->root());
				while ( SP != S )
				{
					EnumerateRangeSymbolsStackElement & E = *(--SP);

					if ( includeEmpty || (E.r-E.l) )
					{
						if ( H->isLeaf(E.node) )
						{
							int64_t const sym = H->getSymbol(E.node);
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
									EnumerateRangeSymbolsStackElement SE = E.left(*H,dicts);
									if ( includeEmpty || (SE.r-SE.l) )
										*(SP++) = SE;
								}
									break;
								case EnumerateRangeSymbolsStackElement::second:
								{
									// register next visit of E
									E.v = EnumerateRangeSymbolsStackElement::third; ++SP;
									EnumerateRangeSymbolsStackElement SE = E.right(*H,dicts);
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

			template<typename iterator_D, typename iterator_P>
			uint64_t multiStepSortedNoEmpty(uint64_t const l, uint64_t const r, iterator_D D, iterator_P P) const
			{
				return multiStep<iterator_D,iterator_P,true,false>(l,r,D,P);
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

				*(SP++) = EnumerateRangeSymbolsStackElement(l,r,EnumerateRangeSymbolsStackElement::first,H->root());
				while ( SP != S )
				{
					EnumerateRangeSymbolsStackElement & E = *(--SP);

					if ( includeEmpty || (E.r-E.l) )
					{
						if ( H->isLeaf(E.node) )
						{
							int64_t const sym = H->getSymbol(E.node);
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
									EnumerateRangeSymbolsStackElement SE = E.left(*H,dicts);
									if ( includeEmpty || (SE.r-SE.l) )
										*(SP++) = SE;
								}
									break;
								case EnumerateRangeSymbolsStackElement::second:
								{
									// register next visit of E
									E.v = EnumerateRangeSymbolsStackElement::third; ++SP;
									EnumerateRangeSymbolsStackElement SE = E.right(*H,dicts);
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

				*(SP++) = EnumerateRangeSymbolsStackElement(l,r,EnumerateRangeSymbolsStackElement::first,H->root());
				while ( SP != S )
				{
					EnumerateRangeSymbolsStackElement & E = *(--SP);

					if ( (E.r-E.l) )
					{
						if ( H->isLeaf(E.node) )
						{
							int64_t const sym = H->getSymbol(E.node);
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
									EnumerateRangeSymbolsStackElement SE = E.left(*H,dicts);
									if ( (SE.r-SE.l) )
										*(SP++) = SE;
								}
									break;
								case EnumerateRangeSymbolsStackElement::second:
								{
									// register next visit of E
									E.v = EnumerateRangeSymbolsStackElement::third; ++SP;
									EnumerateRangeSymbolsStackElement SE = E.right(*H,dicts);
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

				*(SP++) = EnumerateRangeSymbolsStackElement(l,r,EnumerateRangeSymbolsStackElement::first,H->root());
				while ( SP != S )
				{
					EnumerateRangeSymbolsStackElement & E = *(--SP);

					if ( (E.r-E.l) )
					{
						if ( H->isLeaf(E.node) )
						{
							int64_t const sym = H->getSymbol(E.node);
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
									EnumerateRangeSymbolsStackElement SE = E.left(*H,dicts);
									if ( (SE.r-SE.l) )
										*(SP++) = SE;
								}
									break;
								case EnumerateRangeSymbolsStackElement::second:
								{
									// register next visit of E
									E.v = EnumerateRangeSymbolsStackElement::third; ++SP;
									EnumerateRangeSymbolsStackElement SE = E.right(*H,dicts);
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

		typedef ImpCompactHuffmanWaveletTreeTemplate< ::libmaus2::rank::ImpCacheLineRank > ImpCompactHuffmanWaveletTree;
		typedef ImpCompactHuffmanWaveletTreeTemplate< ::libmaus2::rank::RunLengthBitVector > ImpCompactRLHuffmanWaveletTree;
	}
}
#endif
