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
#if ! defined(LIBMAUS2_SUFFIXTREE_COMPRESSEDSUFFIXTREE_HPP)
#define LIBMAUS2_SUFFIXTREE_COMPRESSEDSUFFIXTREE_HPP

#include <libmaus2/lf/ImpCompactHuffmanWaveletLF.hpp>
#include <libmaus2/fm/SampledSA.hpp>
#include <libmaus2/fm/SampledISA.hpp>
#include <libmaus2/lcp/SuccinctLCP.hpp>
#include <libmaus2/rmq/RMMTree.hpp>
#include <libmaus2/parallel/OMPNumThreadsScope.hpp>

namespace libmaus2
{
	namespace suffixtree
	{
		struct CompressedSuffixTree
		{
			typedef CompressedSuffixTree this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			typedef libmaus2::lf::ImpCompactHuffmanWaveletLF lf_type;
			typedef lf_type::unique_ptr_type lf_ptr_type;
			typedef libmaus2::fm::SimpleSampledSA<lf_type> ssa_type;
			typedef ssa_type::unique_ptr_type ssa_ptr_type;
			typedef libmaus2::fm::SampledISA<lf_type> sisa_type;
			typedef sisa_type::unique_ptr_type sisa_ptr_type;
			typedef libmaus2::lcp::SuccinctLCP<lf_type,ssa_type,sisa_type> lcp_type;
			typedef lcp_type::unique_ptr_type lcp_ptr_type;
			typedef libmaus2::rmq::RMMTree<lcp_type,3> rmm_tree_type;
			typedef rmm_tree_type::unique_ptr_type rmm_tree_ptr_type;

			// compressed LF/phi
			lf_ptr_type LF;
			// sampled suffix array
			ssa_ptr_type SSA;
			// sampled inverse suffix array
			sisa_ptr_type SISA;
			// compressed LCP array
			lcp_ptr_type LCP;
			// RMM tree for rmq/psv/nsv
			rmm_tree_ptr_type RMM;
			// length of text
			uint64_t n;

			CompressedSuffixTree(
				lf_ptr_type & rLF,
				ssa_ptr_type & rSSA,
				sisa_ptr_type & rSISA,
				lcp_ptr_type & rLCP,
				rmm_tree_ptr_type & rRMM
			)
			: LF(UNIQUE_PTR_MOVE(rLF)), SSA(UNIQUE_PTR_MOVE(rSSA)), SISA(UNIQUE_PTR_MOVE(rSISA)), LCP(UNIQUE_PTR_MOVE(rLCP)), RMM(UNIQUE_PTR_MOVE(rRMM)),
			  n(LF->getN())
			{}

			uint64_t getSigma() const
			{
				return LF->getSymbols().size();
			}

			uint64_t byteSize() const
			{
				return
					LF->byteSize() +
					SSA->byteSize() +
					SISA->byteSize() +
					LCP->byteSize() +
					RMM->byteSize() +
					sizeof(uint64_t);
			}

			// suffix tree node (interval [sp,ep) on suffix array)
			struct Node
			{
				uint64_t sp;
				uint64_t ep;

				Node() : sp(0), ep(0) {}
				Node(uint64_t const rsp, uint64_t const rep) : sp(rsp), ep(rep) {}

				bool operator==(Node const & N) const
				{
					return N.sp == sp && N.ep == ep;
				}

				bool operator!=(Node const & N) const
				{
					return !(*this==N);
				}

				bool isLeaf() const
				{
					return ep-sp == 1;
				}

				uint64_t size() const
				{
					return (ep > sp) ? (ep-sp) : 0;
				}
			};

			template<typename iterator>
			void extractText(iterator const it, uint64_t const low, uint64_t const high) const
			{
				uint64_t r = (*SISA)[ high % n ];
				// uint64_t c = high-low;
				iterator ite = it + (high-low);

				while ( ite != it )
				{
					std::pair<int64_t,uint64_t> const P = LF->extendedLF(r);
					*(--ite) = P.first;
					r = P.second;
				}
			}

			template<typename data_type>
			uint64_t extractText(libmaus2::autoarray::AutoArray<data_type> & D) const
			{
				if ( D.size() < n )
					D.resize(n);
				extractText(D.begin(),0,n);

				return n;
			}

