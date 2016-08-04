/*
    libmaus2
    Copyright (C) 2015 German Tischler

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
#define LIBMAUS2_RANK_DNARANK_TESTFROMRUNLENGTH

#include <libmaus2/rank/DNARankSuffixTreeNodeEnumerator.hpp>
#include <libmaus2/rank/DNARank.hpp>
#include <libmaus2/rank/DNARankBiDir.hpp>
#include <libmaus2/util/ArgParser.hpp>
#include <libmaus2/huffman/RLEncoder.hpp>
#include <libmaus2/aio/OutputStreamInstance.hpp>
#include <libmaus2/wavelet/ImpCompactHuffmanWaveletTree.hpp>
#include <libmaus2/parallel/NumCpus.hpp>
#include <libmaus2/fastx/acgtnMap.hpp>
#include <libmaus2/wavelet/RlToHwtBase.hpp>

template<typename rank_type>
void testShort(uint64_t const numthreads)
{
	std::string const memfn = "mem://file";
	uint64_t const n = 160;
	{
	libmaus2::huffman::RLEncoderStd rlenc(memfn,n,64*1024);
	for ( uint64_t i = 0; i < n; ++i )
		rlenc.encode(i&3);
	}
	{
	libmaus2::huffman::RLDecoder rldec(std::vector<std::string>(1,memfn),0 /* offset */,numthreads);
	for ( uint64_t i = 0; i < n; ++i )
		assert ( rldec.decode() == static_cast<int>(i&3) );
	}
	typename rank_type::unique_ptr_type Prank(rank_type::loadFromRunLength(memfn,numthreads));
}

template<typename rank_type>
void testLong(std::string const & fn, uint64_t const numthreads)
{
	libmaus2::timing::RealTimeClock rtc;

	typename rank_type::unique_ptr_type Prank(rank_type::loadFromRunLength(fn,numthreads));

	std::cerr << "[V] checking extract...";
	libmaus2::autoarray::AutoArray<char> CC;
	for ( uint64_t i = 0; i < 64*1024; ++i )
	{
		uint64_t const p = libmaus2::random::Random::rand64() % Prank->size();
		uint64_t const s = std::min( (libmaus2::random::Random::rand64() % 512), Prank->size()-p );
		if ( s > CC.size() )
			CC.resize(s);
		Prank->extract(CC.begin(),p,s);
		for ( uint64_t j = 0; j < s; ++j )
			assert ( static_cast<char>((*Prank)[p+j]) == CC[j] );
	}
	std::cerr << "done." << std::endl;

	Prank->testSearch(8,numthreads);

	std::cerr << "[V] computing HWT...";
	::libmaus2::util::Histogram::unique_ptr_type mhist(libmaus2::wavelet::RlToHwtBase<false,libmaus2::huffman::RLDecoder>::computeRlSymHist(fn,numthreads));
	::std::map<int64_t,uint64_t> const chist = mhist->getByType<int64_t>();
	::libmaus2::huffman::HuffmanTree H ( chist.begin(), chist.size(), false, true, true );
	libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type hptr(libmaus2::wavelet::RlToHwtBase<false,libmaus2::huffman::RLDecoder>::rlToHwtSmallAlphabet<uint8_t>(fn,H,numthreads));
	std::cerr << "done." << std::endl;

	libmaus2::autoarray::AutoArray<char> C(Prank->size());
	#if defined(_OPENMP)
	#pragma omp parallel for
	#endif
	for ( uint64_t i = 0; i < Prank->size(); ++i )
		C[i] = (*Prank)[i];

	libmaus2::autoarray::AutoArray<uint64_t> RA(256*1024*1024,false);
	for ( uint64_t i = 0; i < RA.size(); ++i )
		RA[i] = libmaus2::random::Random::rand64() % Prank->size();

	uint64_t R0[4];
	uint64_t R1[4];
	double t;
	rtc.start();
	for ( uint64_t i = 0; i < RA.size(); ++i )
	{
		R0[0] = 0;
		R0[1] = 0;
		R0[2] = 0;
		R0[3] = 0;
		hptr->enumerateSymbolsRank(RA[i], &R0[0]);

		#if 0
		if ( i % (1024*1024) == 0 )
			std::cerr.put('.');
		#endif
	}
	t = rtc.getElapsedSeconds();
	std::cerr << "hwt random rank " << RA.size()/t << std::endl;

	rtc.start();
	for ( uint64_t i = 0; i < RA.size(); ++i )
	{
		Prank->rankm(RA[i], &R1[0]);

		#if 0
		if ( i % (1024*1024) == 0 )
			std::cerr.put('.');
		#endif
	}
	t = rtc.getElapsedSeconds();
	std::cerr << "DNArank random rank " << RA.size()/t << std::endl;

	rtc.start();
	for ( uint64_t i = 0; i < RA.size(); ++i )
	{
		Prank->rankm(RA[i], i%3);

		#if 0
		if ( i % (1024*1024) == 0 )
			std::cerr.put('.');
		#endif
	}
	t = rtc.getElapsedSeconds();
	std::cerr << "DNArank random rank spec " << RA.size()/t << std::endl;

	for ( uint64_t i = 0; i < Prank->size(); ++i )
	{
		R0[0] = 0;
		R0[1] = 0;
		R0[2] = 0;
		R0[3] = 0;
		hptr->enumerateSymbolsRank(i, &R0[0]);
		Prank->rankm(i, &R1[0]);
		for ( uint64_t j = 0; j < 4; ++j )
		{
			assert ( R0[j] == R1[j] );
			assert ( Prank->rankm(j,i) == R1[j] );
			assert ( Prank->rankm(j,i) == R0[j] );
		}
	}

	rtc.start();
	for ( uint64_t i = 0; i < Prank->size(); ++i )
	{
		R0[0] = 0;
		R0[1] = 0;
		R0[2] = 0;
		R0[3] = 0;
		hptr->enumerateSymbolsRank(i, &R0[0]);

		#if 0
		if ( i % (1024*1024) == 0 )
			std::cerr.put('.');
		#endif
	}
	t = rtc.getElapsedSeconds();
	std::cerr << "hwt rank " << Prank->size()/t << std::endl;

	rtc.start();
	for ( uint64_t i = 0; i < Prank->size(); ++i )
	{
		Prank->rankm(i, &R1[0]);

		#if 0
		if ( i % (1024*1024) == 0 )
			std::cerr.put('.');
		#endif
	}
	t = rtc.getElapsedSeconds();
	std::cerr << "prank rank " << Prank->size()/t << std::endl;

	rtc.start();
	for ( uint64_t i = 0; i < Prank->size(); ++i )
		assert ( (*hptr)[i] == C[i] );
	t = rtc.getElapsedSeconds();
	std::cerr << "hwt access " << Prank->size()/t << std::endl;

	rtc.start();
	for ( uint64_t i = 0; i < Prank->size(); ++i )
		assert ( static_cast<char>((*Prank)[i]) == static_cast<char>(C[i]) );
	t = rtc.getElapsedSeconds();
	std::cerr << "DNARank access " << Prank->size()/t << std::endl;
}

