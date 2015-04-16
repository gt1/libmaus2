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

#include <libmaus2/wavelet/ExternalWaveletGenerator.hpp>
#include <libmaus2/wavelet/ImpWaveletTree.hpp>
#include <libmaus2/wavelet/ImpExternalWaveletGenerator.hpp>
#include <libmaus2/wavelet/ImpExternalWaveletGeneratorHuffmanParallel.hpp>
#include <libmaus2/wavelet/ImpExternalWaveletGeneratorHuffman.hpp>
#include <libmaus2/wavelet/ImpHuffmanWaveletTree.hpp>
#include <libmaus2/lf/LFZero.hpp>

#include <libmaus2/wavelet/ImpExternalWaveletGeneratorCompactHuffman.hpp>
#include <libmaus2/wavelet/ImpCompactHuffmanWaveletTree.hpp>

void testImpExternalWaveletGenerator()
{
	::libmaus2::util::TempFileNameGenerator tmpgen("tmpdir",1);
	uint64_t const b = 3;
	::libmaus2::wavelet::ImpExternalWaveletGenerator IEWG(b,tmpgen);
	
	#if 0
	IEWG.putSymbol(0);
	IEWG.putSymbol(1);
	IEWG.putSymbol(2);
	IEWG.putSymbol(3);
	IEWG.putSymbol(4);
	IEWG.putSymbol(5);
	IEWG.putSymbol(6);
	IEWG.putSymbol(7);
	#endif

	std::vector < uint64_t > V;	
	for ( uint64_t i = 0; i < 381842*41; ++i )
	{
		uint64_t const v = rand() % (1ull<<b);
		// uint64_t const v = i % (1ull<<b);
		IEWG.putSymbol(v);
		V.push_back(v);
	}

	
	std::ostringstream ostr;
	IEWG.createFinalStream(ostr);
	std::istringstream istr(ostr.str());
	
	::libmaus2::wavelet::ImpWaveletTree IWT(istr);
	
	std::cerr << "Testing...";
	std::vector<uint64_t> R(1ull << b,0);
	for ( uint64_t i = 0; i < IWT.size(); ++i )
	{
		std::pair<uint64_t,uint64_t> IS = IWT.inverseSelect(i);
		assert ( IS.first == V[i] );
		assert ( IS.second == R[V[i]] );

		uint64_t const s = IWT.select(V[i],R[V[i]]);
		// std::cerr << "expect " << i << " got " << s << std::endl;
		assert ( s == i );

		R [ V[i] ] ++;
		assert ( IWT.rank(V[i],i) == R[V[i]] );
		assert ( IWT[i] == V[i] );
	}
	std::cerr << "done." << std::endl;
	
	#if 0
	for ( uint64_t i = 0; i < IWT.n; ++i )
	{
		std::cerr << "IWT[" << i << "]=" << IWT[i] << std::endl;
		std::cerr << "rank(" << IWT[i] << "," << i << ")" << "=" << IWT.rank(IWT[i],i) << std::endl;
	}
	#endif
}

#include <libmaus2/huffman/huffman.hpp>

