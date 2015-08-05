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

#include <libmaus2/wavelet/DynamicWaveletTree.hpp>
#include <libmaus2/cumfreq/SlowCumFreq.hpp>
#include <libmaus2/huffman/huffman.hpp>
#include <libmaus2/eta/LinearETA.hpp>
#include <libmaus2/math/bitsPerNum.hpp>

#include <fstream>
#include <vector>
#include <set>

uint64_t getFileLength(std::string const & textfilename)
{
	std::ifstream istr(textfilename.c_str(),std::ios::binary);
	
	if ( ! istr.is_open() )
		throw std::runtime_error("Failed to open file.");
	
	istr.seekg(0,std::ios_base::end);
	uint64_t const n = istr.tellg();

	return n;
}

::libmaus2::autoarray::AutoArray<uint64_t> getSymbolFrequencies(std::string const & textfilename)
{
	uint64_t const n = getFileLength(textfilename);

	std::ifstream istr(textfilename.c_str(),std::ios::binary);
	
	if ( ! istr.is_open() )
		throw std::runtime_error("Failed to open file.");

	::libmaus2::autoarray::AutoArray<uint64_t> F(256);
	for ( unsigned int i = 0; i < 256; ++i )
		F[i] = 0;

	uint64_t const s = 1024*1024;
	::libmaus2::autoarray::AutoArray<uint8_t> buf(s,false);

	for ( uint64_t j = 0; j < (n + (s-1))/s; ++j )
	{
		uint64_t const a = j*s;
		uint64_t const b = std::min( a+s , n );
		uint64_t const c = b-a;
		
		istr.read ( reinterpret_cast<char *>(buf.get()), c );
		
		for ( uint64_t i = 0; i < c; ++i )
			F[ buf[i] ] ++;
	}
	
	assert ( static_cast<int64_t>(istr.tellg()) == static_cast<int64_t>(n) );
		
	return F;
}

void reorderHuffmanTreeZero(::libmaus2::huffman::HuffmanTreeNode * hnode)
{

	std::map < ::libmaus2::huffman::HuffmanTreeNode *, ::libmaus2::huffman::HuffmanTreeInnerNode * > hparentmap;
	hnode->fillParentMap(hparentmap);
	std::map < int64_t, ::libmaus2::huffman::HuffmanTreeLeaf * > hleafmap;
	hnode->fillLeafMap(hleafmap);

	::libmaus2::huffman::HuffmanTreeNode * hcur = hleafmap.find(-1)->second;
	
	while ( hparentmap.find(hcur) != hparentmap.end() )
	{
		::libmaus2::huffman::HuffmanTreeInnerNode * hparent = hparentmap.find(hcur)->second;
		
		if ( hcur == hparent->right )
			std::swap(hparent->left, hparent->right);
		
		hcur = hparent;
	}
}

void applyRankMap(::libmaus2::huffman::HuffmanTreeNode * hnode, std::map<int64_t,uint64_t> const & rankmap)
{
	std::map < int64_t, ::libmaus2::huffman::HuffmanTreeLeaf * > hleafmap;
	hnode->fillLeafMap(hleafmap);

	for ( std::map < int64_t, ::libmaus2::huffman::HuffmanTreeLeaf * >::iterator ita = hleafmap.begin(); ita != hleafmap.end(); ++ita )
	{
		int const srcsym = ita->first;
		uint64_t const dstsym = rankmap.find(srcsym)->second;
		assert ( ita->second->symbol == srcsym );
		ita->second->symbol = dstsym;
	}
}

void printCodeLength( ::libmaus2::huffman::HuffmanTreeNode const * const hnode, uint64_t const * const F)
{
	::libmaus2::huffman::EncodeTable<4> enctable(hnode);
	
	uint64_t l = 0;
	uint64_t n = 0;
	for ( uint64_t i = 0; i < 256; ++i )
		if ( F[i] )
		{
			l += F[i] * enctable[i].second;
			n += F[i];
		}
			
	std::cerr << "Total code length " << l << " average bits per symbol " << static_cast<double>(l) / n << std::endl;
}

