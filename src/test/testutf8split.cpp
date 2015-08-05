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

#include <iostream>
#include <libmaus2/util/ArgInfo.hpp>
#include <libmaus2/util/Utf8BlockIndex.hpp>
#include <libmaus2/util/Utf8DecoderBuffer.hpp>
#include <libmaus2/util/Utf8String.hpp>
#include <libmaus2/util/Utf8StringPairAdapter.hpp>
#include <libmaus2/aio/CircularWrapper.hpp>
#include <libmaus2/huffman/huffman.hpp>
#include <libmaus2/wavelet/ImpExternalWaveletGeneratorHuffman.hpp>
#include <libmaus2/suffixsort/divsufsort.hpp>
#include <libmaus2/wavelet/ImpHuffmanWaveletTree.hpp>
#include <libmaus2/wavelet/ImpCompactHuffmanWaveletTree.hpp>
#include <libmaus2/util/TempFileRemovalContainer.hpp>
#include <libmaus2/lf/LF.hpp>
#include <libmaus2/util/MemUsage.hpp>
#include <libmaus2/wavelet/Utf8ToImpHuffmanWaveletTree.hpp>
#include <libmaus2/util/FileTempFileContainer.hpp>

void testUtf8String(std::string const & fn)
{
	::libmaus2::util::Utf8String::shared_ptr_type us = ::libmaus2::util::Utf8String::constructRaw(fn);
	::libmaus2::util::Utf8StringPairAdapter usp(us);
	
	::libmaus2::util::Utf8DecoderWrapper decwr(fn);
	uint64_t const numsyms = ::libmaus2::util::GetFileSize::getFileSize(decwr);
	assert ( us->size() == numsyms );

	for ( uint64_t i = 0; i < numsyms; ++i )
	{
		assert ( static_cast<wchar_t>(decwr.get()) == us->get(i) );
		assert ( 
			us->get(i) ==
			((usp[2*i] << 12) | (usp[2*i+1]))
		);
	}
	
	::std::map<int64_t,uint64_t> const chist = us->getHistogramAsMap();
	::libmaus2::huffman::HuffmanTreeNode::shared_ptr_type htree = ::libmaus2::huffman::HuffmanBase::createTree(chist);
	::libmaus2::huffman::EncodeTable<1> ET(htree.get());
	
	for ( ::std::map<int64_t,uint64_t>::const_iterator ita = chist.begin(); ita != chist.end(); ++ita )
	{
		::libmaus2::util::UTF8::encodeUTF8(ita->first,std::cerr);
		std::cerr << "\t" << ita->second << "\t" << ET.printCode(ita->first) << std::endl;
	}
}

void testUtf8Bwt(std::string const & fn)
{
	::libmaus2::util::Utf8String us(fn);

	typedef ::libmaus2::util::Utf8String::saidx_t saidx_t;
	::libmaus2::autoarray::AutoArray<saidx_t,::libmaus2::autoarray::alloc_type_c> SA = us.computeSuffixArray32();

	// produce bwt
	for ( uint64_t i = 0; i < SA.size(); ++i )
		if ( SA[i] )
			SA[i] = us[SA[i]-1];
		else
			SA[i] = -1;

	// produce huffman shaped wavelet tree of bwt
	::std::map<int64_t,uint64_t> chist = us.getHistogramAsMap();
	chist[-1] = 1;
	::libmaus2::huffman::HuffmanTreeNode::shared_ptr_type htree = ::libmaus2::huffman::HuffmanBase::createTree(chist);

	::libmaus2::util::TempFileNameGenerator tmpgen(fn+"_tmp",3);
	::libmaus2::util::FileTempFileContainer tmpcnt(tmpgen);
	::libmaus2::wavelet::ImpExternalWaveletGeneratorHuffman IEWGH(htree.get(),tmpcnt);
	
	IEWGH.putSymbol(us[us.size()-1]);
	for ( uint64_t i = 0; i < SA.size(); ++i )
		IEWGH.putSymbol(SA[i]);
	IEWGH.createFinalStream(fn+".hwt");

	// load huffman shaped wavelet tree of bwt
	::libmaus2::wavelet::ImpHuffmanWaveletTree::unique_ptr_type IHWT
		(::libmaus2::wavelet::ImpHuffmanWaveletTree::load(fn+".hwt"));
		
	// check rank counts
	for ( ::std::map<int64_t,uint64_t>::const_iterator ita = chist.begin(); ita != chist.end(); ++ita )
		assert ( IHWT->rank(ita->first,SA.size()) == ita->second );
		
	/* cumulative symbol freqs, shifted by 1 to accomodate for terminator -1 */
	int64_t const maxsym = chist.rbegin()->first;
	int64_t const shiftedmaxsym = maxsym+1;
	::libmaus2::autoarray::AutoArray<uint64_t> D(shiftedmaxsym+1);
	for ( ::std::map<int64_t,uint64_t>::const_iterator ita = chist.begin(); ita != chist.end(); ++ita )
		D [ ita->first + 1 ] = ita->second;
	D.prefixSums();
	
	// terminator has rank 0 and is at position us.size()
	uint64_t rank = 0;
	int64_t pos = us.size();
	
	// decode text backward from bwt
	while ( --pos >= 0 )
	{
		std::pair< int64_t,uint64_t> const is = IHWT->inverseSelect(rank);
		rank = D[is.first+1] + is.second;		
		assert ( is.first == us[pos] );
	}
	
	// remove huffman shaped wavelet tree
	remove ((fn+".hwt").c_str());
}