void testHuffmanWavelet()
{
	// std::string text = "Hello world.";
	std::string text = "fischers fritze fischt frische fische der biber schwimmt im fluss und bleibt immer treu";
	
	#if 1
	for ( uint64_t i = 0; i < 16; ++i )
		text = text+text;
	#endif
	
	#if 1	
	text = text.substr(0,1572929);
	#endif
		
	std::cerr << "Checking text of size " << text.size() << std::endl;
	
	::libmaus2::util::shared_ptr< ::libmaus2::huffman::HuffmanTreeNode >::type sroot = ::libmaus2::huffman::HuffmanBase::createTree(text.begin(),text.end());
	
	::libmaus2::util::TempFileNameGenerator tmpgen("tmphuf",3);
	uint64_t const numfrags = 128;

	#define PAR
	#if defined(PAR)	
	::libmaus2::wavelet::ImpExternalWaveletGeneratorHuffmanParallel exgen(sroot.get(), tmpgen, numfrags);
	#else
	::libmaus2::wavelet::ImpExternalWaveletGeneratorHuffman exgen(sroot.get(), tmpgen);
	#endif
	
	#if defined(PAR) && defined(_OPENMP)
	#pragma omp parallel for
	#endif
	for ( int64_t f = 0; f < static_cast<int64_t>(numfrags); ++f )
	{	
		uint64_t const symsperfrag = (text.size() + numfrags-1)/numfrags;
		uint64_t const low = std::min(static_cast<uint64_t>(f*symsperfrag),static_cast<uint64_t>(text.size()));
		uint64_t const high = std::min(static_cast<uint64_t>(low+symsperfrag),static_cast<uint64_t>(text.size()));

		// std::cerr << "f=" << f << " low=" << low << " high=" << high << std::endl;
		
		for ( uint64_t i = low; i < high; ++i )
			#if defined(PAR)
			exgen[f].putSymbol(text[i]);
			#else
			exgen.putSymbol(text[i]);
			#endif
	}
	exgen.createFinalStream("hufwuf");

	std::ifstream istr("hufwuf",std::ios::binary);
	::libmaus2::wavelet::ImpHuffmanWaveletTree IHWT(istr);
	::libmaus2::autoarray::AutoArray<int64_t> symar = sroot->symbolArray();
	
	::libmaus2::huffman::EncodeTable<1> E(IHWT.sroot.get());
	E.print();
	
	#if 0
	for ( uint64_t i = 0; i < IHWT.size(); ++i )
		std::cerr << static_cast<char>(IHWT[i]);
	std::cerr << std::endl;
	#endif
	
	std::map<int64_t, uint64_t> rmap;
	
	#if 0
	for ( uint64_t i = 0; i < text.size(); ++i )
	{
		std::cerr << static_cast<char>(IHWT[i]);
	}
	std::cerr << std::endl;
	#endif
	
	for ( uint64_t i = 0; i < IHWT.size(); ++i )
	{
		#if 0
		std::cerr 
			<< static_cast<char>(IHWT.inverseSelect(i).first)
			<< "("
			<< IHWT.inverseSelect(i).second
			<< ")"
			<< "["
			<< IHWT.rank(text[i],i)
			<< "]";
		#endif

		/**
		 * check symbol
		 **/
		if ( static_cast<int64_t>(IHWT[i]) != text[i] )
			std::cerr << "Failure for i=" << i << " expected " << static_cast<int>(text[i]) << " got " << IHWT[i] << std::endl;
		assert ( static_cast<int64_t>(IHWT[i]) == text[i] );

		/**
		 * compare rank to rankm
		 **/
		for ( uint64_t j = 0; j < symar.size(); ++j )
		{
			int64_t const sym = symar[j];
			uint64_t const ra = i ? IHWT.rank(sym,i-1) : 0;
			uint64_t const rb = IHWT.rankm(sym,i);
			assert ( ra == rb );
		}

		for ( uint64_t j = 0; j < symar.size(); ++j )
		{
			int64_t const sym = symar[j];
			assert ( IHWT.rankm(sym,i) == rmap[sym] );
		}

		assert ( IHWT.inverseSelect(i).second == IHWT.rankm(text[i],i) );
		assert ( IHWT.inverseSelect(i).second == rmap[text[i]] );
		
		++rmap [ IHWT[i] ];
		
		// std::cerr << "i=" << i << " IHWT[i]=" << IHWT[i] << " r=" << r << " IHWT.rank()=" << IHWT.rank(IHWT[i],i) << std::endl;
		
		for ( uint64_t j = 0; j < symar.size(); ++j )
		{
			int64_t const sym = symar[j];
			assert ( IHWT.rank(sym,i) == rmap[sym] );
		}
		
		assert ( IHWT.inverseSelect(i).first == text[i] );
		
		// std::cerr << IHWT.inverseSelect(i).second << "\t" << rmap[text[i]] << std::endl;

		assert ( IHWT.inverseSelect(i).second + 1 == IHWT.rank(text[i],i) );
		assert ( IHWT.inverseSelect(i).second + 1 == rmap[text[i]] );

		// assert ( IHWT.select ( text[i], IHWT.rank(text[i],i)-1 ) == i );
	}
	#if 0
	std::cerr << std::endl;
	#endif
}