			template<typename data_type>
			uint64_t extractTextParallel(libmaus2::autoarray::AutoArray<data_type> & D, uint64_t const numthreads) const
			{
				if ( D.size() < n )
					D.resize(n);

				uint64_t const blocksize = (n + numthreads-1)/numthreads;
				uint64_t const numblocks = (n + blocksize-1)/blocksize;

				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(numthreads)
				#endif
				for ( uint64_t t = 0; t < numblocks; ++t )
				{
					uint64_t const low = t * blocksize;
					uint64_t const high = std::min(low+blocksize,n);
					extractText(D.begin()+low,low,high);
				}

				return n;
			}

			/*
			 * string depth
			 */
			uint64_t sdepth(Node const & node) const
			{
				if ( node.ep - node.sp > 1 )
					return (*LCP)[RMM->rmq(node.sp+1,node.ep-1)];
				#if 0
				else if ( node.ep-node.sp == 1 )
					return n-(*SSA)[node.sp];
				#endif
				else
					return std::numeric_limits<uint64_t>::max();
			}

			std::ostream & printNodeContext(std::ostream & out, Node const & N, uint64_t const n) const
			{
				assert ( sdepth(N) >= n );

				std::map < int64_t, uint64_t > M = LF->getW().enumerateSymbolsInRange(N.sp,N.ep);
				out << "(";
				for ( std::map < int64_t, uint64_t >::const_iterator ita = M.begin(); ita != M.end(); ++ita )
					out << ita->first;
				out << ")";

				for ( uint64_t j = 0; j < n; ++j )
					out << letter(N.sp,j);

				out << "(";
				if ( sdepth(N) == n )
				{
					Node node = firstChild(N);
					while ( node.ep != node.sp )
					{
						out << letter(node.sp,n);
						node = nextSibling(node);
					}
				}
				else
				{
					assert ( sdepth(N) > n );
					out << letter(N.sp,n);
				}
				out << ")";

				return out;
			}

			/**
			 * letter at position SA[r]+i
			 **/
			int64_t letter(uint64_t const r, uint64_t const i) const
			{
				return (*LF)[(*SISA) [ ((*SSA)[r] + i+1) % n ]];
			}

			/*
			 * tree root
			 */
			Node root() const
			{
				return Node(0,n);
			}

			/*
			 * append character on the left
			 */
			Node backwardExtend(Node const & node, int64_t const sym) const
			{
				std::pair<uint64_t,uint64_t> p = LF->step(sym,node.sp,node.ep);
				return Node(p.first,p.second);
			}

			/*
			 * number of suffixes/leafs under node
			 */
			uint64_t count(Node const & node) const
			{
				return node.ep-node.sp;
			}

			/*
			 * parent node for node
			 */
			Node parent(Node const & node) const
			{
				// index on lcp with string depth of parent
				uint64_t const k = (node.ep == n) ? node.sp : (((*LCP)[node.sp] > (*LCP)[node.ep]) ? node.sp : node.ep);
				return Node(RMM->psvz(k),RMM->nsv(k));
			}

			/*
			 * enumerate children
			 */
			template<typename iterator>
			uint64_t enumerateChildren(Node const & par, iterator it) const
			{
				Node node = firstChild(par);
				iterator const ita = it;

				while ( node.ep-node.sp )
				{
					*(it++)	= node;

					if ( node.ep == par.ep )
						break;

					// next one
					node = Node(node.ep, RMM->nsv(node.ep,(*LCP)[node.ep]+1));
				}

				return it-ita;
			}

			/*
			 * enumerate children
			 */
			uint64_t enumerateChildren(Node const & par, libmaus2::autoarray::AutoArray<Node> & A) const
			{
				Node node = firstChild(par);
				uint64_t o = 0;

				while ( node.ep - node.sp )
				{
					A.push(o,node);

					if ( node.ep == par.ep )
						break;

					node = Node(node.ep, RMM->nsv(node.ep,(*LCP)[node.ep]+1));
				}

				return o;
			}

			/*
			 * first child of node (or Node(0,0) if none)
			 */
			Node firstChild(Node const & node) const
			{
				if ( node.ep-node.sp > 1 )
					return Node(node.sp,RMM->rmq(node.sp+1,node.ep-1));
				else
					return Node(0,0);
			}