int main(int argc, char * argv[])
{
	try
	{
		libmaus2::util::ArgParser const arg(argc,argv);

		uint64_t const numthreads = libmaus2::parallel::NumCpus::getNumLogicalProcessors();

		std::cerr << "[V] running short tests...\n";
		testShort<libmaus2::rank::DNARank>(numthreads);
		testShort<libmaus2::rank::DNARank128>(numthreads);
		testShort<libmaus2::rank::DNARank256>(numthreads);
		std::cerr << "[V] short tests done" << std::endl;

		if ( 0 < arg.size() )
		{
			std::string const fn = arg[0];
			std::cerr << "[V] running long tests...\n";
			testLong<libmaus2::rank::DNARank>(fn,numthreads);
			testLong<libmaus2::rank::DNARank128>(fn,numthreads);
			testLong<libmaus2::rank::DNARank256>(fn,numthreads);
			std::cerr << "[V] long tests done" << std::endl;
		}

		#if 0
		libmaus2::rank::DNARank::unique_ptr_type Prank(libmaus2::rank::DNARank::loadFromRunLength(fn,numthreads));
		{
			libmaus2::rank::DNARankSuffixTreeNodeEnumerator enumer(*Prank);
			libmaus2::rank::DNARankSuffixTreeNodeEnumeratorQueueElement E;
			unsigned int const k = 8;
			while ( enumer.getNextBFS(E) && E.sdepth <= k )
			{
				if ( E.sdepth == k )
					std::cerr << libmaus2::fastx::remapString(Prank->decode(E.intv.forw, E.sdepth)) << " " << E.intv.size << std::endl;
			}
		}
		#endif

		#if 0
		std::string const fn1 = arg[2];
		libmaus2::rank::DNARankBiDir::unique_ptr_type Pbidir(libmaus2::rank::DNARankBiDir::loadFromRunLength(fn,fn1,numthreads));
		Pbidir->testSearch(8,numthreads);
		#endif
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