void testHuffmanWaveletSer()
{
	// std::string text = "Hello world.";
	std::string text = "fischers fritze fischt frische fische";
	::libmaus2::util::shared_ptr< ::libmaus2::huffman::HuffmanTreeNode >::type sroot = ::libmaus2::huffman::HuffmanBase::createTree(text.begin(),text.end());
	
	::libmaus2::util::TempFileNameGenerator tmpgen("tmphuf",3);
	// ::libmaus2::wavelet::ImpExternalWaveletGeneratorHuffman exgen(sroot.get(), tmpgen);
	::libmaus2::wavelet::ImpExternalWaveletGeneratorHuffmanParallel exgen(sroot.get(), tmpgen, 1);
	for ( uint64_t i = 0; i < text.size(); ++i )
		exgen[0].putSymbol(text[i]);
		// exgen.putSymbol(text[i]);
	exgen.createFinalStream("hufwuf");

	std::ifstream istr("hufwuf",std::ios::binary);
	::libmaus2::wavelet::ImpHuffmanWaveletTree IHWT(istr);
	
	for ( uint64_t i = 0; i < IHWT.size(); ++i )
		std::cerr << static_cast<char>(IHWT[i]);
	std::cerr << std::endl;
	for ( uint64_t i = 0; i < IHWT.size(); ++i )
	{
		std::cerr 
			<< static_cast<char>(IHWT.inverseSelect(i).first)
			<< "("
			<< IHWT.inverseSelect(i).second
			<< ")"
			<< "["
			<< IHWT.rank(text[i],i)
			<< "]";
		assert ( IHWT.inverseSelect(i).second + 1 == IHWT.rank(text[i],i) );
		assert ( static_cast<int64_t>(IHWT[i]) == text[i] );
	}
	std::cerr << std::endl;
}

#include <libmaus2/util/MemTempFileContainer.hpp>
#include <libmaus2/wavelet/ImpExternalWaveletGeneratorCompactHuffmanParallel.hpp>