			/*
			 * next sibling of node (or Node(0,0) if none)
			 */
			Node nextSibling(Node const & node) const
			{
				if ( node.ep-node.sp > 0 )
				{
					Node const par = parent(node);

					// is node the last child?
					if ( node.ep == par.ep )
						return Node(0,0);

					return Node(node.ep, RMM->nsv(node.ep,(*LCP)[node.ep]+1));
				}
				else
				{
					return Node(0,0);
				}
			}

			// comparator for suffix tree edges
			struct ChildSearchAccessor
			{
				typedef ChildSearchAccessor this_type;

				CompressedSuffixTree const & CST;
				std::vector < Node > const & children;
				uint64_t const sl;

				typedef libmaus2::util::ConstIterator<this_type,int64_t> const_iterator;

				const_iterator begin() const
				{
					return const_iterator(this,0);
				}
				const_iterator end() const
				{
					return const_iterator(this,children.size());
				}
				ChildSearchAccessor(CompressedSuffixTree const & rCST, std::vector < Node > const & rchildren, uint64_t const rsl)
				: CST(rCST), children(rchildren), sl(rsl)
				{

				}

				int64_t get(uint64_t const i) const
				{
					return CST.letter(children[i].sp,sl);
				}

				int64_t operator[](uint64_t const i) const
				{
					return get(i);
				}
			};

			/**
			 * follow edge starting by letter a from node and return obtained node/leaf
			 * or Node(0,0) if there is no such edge
			 **/
			Node child(Node const & node, int64_t const a)
			{
				// string depth of input node
				uint64_t const sl = sdepth(node);

				// extract all children
				std::vector < Node > children;
				Node chi = firstChild(node);
				while ( chi.sp != chi.ep )
				{
					children.push_back(chi);
					chi = nextSibling(chi);
				}

				// find queried letter by binary search on list of children
				ChildSearchAccessor CSA(*this,children,sl);
				ChildSearchAccessor::const_iterator const it = std::lower_bound(CSA.begin(),CSA.end(),a);

				if ( it != CSA.end() && *it == a )
					return children[it-CSA.begin()];
				else
					return Node(0,0);
			}

			/**
			 * suffix link
			 **/
			Node slink(Node const & node) const
			{
				uint64_t const cnt = count(node);

				if ( cnt == n )
				{
					return Node(0,0);
				}
				else if ( cnt > 1 )
				{
					uint64_t const phisp = LF->phi(node.sp);
					uint64_t const phiep = LF->phi(node.ep-1);
					uint64_t const k = RMM->rmq(phisp+1,phiep);
					return Node(RMM->psvz(k),RMM->nsv(k));
				}
				else if ( cnt == 1 )
				{
					uint64_t const phisp = LF->phi(node.sp);
					return Node(phisp,phisp+1);
				}
				else
				{
					return Node(0,0);
				}
			}

			/*
			 * constructor from single files
			 */
			CompressedSuffixTree(
				std::string const & hwtname,
				std::string const & saname,
				std::string const & isaname,
				std::string const & lcpname,
				std::string const & rmmname,
				uint64_t const numthreads
			)
			: LF(lf_type::load(hwtname,numthreads)),
			  SSA(ssa_type::load(LF.get(),saname)),
			  SISA(sisa_type::load(LF.get(),isaname)),
			  LCP(lcp_type::load(*SSA,lcpname)),
			  RMM(rmm_tree_type::load(*LCP,rmmname)),
			  n(LF->getN())
			{
			}

			/**
			 * constructor from serialised object
			 **/
			template<typename stream_type>
			CompressedSuffixTree(stream_type & in)
			:
				LF(new lf_type(in)),
				SSA(new ssa_type(LF.get(),in)),
				SISA(new sisa_type(LF.get(),in)),
				LCP(new lcp_type(in,*SSA)),
				RMM(new rmm_tree_type(in,*LCP)),
				n(LF->getN())
			{

			}

			/**
			 * serialise object to stream
			 **/
			template<typename stream_type>
			void serialise(stream_type & out) const
			{
				LF->W->serialise(out);
				SSA->serialize(out);
				SISA->serialize(out);
				LCP->serialize(out);
				RMM->serialise(out);
			}
		};
	}
}

std::ostream & operator<<(std::ostream & out, libmaus2::suffixtree::CompressedSuffixTree::Node const & node);
#endif