void testUtf8Circular(std::string const & fn)
{
	::libmaus2::util::Utf8String us(fn);
	::libmaus2::aio::Utf8CircularWrapper CW(fn);

	for ( uint64_t i = 0; i < 16*us.size(); ++i )
		assert ( us[i%us.size()] == static_cast<wchar_t>(CW.get()) );
}

void testUtf8Seek(std::string const & fn)
{
	std::vector < wchar_t > W;
	::libmaus2::util::Utf8DecoderWrapper decwr(fn);
	int64_t lastperc = -1;
	
	wchar_t w = -1;
	while ( (w=decwr.get()) >= 0 )
		W.push_back(w);
	
	for ( uint64_t i = 0; i <= W.size(); i += std::min(
		static_cast<uint64_t>(W.size()-i+1),
		static_cast<uint64_t>(static_cast<size_t>(rand() % 1024))) )
	{
		int64_t const newperc = std::floor((i*100.0)/W.size()+0.5);
		
		if ( newperc != lastperc )
		{
			lastperc = newperc;
			if ( isatty(STDERR_FILENO) )
				std::cerr << "\r" << std::string(40,' ') << "\r" << newperc << std::flush;
			else
				std::cerr << newperc << std::endl;			
		}
		
		decwr.clear();
		decwr.seekg(i);
		
		for ( uint64_t j = i; j < W.size(); ++j )
			assert ( static_cast<wchar_t>(decwr.get()) == W[j] );
	}

	if ( isatty(STDERR_FILENO) )
		std::cerr << "\r" << std::string(40,' ') << "\r" << 100 << std::endl;	
	else
		std::cerr << 100 << std::endl;			
}

void testUtf8BlockIndexDecoder(std::string const & fn)
{
	::libmaus2::util::Utf8BlockIndex::unique_ptr_type index = 
		(::libmaus2::util::Utf8BlockIndex::constructFromUtf8File(fn));

	std::string const idxfn = fn + ".idx";
	{
		::libmaus2::aio::OutputStreamInstance COS(idxfn);
		index->serialise(COS);
		COS.flush();
	}
	
	::libmaus2::util::Utf8BlockIndexDecoder deco(idxfn);
	assert ( deco.numblocks+1 == index->blockstarts.size() );
	
	for ( uint64_t i = 0; i < deco.numblocks; ++i )
		assert (  deco[i] == index->blockstarts[i] );
		
	assert ( index->blockstarts[deco.numblocks] == ::libmaus2::util::GetFileSize::getFileSize(fn) );
	assert ( deco[deco.numblocks] == ::libmaus2::util::GetFileSize::getFileSize(fn) );
}

#include <libmaus2/wavelet/Utf8ToImpCompactHuffmanWaveletTree.hpp>

void testUtf8ToImpHuffmanWaveletTree(std::string const & fn)
{
	{
		::libmaus2::wavelet::Utf8ToImpHuffmanWaveletTree::constructWaveletTree<true>(fn,fn+".hwt");
		// load huffman shaped wavelet tree of bwt
		::libmaus2::wavelet::ImpHuffmanWaveletTree::unique_ptr_type IHWT
			(::libmaus2::wavelet::ImpHuffmanWaveletTree::load(fn+".hwt"));
		::libmaus2::util::Utf8String::shared_ptr_type us = ::libmaus2::util::Utf8String::constructRaw(fn);
		std::cerr << "checking length " << us->size() << std::endl;
		for ( uint64_t i = 0; i < us->size(); ++i )
			assert ( (*us)[i] == (*IHWT)[i] );
	}
	
	{
		::libmaus2::wavelet::Utf8ToImpCompactHuffmanWaveletTree::constructWaveletTree<true>(fn,fn+".hwt");
		// load huffman shaped wavelet tree of bwt
		::libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type IHWT
			(::libmaus2::wavelet::ImpCompactHuffmanWaveletTree::load(fn+".hwt"));
		::libmaus2::util::Utf8String::shared_ptr_type us = ::libmaus2::util::Utf8String::constructRaw(fn);
		std::cerr << "checking length " << us->size() << "," << IHWT->size() << std::endl;
		for ( uint64_t i = 0; i < us->size(); ++i )
			assert ( (*us)[i] == (*IHWT)[i] );		
	}
}

int main(int argc, char * argv[])
{
	try
	{
		::libmaus2::util::ArgInfo const arginfo(argc,argv);
		
		std::string const fn = arginfo.getRestArg<std::string>(0);

		testUtf8BlockIndexDecoder(fn); // also creates index file
		testUtf8Bwt(fn);
		
		#if !defined(__GNUC__) || (defined(__GNUC__) && GCC_VERSION >= 40200)
		testUtf8ToImpHuffmanWaveletTree(fn);	
		#endif

		#if 0
		testUtf8String(fn);		
		testUtf8Circular(fn);
		testUtf8Seek(fn);
		#endif
		
		remove ( (fn + ".idx").c_str() ); // remove index
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;	
		return EXIT_FAILURE;
	}
}