void testCompactHuffman()
{
	std::map<int64_t,uint64_t> F;
	F[0] = 1;
	F[1] = 1;
	F[2] = 1;
	F[3] = 1;
	libmaus2::huffman::HuffmanTree H(F.begin(),F.size(),false,true);
	
	// std::cerr << H;
	
	libmaus2::util::MemTempFileContainer MTFC;
	libmaus2::wavelet::ImpExternalWaveletGeneratorCompactHuffman IEWGHN(H,MTFC);
	// libmaus2::util::TempFileNameGenerator tmpgen("tmpdir",2);
	// libmaus2::wavelet::ImpExternalWaveletGeneratorCompactHuffmanParallel IEWGHN(H,tmpgen,8);
	
	// std::cerr << "left construction." << std::endl;

	uint64_t A[] = { 0,0,0,0,0,0,3,3,1,3,2,1,2,1,1,2,1,1,1,2,1 };
	uint64_t const n = sizeof(A)/sizeof(A[0]);
	// uint64_t A[] = { 0,0,0,0,0,0  };
	
	for ( uint64_t i = 0; i < n; ++i )
		IEWGHN.putSymbol(A[i]);
	
	std::ostringstream ostr;
	IEWGHN.createFinalStream(ostr);
	std::istringstream istr(ostr.str());
	libmaus2::wavelet::ImpCompactHuffmanWaveletTree IHWTN(istr);
	
	// std::cerr << IHWTN.size() << std::endl;
	assert ( IHWTN.size() == n );
	
	std::map<uint64_t,uint64_t> R;

	for ( uint64_t i = 0; i < IHWTN.size(); ++i )
	{
		std::cerr << IHWTN[i] << ";";
		assert ( IHWTN[i] == static_cast<int64_t>(A[i]) );

		// std::cerr << "[" << i << "," << IHWTN.select(A[i],R[A[i]]) << "]" << ";";
		assert ( i == IHWTN.select(A[i],R[A[i]]) );

		assert ( IHWTN.rankm(A[i],i) == R[A[i]] );
		R[A[i]]++;
		assert ( IHWTN.rank (A[i],i) == R[A[i]] );
	}
	std::cerr << std::endl;
	
	for ( uint64_t i = 0; i <= IHWTN.size(); ++i )
		for ( uint64_t j = i; j <= IHWTN.size(); ++j )
		{
			assert ( IHWTN.enumerateSymbolsInRange(i,j) == IHWTN.enumerateSymbolsInRangeSlow(i,j) );
		}

	// ImpExternalWaveletGeneratorCompactHuffman(libmaus2::huffman::HuffmanTree const & rH, ::libmaus2::util::TempFileContainer & rtmpcnt)

}

void testCompactHuffmanPar()
{
	std::vector<uint8_t> A;
	std::map<int64_t,uint64_t> F;
	// uint64_t const n = 1024*1024;
	uint64_t const n = 64*1024*1024;
	for ( uint64_t i = 0; i < n; ++i )
	{
		A.push_back(libmaus2::random::Random::rand8() & 0xFF);
		F[A.back()]++;
	}
	libmaus2::huffman::HuffmanTree H(F.begin(),F.size(),false,true);
	
	std::cerr << H;
	
	libmaus2::util::MemTempFileContainer MTFC;
	// libmaus2::wavelet::ImpExternalWaveletGeneratorCompactHuffman IEWGHN(H,MTFC);
	libmaus2::util::TempFileNameGenerator tmpgen("tmpdir",2);
	#if defined(_OPENMP)
	uint64_t const numthreads = omp_get_max_threads();
	#else
	uint64_t const numthreads = 1;
	#endif
	libmaus2::wavelet::ImpExternalWaveletGeneratorCompactHuffmanParallel IEWGHN(H,tmpgen,numthreads);
	
	// std::cerr << "left construction." << std::endl;

	#if 0
	uint64_t A[] = { 0,0,0,0,0,0,3,3,1,3,2,1,2,1,1,2,1,1,1,2,1 };
	uint64_t const n = sizeof(A)/sizeof(A[0]);
	// uint64_t A[] = { 0,0,0,0,0,0  };
	#endif
	
	#if 0
	uint64_t const perthread = (n + numthreads-1)/numthreads;
	#endif
	
	#if defined(_OPENMP)
	#pragma omp parallel for
	#endif
	for ( int64_t i = 0; i < static_cast<int64_t>(n); ++i )
	{
		#if defined(_OPENMP)
		uint64_t const tid = omp_get_thread_num();
		#else
		uint64_t const tid = 0;
		#endif
		IEWGHN[tid].putSymbol(A[i]);
	}
	
	std::string tmpfilename = "tmp.hwt";
	// std::ostringstream ostr;
	libmaus2::aio::CheckedOutputStream COS(tmpfilename);
	IEWGHN.createFinalStream(COS);
	COS.close();
	// std::istringstream istr(ostr.str());
	// libmaus2::wavelet::ImpCompactHuffmanWaveletTree IHWTN(tmpfilename);
	libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type pIHWTN(libmaus2::wavelet::ImpCompactHuffmanWaveletTree::load(tmpfilename));
	libmaus2::wavelet::ImpCompactHuffmanWaveletTree const & IHWTN = *pIHWTN;
	// libmaus2::wavelet::ImpCompactHuffmanWaveletTree IHWTN(istr);
	remove(tmpfilename.c_str());
	
	// std::cerr << IHWTN.size() << std::endl;
	assert ( IHWTN.size() == n );
	
	std::map<uint64_t,uint64_t> R;

	for ( uint64_t i = 0; i < IHWTN.size(); ++i )
	{
		if ( i % (32*1024) == 0 )
			std::cerr << static_cast<double>(i) / n << std::endl;
		// std::cerr << IHWTN[i] << ";";
		assert ( IHWTN[i] == A[i] );

		// std::cerr << "[" << i << "," << IHWTN.select(A[i],R[A[i]]) << "]" << ";";
		assert ( i == IHWTN.select(A[i],R[A[i]]) );

		assert ( IHWTN.rankm(A[i],i) == R[A[i]] );
		R[A[i]]++;
		assert ( IHWTN.rank (A[i],i) == R[A[i]] );
	}
	std::cerr << std::endl;
	
	if ( n <= 128 )
	{
		#if defined(_OPENMP)
		#pragma omp parallel for
		#endif
		for ( uint64_t i = 0; i <= IHWTN.size(); ++i )
			for ( uint64_t j = i; j <= IHWTN.size(); ++j )
			{
				assert ( IHWTN.enumerateSymbolsInRange(i,j) == IHWTN.enumerateSymbolsInRangeSlow(i,j) );
			}
	}

	// ImpExternalWaveletGeneratorCompactHuffman(libmaus2::huffman::HuffmanTree const & rH, ::libmaus2::util::TempFileContainer & rtmpcnt)

}

