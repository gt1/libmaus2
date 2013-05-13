/**
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
**/

#include <libmaus/util/ArgInfo.hpp>
#include <libmaus/lf/LF.hpp>
#include <libmaus/fm/SampledSA.hpp>
#include <libmaus/fm/SampledISA.hpp>
#include <libmaus/lcp/LCP.hpp>
#include <libmaus/rmq/RMMTree.hpp>

namespace libmaus
{
	namespace suffixtree
	{
		struct CompressedSuffixTree
		{
			typedef libmaus::lf::ImpHuffmanWaveletLF lf_type;
			typedef lf_type::unique_ptr_type lf_ptr_type;
			typedef libmaus::fm::SimpleSampledSA<lf_type> ssa_type;
			typedef ssa_type::unique_ptr_type ssa_ptr_type;
			typedef libmaus::fm::SampledISA<lf_type> sisa_type;
			typedef sisa_type::unique_ptr_type sisa_ptr_type;
			typedef libmaus::lcp::SuccinctLCP<lf_type,ssa_type,sisa_type> lcp_type;
			typedef lcp_type::unique_ptr_type lcp_ptr_type;
			typedef libmaus::rmq::RMMTree<lcp_type,3> rmm_tree_type;
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
			// length of sequence
			uint64_t n;
			
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
			};
			
			/*
			 * string depth
			 */
			uint64_t sdepth(Node const & node) const
			{
				if ( node.ep - node.sp > 1 )
					return (*LCP)[RMM->rmq(node.sp+1,node.ep-1)];
				else if ( node.ep-node.sp == 1 )
					return n-(*SSA)[node.sp];
				else
					return std::numeric_limits<uint64_t>::max();
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
				
				typedef libmaus::util::ConstIterator<this_type,int64_t> const_iterator;
				
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
				std::string const & rmmname
			)
			: LF(lf_type::load(hwtname)), 
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

std::ostream & operator<<(std::ostream & out, libmaus::suffixtree::CompressedSuffixTree::Node const & node);

std::ostream & operator<<(std::ostream & out, libmaus::suffixtree::CompressedSuffixTree::Node const & node)
{
	return (out << "[" << node.sp << "," << node.ep << ")");
}

int main(int argc, char * argv[])
{
	try
	{
		libmaus::util::ArgInfo const arginfo(argc,argv);
		std::string const prefix = arginfo.getRestArg<std::string>(0);
		std::string const hwtname = prefix+".hwt";
		std::string const saname = prefix+".sa";
		std::string const isaname = prefix+".isa";
		std::string const lcpname = prefix+".lcp";
		std::string const rmmname = prefix+".rmm";
		libmaus::suffixtree::CompressedSuffixTree CST(hwtname,saname,isaname,lcpname,rmmname);
	
		#if 0
		// serialise cst and read it back	
		std::ostringstream ostr;
		CST.serialise(ostr);
		std::istringstream istr(ostr.str());
		libmaus::suffixtree::CompressedSuffixTree rCST(istr);
		#endif
		
		std::cerr << "[0] = " << CST.backwardExtend(CST.root(),0) << std::endl;
		std::cerr << "[1] = " << CST.backwardExtend(CST.root(),1) << std::endl;
		std::cerr << "[2] = " << CST.backwardExtend(CST.root(),2) << std::endl;
		std::cerr << "[3] = " << CST.backwardExtend(CST.root(),3) << std::endl;
		std::cerr << "[4] = " << CST.backwardExtend(CST.root(),4) << std::endl;
		std::cerr << "[5] = " << CST.backwardExtend(CST.root(),5) << std::endl;
		
		typedef libmaus::suffixtree::CompressedSuffixTree::Node Node;
		Node node = CST.root();
		std::cerr << CST.parent(CST.firstChild(CST.root())) << std::endl;
		std::cerr << "parent("<<node <<")="<< CST.parent(node) << " sdepth=" << CST.sdepth(node) << " firstChild=" << CST.firstChild(node) << " next=" << CST.nextSibling(CST.firstChild(node)) << std::endl;
		node = CST.backwardExtend(node,1);
		std::cerr << "parent("<<node <<")="<< CST.parent(node) << " sdepth=" << CST.sdepth(node) << " slink=" << CST.slink(node) << std::endl;
		node = CST.backwardExtend(node,1);
		std::cerr << "parent("<<node <<")="<< CST.parent(node) << " sdepth=" << CST.sdepth(node) << " slink=" << CST.slink(node) << std::endl;
		node = CST.backwardExtend(node,1);
		std::cerr << "parent("<<node <<")="<< CST.parent(node) << " sdepth=" << CST.sdepth(node) << " slink=" << CST.slink(node) << std::endl;
		
		std::cerr << "LCP[0]=" << (*(CST.LCP))[0] << std::endl;
		std::cerr << "LCP[1]=" << (*(CST.LCP))[1] << std::endl;
		std::cerr << "LCP[2]=" << (*(CST.LCP))[2] << std::endl;
		std::cerr << "LCP[3]=" << (*(CST.LCP))[3] << std::endl;
		
		node = CST.root();
		std::cerr << "root=" << node << std::endl;
		node = CST.firstChild(node);
		while ( node.sp != node.ep )
		{
			std::cerr << "child " << node << std::endl;
			node = CST.nextSibling(node);
		}

		for ( uint64_t i = 0; i < 7; ++i )
			std::cerr << "["<<i<<"]=" << CST.child(CST.root(),i) << std::endl;
			
		std::cerr << CST.child(CST.child(CST.root(),1),1) << std::endl;
		
		std::cerr << "byteSize=" << CST.byteSize() << std::endl;
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
	}
}
