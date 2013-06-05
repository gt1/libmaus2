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

#include <libmaus/util/ArgInfo.hpp>
#include <libmaus/suffixtree/CompressedSuffixTree.hpp>

#include <libmaus/eta/LinearETA.hpp>
#include <libmaus/parallel/SynchronousCounter.hpp>

uint64_t countLeafsByTraversal(libmaus::suffixtree::CompressedSuffixTree const & CST)
{
	libmaus::eta::LinearETA eta(CST.n);

	typedef libmaus::suffixtree::CompressedSuffixTree::Node Node;
	libmaus::parallel::SynchronousCounter<uint64_t> leafs = 0;
	std::stack< std::pair<Node,uint64_t> > S; S.push(std::pair<Node,uint64_t>(CST.root(),0));
	uint64_t const Sigma = CST.getSigma();
	libmaus::autoarray::AutoArray<Node> children(Sigma,false);
	
	std::deque < Node > Q;
	uint64_t const frac = 128;
	uint64_t const maxtdepth = 64;
	
	while ( S.size() )
	{
		std::pair<Node,uint64_t> const P = S.top(); S.pop();
		Node const & parent = P.first;
		uint64_t const tdepth = P.second;
		
		if ( CST.count(parent) < 2 || CST.count(parent) <= CST.n/frac || tdepth >= maxtdepth )
		{
			assert ( parent.ep-parent.sp );
			Q.push_back(parent);
		}
		else
		{
			uint64_t const numc = CST.enumerateChildren(parent,children.begin());
			
			for ( uint64_t i = 0; i < numc; ++i )
				S.push(std::pair<Node,uint64_t>(children[numc-i-1],tdepth+1));
		}
	}
	
	libmaus::parallel::OMPLock lock;
	
	#if defined(_OPENMP)
	#pragma omp parallel
	#endif
	while ( Q.size() )
	{
		libmaus::autoarray::AutoArray<Node> lchildren(Sigma,false);
		Node node(0,0);
		
		lock.lock();
		if ( Q.size() )
		{
			node = Q.front();
			Q.pop_front();
		}
		lock.unlock();
		
		std::stack<Node> SL;
		if ( node.ep-node.sp )
			SL.push(node);

		while ( SL.size() )
		{
			Node const parent = SL.top(); SL.pop();
			
			if ( parent.ep-parent.sp > 1 )
			{
				uint64_t const numc = CST.enumerateChildren(parent,lchildren.begin());
			
				for ( uint64_t i = 0; i < numc; ++i )
					SL.push(lchildren[numc-i-1]);
			}
			else
			{
				if ( (leafs++ % (32*1024)) == 0 )
				{
					lock.lock();
					std::cerr << static_cast<double>(leafs)/CST.n << "\t" << eta.eta(leafs) << std::endl;	
					lock.unlock();
				}
			}
		}
	}

	std::cerr << "Q.size()=" << Q.size() << " leafs=" << leafs << " n=" << CST.n << std::endl;

	return leafs;
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
		std::cerr << "Loading suffix tree...";
		libmaus::suffixtree::CompressedSuffixTree CST(hwtname,saname,isaname,lcpname,rmmname);
		std::cerr << "done." << std::endl;
	
		uint64_t const leafs = countLeafsByTraversal(CST);
		
		assert ( CST.n == leafs );
			
		#if 0
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
		#endif
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
	}
}