int main()
{
	testCompactHuffmanPar();
	
	return 0;
	
	#if 0
	::libmaus2::wavelet::ImpHuffmanWaveletTree::unique_ptr_type IMP(new ::libmaus2::wavelet::ImpHuffmanWaveletTree(std::cin));
	::libmaus2::autoarray::AutoArray<uint32_t>::unique_ptr_type Z(new ::libmaus2::autoarray::AutoArray<uint32_t>(64));
	::libmaus2::lf::LFZeroImp L(IMP,Z,0);
	#endif
	
	#if 0
	LFZeroTemplate (
        wt_ptr_type & rW,
        z_array_ptr_type & rZ,
        uint64_t const rp0rank
        )
    	#endif                                                                                                            

	// testImpExternalWaveletGenerator();
	testHuffmanWavelet();
	testHuffmanWaveletSer();
	

	#if 0
	srand(time(0));

	uint64_t const b = 5;
	::libmaus2::util::TempFileNameGenerator tmpgen(std::string("tmp"),3);
	::libmaus2::wavelet::ExternalWaveletGenerator ex(b,tmpgen);

	std::vector < uint64_t > V;	
	for ( uint64_t i = 0; i < 381842; ++i )
	{
		uint64_t const v = rand() % (1ull<<b);
		// uint64_t const v = i % (1ull<<b);
		ex.putSymbol(v);
		V.push_back(v);
	}
	
	std::string const outfilename = "ex";
	uint64_t const n = ex.createFinalStream(outfilename);
	
	::std::ifstream istr(outfilename.c_str(), std::ios::binary);
	::libmaus2::wavelet::WaveletTree < ::libmaus2::rank::ERank222B, uint64_t > WT(istr);
	
	std::cerr << "Checking...";
	for ( uint64_t i = 0; i < n; ++i )
		assert ( WT[i] == V[i] );
	std::cerr << "done." << std::endl;
		
	if ( n < 256 )
	{
		for ( uint64_t i = 0; i < n; ++i )
			std::cerr << WT[i] << ";";
		std::cerr << std::endl;
	}
	#endif
}
