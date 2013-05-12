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

	lf_ptr_type LF;
	ssa_ptr_type SSA;
	sisa_ptr_type SISA;
	lcp_ptr_type LCP;
	rmm_tree_ptr_type RMM;
	
	struct Node
	{
		uint64_t sp;
		uint64_t ep;
		
		Node() : sp(0), ep(0) {}
		Node(uint64_t const rsp, uint64_t const rep) : sp(rsp), ep(rep) {}
	};
	
	Node root() const
	{
		return Node(0,LF->getN());
	}
	
	Node backwardExtend(Node const & node, int64_t const sym) const
	{
		std::pair<uint64_t,uint64_t> p = LF->step(sym,node.sp,node.ep);
		return Node(p.first,p.second);
	}
	
	uint64_t count(Node const & node) const
	{
		return node.ep-node.sp;
	}
	
	Node parent(Node const & node) const
	{
		uint64_t const k = 
			(node.ep == LF->getN()) ? node.sp : (((*LCP)[node.sp] > (*LCP)[node.ep]) ? node.sp : node.ep);
		return Node(RMM->psvz(k),RMM->nsv(k));
	}
	
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
	  RMM(rmm_tree_type::load(*LCP,rmmname))
	{
	}
};

std::ostream & operator<<(std::ostream & out, CompressedSuffixTree::Node const & node)
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
		CompressedSuffixTree CST(hwtname,saname,isaname,lcpname,rmmname);
		
		typedef CompressedSuffixTree::Node Node;
		Node node = CST.root();
		std::cerr << "parent("<<node <<")="<< CST.parent(node) << std::endl;
		node = CST.backwardExtend(node,1);
		std::cerr << "parent("<<node <<")="<< CST.parent(node) << std::endl;
		node = CST.backwardExtend(node,1);
		std::cerr << "parent("<<node <<")="<< CST.parent(node) << std::endl;
		node = CST.backwardExtend(node,1);
		std::cerr << "parent("<<node <<")="<< CST.parent(node) << std::endl;
		
		std::cerr << "LCP[0]=" << (*(CST.LCP))[0] << std::endl;
		std::cerr << "LCP[1]=" << (*(CST.LCP))[1] << std::endl;
		std::cerr << "LCP[2]=" << (*(CST.LCP))[2] << std::endl;
		std::cerr << "LCP[3]=" << (*(CST.LCP))[3] << std::endl;
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
	}
}
