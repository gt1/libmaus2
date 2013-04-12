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

#include <iostream>
#include <libmaus/util/ArgInfo.hpp>
#include <libmaus/util/Utf8BlockIndex.hpp>
#include <libmaus/util/Utf8DecoderBuffer.hpp>
#include <libmaus/util/Utf8String.hpp>
#include <libmaus/aio/CircularWrapper.hpp>
#include <libmaus/huffman/huffman.hpp>
#include <libmaus/wavelet/ImpExternalWaveletGeneratorHuffman.hpp>
#include <libmaus/suffixsort/divsufsort.hpp>
#include <libmaus/wavelet/ImpHuffmanWaveletTree.hpp>
#include <libmaus/lf/LF.hpp>

void testUtf8String(std::string const & fn)
{
	::libmaus::util::Utf8String::shared_ptr_type us = ::libmaus::util::Utf8String::constructRaw(fn);
	::libmaus::util::Utf8StringPairAdapter usp(us);
	
	::libmaus::util::Utf8DecoderWrapper decwr(fn);
	uint64_t const numsyms = ::libmaus::util::GetFileSize::getFileSize(decwr);
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
	::libmaus::huffman::HuffmanTreeNode::shared_ptr_type htree = ::libmaus::huffman::HuffmanBase::createTree(chist);
	::libmaus::huffman::EncodeTable<1> ET(htree.get());
	
	for ( ::std::map<int64_t,uint64_t>::const_iterator ita = chist.begin(); ita != chist.end(); ++ita )
	{
		::libmaus::util::UTF8::encodeUTF8(ita->first,std::cerr);
		std::cerr << "\t" << ita->second << "\t" << ET.printCode(ita->first) << std::endl;
	}
}

void testUtf8Bwt(std::string const & fn)
{
	::libmaus::util::Utf8String us(fn);

	typedef ::libmaus::util::Utf8String::saidx_t saidx_t;
	::libmaus::autoarray::AutoArray<saidx_t,::libmaus::autoarray::alloc_type_c> SA = us.computeSuffixArray32();

	// produce bwt
	for ( uint64_t i = 0; i < SA.size(); ++i )
		if ( SA[i] )
			SA[i] = us[SA[i]-1];
		else
			SA[i] = -1;

	// produce huffman shaped wavelet tree of bwt
	::std::map<int64_t,uint64_t> chist = us.getHistogramAsMap();
	chist[-1] = 1;
	::libmaus::huffman::HuffmanTreeNode::shared_ptr_type htree = ::libmaus::huffman::HuffmanBase::createTree(chist);

	::libmaus::util::TempFileNameGenerator tmpgen(fn+"_tmp",3);
	::libmaus::wavelet::ImpExternalWaveletGeneratorHuffman IEWGH(htree.get(),tmpgen);
	
	IEWGH.putSymbol(us[us.size()-1]);
	for ( uint64_t i = 0; i < SA.size(); ++i )
		IEWGH.putSymbol(SA[i]);
	IEWGH.createFinalStream(fn+".hwt");

	// load huffman shaped wavelet tree of bwt
	::libmaus::wavelet::ImpHuffmanWaveletTree::unique_ptr_type IHWT =
		UNIQUE_PTR_MOVE(::libmaus::wavelet::ImpHuffmanWaveletTree::load(fn+".hwt"));
		
	// check rank counts
	for ( ::std::map<int64_t,uint64_t>::const_iterator ita = chist.begin(); ita != chist.end(); ++ita )
		assert ( IHWT->rank(ita->first,SA.size()) == ita->second );
		
	/* cumulative symbol freqs, shifted by 1 to accomodate for terminator -1 */
	int64_t const maxsym = chist.rbegin()->first;
	int64_t const shiftedmaxsym = maxsym+1;
	::libmaus::autoarray::AutoArray<uint64_t> D(shiftedmaxsym+1);
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
	::libmaus::util::Utf8String us(fn);
	::libmaus::aio::Utf8CircularWrapper CW(fn);

	for ( uint64_t i = 0; i < 16*us.size(); ++i )
		assert ( us[i%us.size()] == static_cast<wchar_t>(CW.get()) );
}

void testUtf8Seek(std::string const & fn)
{
	std::vector < wchar_t > W;
	::libmaus::util::Utf8DecoderWrapper decwr(fn);
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
	::libmaus::util::Utf8BlockIndex::unique_ptr_type index = 
		UNIQUE_PTR_MOVE(::libmaus::util::Utf8BlockIndex::constructFromUtf8File(fn));

	std::string const idxfn = fn + ".idx";
	::libmaus::aio::CheckedOutputStream COS(idxfn);
	index->serialise(COS);
	COS.flush();
	COS.close();
	
	::libmaus::util::Utf8BlockIndexDecoder deco(idxfn);
	assert ( deco.numblocks+1 == index->blockstarts.size() );
	
	for ( uint64_t i = 0; i < deco.numblocks; ++i )
		assert (  deco[i] == index->blockstarts[i] );
		
	assert ( index->blockstarts[deco.numblocks] == ::libmaus::util::GetFileSize::getFileSize(fn) );
	assert ( deco[deco.numblocks] == ::libmaus::util::GetFileSize::getFileSize(fn) );
}