void computeBWT(std::string const & textfilename, std::ostream & output)
{
	unsigned int const k = 8;
	unsigned int const w = 32;
	
	uint64_t const n = getFileLength(textfilename);
	if ( ! n )
		return;

	::libmaus2::autoarray::AutoArray<uint64_t> F = getSymbolFrequencies(textfilename);
		
	std::map<int64_t,uint64_t> freq;
	for ( unsigned int i = 0; i < 256; ++i )
		if ( F[i] )
			freq[i] = F[i];
	freq[-1] = 1;

	double ent = 0;
	for ( unsigned int i = 0; i < 256; ++i )
		if ( F[i] )
		{
			ent -= F[i] * (log(static_cast<double>(F[i])/n)/log(2));
		}
	std::cerr << "Entropy of text is " << ent/n << std::endl;

	::libmaus2::util::shared_ptr < ::libmaus2::huffman::HuffmanTreeNode >::type ahnode = ::libmaus2::huffman::HuffmanBase::createTree( freq );
	
	printCodeLength(ahnode.get(), F.get());
	
	reorderHuffmanTreeZero(ahnode.get());

	std::map<int64_t,uint64_t> rankmap = ::libmaus2::huffman::EncodeTable<4>(ahnode.get()).symsOrderedByCodeMap();
	
	applyRankMap(ahnode.get(), rankmap);
	
	::libmaus2::autoarray::AutoArray<uint64_t> M(256);
	::libmaus2::autoarray::AutoArray<uint64_t> R(257);
	uint64_t const a = rankmap.size();
	for ( std::map<int64_t,uint64_t>::const_iterator ita = rankmap.begin(); ita != rankmap.end(); ++ita )
	{
		// std::cerr << ita->first << " " << ita->second << std::endl;
		if ( ita->first >= 0 )
		{
			M[ita->first] = ita->second;
			R[ita->second] = ita->first;
		}
	}

	unsigned int const bpn = ::libmaus2::math::bitsPerNum(a-1);

	::libmaus2::wavelet::DynamicWaveletTree<k,w> B(bpn);
	::libmaus2::cumfreq::SlowCumFreq scf(a);
	
	uint64_t const s = 1024*1024;
	::libmaus2::autoarray::AutoArray<uint8_t> buf(s,false);
	uint64_t const numblocks = (n + (s-1))/s;

	std::ifstream istr(textfilename.c_str(),std::ios::binary);
	
	if ( ! istr.is_open() )
	{
		std::cerr << "Failed to open file " << textfilename << std::endl;
		throw std::runtime_error("Failed to open file.");
	}

	istr.seekg ( 0 , std::ios_base::end );
	
	uint64_t p = 0;
	for ( uint64_t b = 0; b < numblocks; ++b )
	{
		uint64_t const low = (numblocks-b-1)*s;
		uint64_t const high = std::min(low+s,n);
		uint64_t const len = high-low;
		
		istr.seekg ( - static_cast<int64_t>(len), std::ios_base::cur );
		
		std::cerr << "Reading len=" << len << " at position " << istr.tellg() << std::endl;
		istr.read ( reinterpret_cast<char *>(buf.get()), len );
		
		istr.seekg ( - static_cast<int64_t>(len), std::ios_base::cur );
		
		std::reverse ( buf.get(), buf.get() + len );
		
		for ( uint64_t i = 0; i < len; ++i )
			buf[i] = M[buf[i]];
			
		for ( uint64_t i = 0; i < len; ++i )
		{
			uint64_t const c = buf[i];
			uint64_t const f = scf[c]+1; scf.inc(c);
			p = B.insertAndRank(c,p) - 1 + f;
		}
	}
	B.insert ( 0, p );
	scf.inc(0);

	::libmaus2::autoarray::AutoArray<uint8_t> obuf(n,false);	

	// follow LF to skip terminator
	p = scf[ B[p] ] + (p ? B.rank(B[p],p-1) : 0);

	for ( uint64_t i = 0; i < n; ++i )
	{
		uint64_t const sym = B[p];
		p = scf[sym] + (p ? B.rank(sym,p-1) : 0);
		obuf[n-i-1] = R[sym];
	}
	output.write ( reinterpret_cast<char const *>(obuf.get()), n );
	
	{
		std::ostringstream ostr;
		B.serialise(ostr);
		std::istringstream istr(ostr.str());
		::libmaus2::wavelet::DynamicWaveletTree<k,w> Bre(istr);	
		assert ( B.n == Bre.n );
		assert ( B.identical(Bre) );
		// assert ( B == Bre );
		std::set<int> S;
		
		for ( uint64_t i = 0; i < B.n; ++i )
		{
			assert ( B[i] == B[i] );
			S.insert(B[i]);
		}		

		for ( uint64_t i = 0; i < B.n; ++i )
		{
			assert ( B[i] == Bre[i] );
			for ( std::set<int>::const_iterator ita = S.begin(); ita != S.end(); ++ita )
				assert (
					B.rank(*ita,i) == Bre.rank(*ita,i)
				);
		}
	}
}

int main(int argc, char * argv[])
{
	if ( argc < 3 )
	{
		std::cerr << "usage: " << argv[0] << " <text> <out>" << std::endl;
		return EXIT_FAILURE;
	}
	
	std::string const textfilename = argv[1];
	std::string const outputfilename = argv[2];
	
	try
	{
		libmaus2::aio::OutputStreamInstance out(outputfilename);
		computeBWT(textfilename,out);
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}
