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
#include <libmaus/util/TempFileRemovalContainer.hpp>
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

void testUtfWavelet(std::string const & fn, std::string const & outputfilename)
{
	::libmaus::util::TempFileRemovalContainer::setup();

	::libmaus::autoarray::AutoArray<uint8_t> A = ::libmaus::aio::SynchronousGenericInput<uint8_t>::readArray(fn);
	
	::std::map<int64_t,uint64_t> const chist = ::libmaus::util::Utf8String::getHistogramAsMap(A);
	::libmaus::huffman::HuffmanTreeNode::shared_ptr_type htree = ::libmaus::huffman::HuffmanBase::createTree(chist);	
	::libmaus::huffman::EncodeTable<1> ET(htree.get());

	// #define HWTDEBUG
	#define WAVELET_RADIX_SORT
	
	::libmaus::timing::RealTimeClock rtc; rtc.start();	
	if ( ! htree->isLeaf() )
	{
		#if defined(_OPENMP)
		uint64_t const numthreads = omp_get_max_threads();
		#else
		uint64_t const numthreads = 1;
		#endif

		::libmaus::autoarray::AutoArray<uint64_t> const partstarts = ::libmaus::util::Utf8String::computePartStarts(A,numthreads);
		uint64_t const numparts = partstarts.size()-1;
		
		::libmaus::autoarray::AutoArray<uint64_t> symsperpart(numparts+1);
		#if defined(_OPENMP)
		#pragma omp parallel for
		#endif
		for ( int64_t i = 0; i < static_cast<int64_t>(numparts); ++i )
		{
			::libmaus::aio::CheckedInputStream CIS(fn);
			CIS.seekg(partstarts[i]);
			uint8_t const * const pe = A.begin() + partstarts[i+1];
			uint64_t lsyms = 0;

			for ( uint8_t const * pa = A.begin() + partstarts[i]; pa != pe; ++pa )
				if ( (*pa & 0xc0) != 0x80 )
					++lsyms;
			
			symsperpart[i] = lsyms;
		}
		symsperpart.prefixSums();

		std::string const tmpfilenamebase = fn+"_tmp";
		std::vector<std::string> tmpfilenames;
		for ( uint64_t i = 0; i < numparts; ++i )
		{
			tmpfilenames.push_back(
				tmpfilenamebase + "_" + ::libmaus::util::NumberSerialisation::formatNumber(i,6)
			);
			::libmaus::util::TempFileRemovalContainer::addTempFile(tmpfilenames[i]);
			// touch file
			::libmaus::aio::CheckedOutputStream tmpCOS(tmpfilenames[i]);
		}
	
		uint64_t const numnodes = chist.size()-1;
		::libmaus::autoarray::AutoArray< ::libmaus::autoarray::AutoArray<uint64_t> > vnodebitcnt(numparts);
		::libmaus::autoarray::AutoArray< ::libmaus::autoarray::AutoArray<uint64_t> > vnodewordcnt(numparts+1);
		for ( uint64_t i = 0; i < numparts; ++i )
		{
			vnodebitcnt[i] = ::libmaus::autoarray::AutoArray<uint64_t>(numnodes);
			vnodewordcnt[i] = ::libmaus::autoarray::AutoArray<uint64_t>(numnodes+1);
		}
		
		#if defined(_OPENMP)
		#pragma omp parallel for
		#endif
		for ( int64_t partid = 0; partid < static_cast<int64_t>(numparts); ++partid )
		{
			::libmaus::autoarray::AutoArray<uint64_t> & nodebitcnt = vnodebitcnt[partid];
			::libmaus::autoarray::AutoArray<uint64_t> & nodewordcnt = vnodewordcnt[partid];
			uint64_t lnodeid = 0;
			
			uint64_t const pbleft = partstarts[partid];
			uint64_t const pbright = partstarts[partid+1];
			uint64_t const lnumsyms = symsperpart[partid+1]-symsperpart[partid];
			
			#if defined(WAVELET_RADIX_SORT)
			::libmaus::autoarray::AutoArray<uint8_t> Z(pbright-pbleft,false);
			#endif

			std::stack<ImpWaveletStackElement> S;
			S.push(ImpWaveletStackElement(pbleft,pbright,0,lnumsyms,0,htree.get()));
		
			::libmaus::aio::CheckedOutputStream::unique_ptr_type tmpCOS(new ::libmaus::aio::CheckedOutputStream(tmpfilenames[partid]));
			::libmaus::aio::SynchronousGenericOutput<uint64_t>::unique_ptr_type tmpSGO(
				new ::libmaus::aio::SynchronousGenericOutput<uint64_t>(*tmpCOS,8*1024));
		
			while ( ! S.empty() )
			{
				ImpWaveletStackElement const T = S.top();
				S.pop();
				
				assert ( ! T.hnode->isLeaf() );
			
				unsigned int const level = T.level;
				uint64_t const srange = T.sright-T.sleft;
				
				nodebitcnt[lnodeid] = srange;
				nodewordcnt[lnodeid] = (srange + 63)/64;
				lnodeid++;
			
				::libmaus::util::GetObject<uint8_t const *> G(A.begin()+T.bleft);
				#if !defined(WAVELET_RADIX_SORT)
				uint64_t codelen0 = 0;
				uint64_t codelen1 = 0;
				#endif
				
				uint64_t const prewords = tmpSGO->getWrittenWords();
				uint64_t numsyms0 = 0;
				uint64_t numsyms1 = 0;
				unsigned int av = 64;
				uint64_t outword = 0;
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
						
						outword <<= 1;
						outword |= 1ull;
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

						outword <<= 1;
					}
					
					if ( ! --av )
					{
						tmpSGO->put(outword);
						av = 64;
					}
				}
				
				if ( av != 64 )
				{
					outword <<= av;
					tmpSGO->put(outword);
				}

				uint64_t const postwords = tmpSGO->getWrittenWords();
				uint64_t const wordswritten = postwords-prewords;
				assert ( wordswritten == nodewordcnt[lnodeid-1] );
				
				#if defined(WAVELET_RADIX_SORT)
				uint64_t const codelen0 = (P0.p-Z.begin());
				uint64_t const codelen1 = (Z.end()-P1.p);
				#endif

				#if defined(WAVELET_RADIX_SORT)
				std::copy(Z.begin(),Z.begin()+codelen0,A.begin()+T.bleft);
				uint8_t const * zp = Z.end();
				uint8_t * p1 = A.begin()+T.bleft+codelen0;
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
						
						::libmaus::util::GetObject<uint8_t const *> LG(A.begin()+bleft);
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
							std::reverse(A.begin() + bleft + l_codelen0             , A.begin() + bleft + l_codelen0 + l_codelen1              );
							std::reverse(A.begin() + bleft + l_codelen0 + l_codelen1, A.begin() + bleft + l_codelen0 + l_codelen1 + r_codelen0 );
							std::reverse(A.begin() + bleft + l_codelen0             , A.begin() + bleft + l_codelen0 + l_codelen1 + r_codelen0 );
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
				::libmaus::util::GetObject<uint8_t const *> DG(A.begin()+T.bleft);
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

			assert ( lnodeid == nodebitcnt.size() );
			
			tmpSGO->flush();
			tmpSGO.reset();
			tmpCOS->flush();
			tmpCOS->close();
			tmpCOS.reset();
		}

		// accumulate word offsets
		for ( uint64_t i = 0; i < numparts; ++i )
			vnodewordcnt[i].prefixSums();
		
		::libmaus::autoarray::AutoArray<uint64_t> vnodebits(numnodes);
		uint64_t tnumbits = 0;
		for ( uint64_t nodeid = 0; nodeid < numnodes; ++nodeid )
		{
			uint64_t nodebits = 0;
			for ( uint64_t p = 0; p < numparts; ++p )
				nodebits += vnodebitcnt[p][nodeid];
			
			vnodebits[nodeid] = nodebits;
			tnumbits += nodebits;
		}
				
		uint64_t const tpartnodebits = (tnumbits+numthreads-1)/numthreads;
		std::vector < std::pair<uint64_t,uint64_t> > nodepacks;
		
		uint64_t llow = 0;
		while ( llow != numnodes )
		{
			uint64_t lhigh = llow;
			uint64_t s = 0;
			
			while ( lhigh != numnodes && s < tpartnodebits )
				s += vnodebits[lhigh++];

			nodepacks.push_back(std::pair<uint64_t,uint64_t>(llow,lhigh));

			llow = lhigh;
		}
		
		assert ( nodepacks.size() <= numthreads );

		std::vector<std::string> nptempfilenames;
		::libmaus::autoarray::AutoArray< ::libmaus::aio::CheckedOutputStream::unique_ptr_type > tmpCOS(nodepacks.size());
		for ( uint64_t np = 0; np < nodepacks.size(); ++np )
		{
			nptempfilenames.push_back(tmpfilenamebase + "_np_" + ::libmaus::util::NumberSerialisation::formatNumber(np,6));
			::libmaus::util::TempFileRemovalContainer::addTempFile(nptempfilenames[np]);
			tmpCOS[np] = UNIQUE_PTR_MOVE(
				::libmaus::aio::CheckedOutputStream::unique_ptr_type(
					new ::libmaus::aio::CheckedOutputStream(nptempfilenames[np])
				)
			);
		}
		
		::libmaus::autoarray::AutoArray<uint64_t> nodebytesizes(numnodes);
		
		#if defined(_OPENMP)
		#pragma omp parallel for
		#endif
		for ( uint64_t np = 0; np < nodepacks.size(); ++np )
		{
			uint64_t const nplow = nodepacks[np].first;
			uint64_t const nphigh = nodepacks[np].second;
			::libmaus::aio::CheckedOutputStream & npout = *(tmpCOS[np]);
			
			::libmaus::autoarray::AutoArray < ::libmaus::aio::CheckedInputStream::unique_ptr_type > tmpCIS(numparts);
			::libmaus::autoarray::AutoArray < ::libmaus::aio::SynchronousGenericInput<uint64_t>::unique_ptr_type > tmpSGI(numparts);
			
			for ( uint64_t i = 0; i < numparts; ++i )
			{
				tmpCIS[i] = UNIQUE_PTR_MOVE(
					::libmaus::aio::CheckedInputStream::unique_ptr_type(new ::libmaus::aio::CheckedInputStream(tmpfilenames[i]))
				);
				tmpCIS[i]->seekg(vnodewordcnt[i][nplow]*sizeof(uint64_t));
				tmpSGI[i] = UNIQUE_PTR_MOVE(
					::libmaus::aio::SynchronousGenericInput<uint64_t>::unique_ptr_type(new ::libmaus::aio::SynchronousGenericInput<uint64_t>(*tmpCIS[i],1024))
				);
			}

			for ( uint64_t npi = nplow; npi < nphigh; ++npi )
			{
				uint64_t const totalnodebits = vnodebits[npi];

				::libmaus::rank::ImpCacheLineRank::WriteContextExternal context(npout,totalnodebits);
				
				for ( uint64_t p = 0; p < numparts; ++p )
				{
					uint64_t bitstowrite = vnodebitcnt[p][npi];
					uint64_t word = 0;
					int shift = -1;
					
					while ( bitstowrite )
					{
						if ( shift < 0 )
						{
							tmpSGI[p]->getNext(word);
							shift = 63;
						}
						
						bool const bit = (word >> shift) & 1;
						context.writeBit(bit);
						
						--bitstowrite;
						--shift;
					}
				}
				
				context.flush();
				
				nodebytesizes[npi] = (2 /* header */+context.wordsWritten())*sizeof(uint64_t);
			}
			
			npout.flush();
			tmpCOS[np].reset();
		}

		for ( uint64_t i = 0; i < tmpfilenames.size(); ++i )
			remove ( tmpfilenames[i].c_str() );
			
		nodebytesizes.prefixSums();

		uint64_t outfilepos = 0;
		::libmaus::aio::CheckedOutputStream::unique_ptr_type Pfinalout(new ::libmaus::aio::CheckedOutputStream(outputfilename));
		::libmaus::aio::CheckedOutputStream & finalout = *Pfinalout;
		
		outfilepos += ::libmaus::util::NumberSerialisation::serialiseNumber(finalout,symsperpart[numparts]);
		outfilepos += htree->serialize(finalout);
		outfilepos += ::libmaus::util::NumberSerialisation::serialiseNumber(finalout,numnodes);
		
		uint64_t const dictbasepos = outfilepos;
		for ( uint64_t i = 0; i < numnodes; ++i )
			nodebytesizes[i] += dictbasepos;
		
		for ( uint64_t i = 0; i < nptempfilenames.size(); ++i )
		{
			::libmaus::aio::CheckedInputStream tmpCIS(nptempfilenames[i]);
			uint64_t const tmpfilesize = ::libmaus::util::GetFileSize::getFileSize(tmpCIS);
			::libmaus::util::GetFileSize::copy(tmpCIS,finalout,tmpfilesize);
			outfilepos += tmpfilesize;
			remove ( nptempfilenames[i].c_str() );
		}
		
		uint64_t const indexpos = outfilepos;	

		outfilepos += ::libmaus::util::NumberSerialisation::serialiseNumber(finalout,numnodes);
		for ( uint64_t i = 0; i < numnodes; ++i )
			outfilepos += ::libmaus::util::NumberSerialisation::serialiseNumber(finalout,nodebytesizes[i]);

		outfilepos += ::libmaus::util::NumberSerialisation::serialiseNumber(finalout,indexpos);
		
		finalout.flush();
		Pfinalout.reset();
		
		#if defined(HWTDEBUG)
		/**
		 * load tree and write out text
		 **/
		::libmaus::wavelet::ImpHuffmanWaveletTree::unique_ptr_type PIHWT = UNIQUE_PTR_MOVE(
			::libmaus::wavelet::ImpHuffmanWaveletTree::load(outputfilename)
		);
		::libmaus::wavelet::ImpHuffmanWaveletTree const & IHWT = *PIHWT;
		assert ( IHWT.getN() == symsperpart[symsperpart.size()-1] );
		
		::libmaus::aio::CheckedOutputStream debCOS(fn + ".debug");
		for ( uint64_t i = 0; i < IHWT.size(); ++i )
			::libmaus::util::UTF8::encodeUTF8(IHWT[i],debCOS);
		debCOS.flush();
		debCOS.close();
		#endif
	}
		
	// std::cerr << rtc.getElapsedSeconds() << std::endl;
}

int main(int argc, char * argv[])
{
	try
	{
		::libmaus::util::ArgInfo const arginfo(argc,argv);
		
		std::string const fn = arginfo.getRestArg<std::string>(0);
		
		// testUtf8BlockIndexDecoder(fn); // also creates index file
		testUtfWavelet(fn,fn+".hwt");
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
		std::cerr << ex.what() << std::endl;	
	}
}