struct ImpWaveletStackElement
{
	uint64_t bleft;
	uint64_t bright;
	uint64_t sleft;
	uint64_t sright;
	unsigned int level;
	::libmaus::huffman::HuffmanTreeNode const * hnode;
	
	ImpWaveletStackElement() : bleft(0), bright(0), sleft(0), sright(0), level(0), hnode(0) {}
	ImpWaveletStackElement(
		uint64_t const rbleft,
		uint64_t const rbright,
		uint64_t const rsleft,
		uint64_t const rsright,
		unsigned int const rlevel,
		::libmaus::huffman::HuffmanTreeNode const * const rhnode
	) : bleft(rbleft), bright(rbright), sleft(rsleft), sright(rsright), level(rlevel), hnode(rhnode) {}
};

std::ostream & operator<<(std::ostream & out, ImpWaveletStackElement const & I)
{
	out << "ImpWaveletStackElement(" 
		<< I.bleft << "," << I.bright << ","
		<< I.sleft << "," << I.sright << ","
		<< I.level << ")";
	return out;
}

void testUtfWavelet(std::string const & fn)
{
	::libmaus::timing::RealTimeClock rtc; rtc.start();	
	::libmaus::util::Utf8String us(fn);
	std::cerr << "loaded string in time " << rtc.getElapsedSeconds() << std::endl;
	
	rtc.start();
	::std::map<int64_t,uint64_t> const chist = us.getHistogramAsMap();
	std::cerr << "computed histogram in time " << rtc.getElapsedSeconds() << std::endl;

	rtc.start();
	::libmaus::huffman::HuffmanTreeNode::shared_ptr_type htree = ::libmaus::huffman::HuffmanBase::createTree(chist);
	std::cerr << "computed Huffman tree in time " << rtc.getElapsedSeconds() << std::endl;
	
	rtc.start();
	::libmaus::huffman::EncodeTable<1> ET(htree.get());
	std::cerr << "computed Huffman encode table in time " << rtc.getElapsedSeconds() << std::endl;

	#define HWTDEBUG
	#define WAVELET_RADIX_SORT
	
	#if defined(WAVELET_RADIX_SORT)
	::libmaus::autoarray::AutoArray<uint8_t> Z(us.A.size(),false);
	#endif

	// std::cerr << "here." << std::endl;
	
	rtc.start();	
	std::stack<ImpWaveletStackElement> S;
	if ( ! htree->isLeaf() )
	{
		S.push(ImpWaveletStackElement(0,us.A.size(),0,us.size(),0,htree.get()));
	
		while ( ! S.empty() )
		{
			ImpWaveletStackElement const T = S.top();
			S.pop();
			
			// std::cerr << T << std::endl;
			
			assert ( ! T.hnode->isLeaf() );
		
			unsigned int const level = T.level;
			uint64_t const srange = T.sright-T.sleft;
		
			::libmaus::util::GetObject<uint8_t const *> G(us.A.begin()+T.bleft);
			#if !defined(WAVELET_RADIX_SORT)
			uint64_t codelen0 = 0;
			uint64_t codelen1 = 0;
			#endif
			
			uint64_t numsyms0 = 0;
			uint64_t numsyms1 = 0;
			#if defined(WAVELET_RADIX_SORT)
			::libmaus::util::PutObject<uint8_t *> P0(Z.begin());
			::libmaus::util::PutObjectReverse<uint8_t *> P1(Z.end());
			#endif
			for ( uint64_t i = 0; i < srange; ++i )
			{
				uint64_t codelen = 0;
				wchar_t const sym = ::libmaus::util::UTF8::decodeUTF8(G,codelen);
				bool const wbit = ET.getBitFromTop(sym,level);
			
				if ( wbit )
				{
					#if !defined(WAVELET_RADIX_SORT)
					codelen1 += codelen;
					#endif
					numsyms1 += 1;

					#if defined(WAVELET_RADIX_SORT)
					P1.write(G.p-codelen,codelen);
					#endif
				}
				else
				{
					#if !defined(WAVELET_RADIX_SORT)
					codelen0 += codelen;
					#endif
					numsyms0 += 1;
					
					#if defined(WAVELET_RADIX_SORT)
					P0.write(G.p-codelen,codelen);
					#endif
				}
			}

			#if defined(WAVELET_RADIX_SORT)
			uint64_t const codelen0 = (P0.p-Z.begin());
			uint64_t const codelen1 = (Z.end()-P1.p);
			#endif

			#if defined(WAVELET_RADIX_SORT)
			std::copy(Z.begin(),Z.begin()+codelen0,us.A.begin()+T.bleft);
			uint8_t const * zp = Z.end();
			uint8_t * p1 = us.A.begin()+T.bleft+codelen0;
			uint8_t * const p1e = p1 + codelen1;
			while ( p1 != p1e )
				*(p1++) = *(--zp);
			#else
			for ( uint64_t sorthalf = 1; sorthalf < srange; sorthalf *=2 )
			{
				uint64_t const sortfull = 2*sorthalf;
				uint64_t const sortparts = (srange + sortfull-1)/sortfull;
				uint64_t bleft = T.bleft;
								
				for ( uint64_t s = 0; s < sortparts; ++s )
				{
					uint64_t const lsortbase = s*sortfull;
					uint64_t const lsortrighta = std::min(lsortbase + sorthalf,srange);
					uint64_t const lsortrightb = std::min(lsortrighta + sorthalf,srange);
					
					uint64_t l_codelen0 = 0;
					uint64_t l_codelen1 = 0;
					uint64_t l_numsyms0 = 0;
					uint64_t l_numsyms1 = 0;
					uint64_t r_codelen0 = 0;
					uint64_t r_codelen1 = 0;
					uint64_t r_numsyms0 = 0;
					uint64_t r_numsyms1 = 0;
					
					::libmaus::util::GetObject<uint8_t const *> LG(us.A.begin()+bleft);
					for ( uint64_t i = 0; i < (lsortrighta-lsortbase); ++i )
					{
						uint64_t codelen = 0;
						wchar_t const sym = ::libmaus::util::UTF8::decodeUTF8(LG,codelen);
						bool const wbit = ET.getBitFromTop(sym,level);
					
						if ( wbit )
						{
							l_codelen1 += codelen;
							l_numsyms1 += 1;
						}
						else
						{
							l_codelen0 += codelen;
							l_numsyms0 += 1;
						}
						
					}
					
					for ( uint64_t i = 0; i < (lsortrightb-lsortrighta); ++i )
					{
						uint64_t codelen = 0;
						wchar_t const sym = ::libmaus::util::UTF8::decodeUTF8(LG,codelen);
						bool const wbit = ET.getBitFromTop(sym,level);
					
						if ( wbit )
						{
							r_codelen1 += codelen;
							r_numsyms1 += 1;
						}
						else
						{
							r_codelen0 += codelen;
							r_numsyms0 += 1;
						}	
					}

					if ( l_numsyms1 && r_numsyms0 )
					{
						std::reverse(us.A.begin() + bleft + l_codelen0             , us.A.begin() + bleft + l_codelen0 + l_codelen1              );
						std::reverse(us.A.begin() + bleft + l_codelen0 + l_codelen1, us.A.begin() + bleft + l_codelen0 + l_codelen1 + r_codelen0 );
						std::reverse(us.A.begin() + bleft + l_codelen0             , us.A.begin() + bleft + l_codelen0 + l_codelen1 + r_codelen0 );
					}
					
					bleft += (l_codelen0+l_codelen1+r_codelen0+r_codelen1);
				}
			}
			#endif
		
			::libmaus::huffman::HuffmanTreeInnerNode const * node = dynamic_cast< ::libmaus::huffman::HuffmanTreeInnerNode const * >(T.hnode);
			
			if ( (!(node->right->isLeaf())) )
				S.push(ImpWaveletStackElement(T.bleft+codelen0,T.bright,T.sleft+numsyms0,T.sright,level+1,node->right));
			if ( (!(node->left->isLeaf())) )
				S.push(ImpWaveletStackElement(T.bleft,T.bleft+codelen0,T.sleft,T.sleft+numsyms0,level+1,node->left));

			#if defined(HWTDEBUG)
			::libmaus::util::GetObject<uint8_t const *> DG(us.A.begin()+T.bleft);
			uint64_t d_codelen0 = 0;
			uint64_t d_codelen1 = 0;
			uint64_t d_numsyms0 = 0;
			uint64_t d_numsyms1 = 0;
			for ( uint64_t i = 0; i < srange; ++i )
			{
				uint64_t codelen = 0;
				wchar_t const sym = ::libmaus::util::UTF8::decodeUTF8(DG,codelen);
				bool const wbit = ET.getBitFromTop(sym,level);
			
				if ( wbit )
				{
					if ( ! d_numsyms1 )
					{
						assert ( codelen0 == d_codelen0 );
						assert ( numsyms0 == d_numsyms0 );
					}
				
					d_codelen1 += codelen;
					d_numsyms1 += 1;
				}
				else
				{
					d_codelen0 += codelen;
					d_numsyms0 += 1;
				}
			}

			assert ( codelen0 == d_codelen0 );
			assert ( codelen1 == d_codelen1 );
			assert ( numsyms0 == d_numsyms0 );
			assert ( numsyms1 == d_numsyms1 );
			#endif
		}
	}
	
	std::cerr << rtc.getElapsedSeconds() << std::endl;
}

int main(int argc, char * argv[])
{
	try
	{
		::libmaus::util::ArgInfo const arginfo(argc,argv);
		
		std::string const fn = arginfo.getRestArg<std::string>(0);
		
		testUtf8BlockIndexDecoder(fn); // also creates index file
		testUtfWavelet(fn);
		/*
		testUtf8Bwt(fn);
		testUtf8String(fn);		
		testUtf8Circular(fn);
		testUtf8Seek(fn);
		*/
		
		remove ( (fn + ".idx").c_str() ); // remove index
	}
	catch(std::exception const & ex)
	{
	
	}
}
