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

#include <libmaus2/util/ArgInfo.hpp>
#include <libmaus2/suffixtree/CompressedSuffixTree.hpp>

#include <libmaus2/eta/LinearETA.hpp>
#include <libmaus2/parallel/SynchronousCounter.hpp>

uint64_t countLeafsByTraversal(libmaus2::suffixtree::CompressedSuffixTree const & CST)
{
	libmaus2::eta::LinearETA eta(CST.n);

	typedef libmaus2::suffixtree::CompressedSuffixTree::Node Node;
	libmaus2::parallel::SynchronousCounter<uint64_t> leafs = 0;
	std::stack< std::pair<Node,uint64_t> > S; S.push(std::pair<Node,uint64_t>(CST.root(),0));
	uint64_t const Sigma = CST.getSigma();
	libmaus2::autoarray::AutoArray<Node> children(Sigma,false);
	
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
	
	libmaus2::parallel::OMPLock lock;
	
	#if defined(_OPENMP)
	#pragma omp parallel
	#endif
	while ( Q.size() )
	{
		libmaus2::autoarray::AutoArray<Node> lchildren(Sigma,false);
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

#include <libmaus2/fastx/acgtnMap.hpp>

libmaus2::uint128_t rc(libmaus2::uint128_t v, uint64_t const k)
{
	libmaus2::uint128_t w = 0;
	
	for ( uint64_t i = 0; i < k; ++i )
	{
		w <<= 2;
		w |= libmaus2::fastx::invertN(v & 3);
		v >>= 2;
	}
	
	return w;
}

#include <libmaus2/fastx/SingleWordDNABitBuffer.hpp>

std::string decode(std::string s)
{
	for ( uint64_t i = 0; i < s.size(); ++i )
		if ( s[i] == 0 )
			s[i] = 'l';
		else
			s[i] = libmaus2::fastx::remapChar(s[i]-1);
	return s;
}

void enumerateMulitpleKMers(libmaus2::suffixtree::CompressedSuffixTree const & CST, uint64_t const k)
{
	libmaus2::autoarray::AutoArray<char> text;
	uint64_t const textn = CST.extractTextParallel(text);

	libmaus2::suffixtree::CompressedSuffixTree::lcp_type const & LCP = *(CST.LCP);
	std::vector<libmaus2::uint128_t> words;
	
	uint64_t const numthreads = libmaus2::parallel::OMPNumThreadsScope::getMaxThreads();
	uint64_t const tnumblocks = 32*numthreads;
        uint64_t const blocksize = (CST.n + tnumblocks-1)/tnumblocks;
        uint64_t blow = 0;

        std::cerr << "Decoding LCP array...";
	libmaus2::autoarray::AutoArray<uint8_t> ulcp(CST.n);
	#if defined(_OPENMP)
	#pragma omp parallel for schedule(dynamic,1)
	#endif
	for ( uint64_t i = 0; i < CST.n; ++i )
	{
		uint64_t const u = LCP[i];
		if ( u >= std::numeric_limits<uint8_t>::max() )
			ulcp[i] = std::numeric_limits<uint8_t>::max();
		else
			ulcp[i] = u;
	}
	std::cerr << "done." << std::endl;
  
  	std::cerr << "computing split points";
        std::vector<uint64_t> splitpoints;
        
        while ( blow < CST.n )
        {
        	std::cerr.put('.');
        	assert ( ulcp[blow] < k );
        	
        	splitpoints.push_back(blow);
        	
        	blow += std::min(blocksize,CST.n-blow);
        	while ( blow < CST.n && ulcp[blow] >= k )
        	{
        		#if 0
        		uint64_t const pos = (*(CST.SSA))[blow];
        		std::cerr << decode(std::string(text.begin()+pos,text.begin()+pos+k)) << std::endl;
        		std::cerr << ulcp[blow] << std::endl;
        		#endif
        		
        		++blow;
		}
        }
        
        splitpoints.push_back(CST.n);
        std::cerr << "done." << std::endl;
        
        typedef libmaus2::uint128_t word_type;
        typedef std::vector < word_type > word_vector_type;
        typedef libmaus2::util::shared_ptr<word_vector_type>::type word_vector_ptr_type;
        
        std::vector < word_vector_ptr_type > wordvecs(splitpoints.size()-1);
  
        std::cerr << "computing words...";      
        #if defined(_OPENMP)
        #pragma omp parallel for schedule(dynamic,1)
        #endif
        for ( uint64_t t = 0; t < splitpoints.size()-1; ++t )
        {
        	word_vector_ptr_type pwords(new word_vector_type);
        	wordvecs[t] = pwords;
        	std::vector<libmaus2::uint128_t> & lwords = *pwords;
        
		uint64_t low = splitpoints[t];

		while ( low < splitpoints[t+1] )
		{
			assert ( ulcp[low] < k );
	
			uint64_t high = low+1;
		
			while ( high < splitpoints[t+1] && ulcp[high] >= k )
				++high;
			
			uint64_t const pos = (*(CST.SSA))[low];
			if ( pos + k <= textn )
			{
				char const * t_f = text.begin()+pos;
				bool ok = true;
				for ( uint64_t i = 0; i < k; ++i )
					if ( t_f[i] < 1 || t_f[i] > 4 )
				 		ok = false;			
			
				if ( ok )
				{
					libmaus2::uint128_t wf = 0;
				
					for ( uint64_t i = 0; i < k; ++i )
					{
						wf <<= 2;					
						wf |= t_f[i]-1;
					}
				
					lwords.push_back((wf << 32)|(high-low));				
				}
			}
						
			low = high;
		}
		std::cerr.put('.');
        }
        std::cerr << "done." << std::endl;

        word_vector_type wv;
        for ( uint64_t j = 0; j < wordvecs.size(); ++j )
        {
        	word_vector_type const & lwv = *(wordvecs[j]);
        	for ( uint64_t i = 0; i < lwv.size(); ++i )
        		wv.push_back(lwv[i]);
		wordvecs[j] = word_vector_ptr_type();
        }
        std::sort(wv.begin(),wv.end());
        
        uint64_t erased = 0;
        for ( uint64_t i = 0; i < wv.size(); ++i )
        {
        	libmaus2::uint128_t const w = (wv[i] >> 32);
		libmaus2::uint128_t const r = rc(w,k);

        	#if 0
        	libmaus2::fastx::SingleWordDNABitBuffer SWDB(k);
        	SWDB.buffer = w;
        	std::cerr << SWDB.toString() << std::endl;
        	SWDB.buffer = r;
        	std::cerr << SWDB.toString() << std::endl;
        	#endif
        	
        	if ( w == r )
        	{
        	
        	}
        	else if ( w < r )
        	{	
        		word_vector_type::iterator it = std::lower_bound(wv.begin(),wv.end(),r << 32);
        		
        		#if 0
        		if ( it != wv.end() )
        		{
		        	libmaus2::fastx::SingleWordDNABitBuffer SWDB(k);
				SWDB.buffer = r;
		        	std::cerr << "word " << SWDB.toString() << std::endl;
		        	SWDB.buffer = (*it)>>32;
		        	std::cerr << "closest " << SWDB.toString() << std::endl;
        		}
        		#endif
	        	
	        	if ( it != wv.end() && (*it>>32) == r )
	        	{
	        		wv[i] =
	        			((wv[i] >> 32) << 32)
	        			|
	        			(
	        				(wv[i] & 0xFFFFFFFFUL)
	        				+
	        				(*it   & 0xFFFFFFFFUL)
					);
				*it = (*it>>32)<<32;
				erased += 1;
	        	}
        	}
        }

        uint64_t o = 0;
        for ( uint64_t i = 0; i < wv.size(); ++i )
        	if ( wv[i] & 0xFFFFFFFFUL )
        		wv[o++] = wv[i];
	wv.resize(o);
	
	libmaus2::util::Histogram kmerhist;
        for ( uint64_t i = 0; i < wv.size(); ++i )
        	kmerhist ( wv[i] & 0xFFFFFFFFUL );
        	
	kmerhist.print(std::cout);
	
	// wv now contains the list of kmers with frequencies
}

int main(int argc, char * argv[])
{
	try
	{
		libmaus2::util::ArgInfo const arginfo(argc,argv);
		std::string const prefix = arginfo.getRestArg<std::string>(0);
		std::string const hwtname = prefix+".hwt";
		std::string const saname = prefix+".sa";
		std::string const isaname = prefix+".isa";
		std::string const lcpname = prefix+".lcp";
		std::string const rmmname = prefix+".rmm";
		std::cerr << "Loading suffix tree...";
		libmaus2::suffixtree::CompressedSuffixTree CST(hwtname,saname,isaname,lcpname,rmmname);
		std::cerr << "done." << std::endl;
		
		enumerateMulitpleKMers(CST,20);
		
		#if 0
		libmaus2::autoarray::AutoArray<char> text;
		uint64_t const textn = CST.extractTextParallel(text);
		
		std::cerr << "textn=" << textn << std::endl;
		
		for ( uint64_t i = 0; i < textn; ++i )
			if ( text[i] > 0 )
				text[i] = libmaus2::fastx::remapChar(text[i]-1);
			else
				text[i] = '\n';
		std::cout.write(text.begin(),textn);
		#endif
	
		uint64_t const leafs = countLeafsByTraversal(CST);
		
		assert ( CST.n == leafs );
			
		#if 0
		#if 0
		// serialise cst and read it back	
		std::ostringstream ostr;
		CST.serialise(ostr);
		std::istringstream istr(ostr.str());
		libmaus2::suffixtree::CompressedSuffixTree rCST(istr);
		#endif
		
		std::cerr << "[0] = " << CST.backwardExtend(CST.root(),0) << std::endl;
		std::cerr << "[1] = " << CST.backwardExtend(CST.root(),1) << std::endl;
		std::cerr << "[2] = " << CST.backwardExtend(CST.root(),2) << std::endl;
		std::cerr << "[3] = " << CST.backwardExtend(CST.root(),3) << std::endl;
		std::cerr << "[4] = " << CST.backwardExtend(CST.root(),4) << std::endl;
		std::cerr << "[5] = " << CST.backwardExtend(CST.root(),5) << std::endl;
		
		typedef libmaus2::suffixtree::CompressedSuffixTree::Node Node;
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
