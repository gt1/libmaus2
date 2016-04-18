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
#include <libmaus2/rank/DNARank.hpp>
#include <libmaus2/rank/DNARankBiDir.hpp>
#include <libmaus2/util/ArgParser.hpp>
#include <libmaus2/huffman/RLEncoder.hpp>
#include <libmaus2/aio/OutputStreamInstance.hpp>
#include <libmaus2/wavelet/ImpCompactHuffmanWaveletTree.hpp>
#include <libmaus2/parallel/NumCpus.hpp>

int main(int argc, char * argv[])
{
	try
	{
		libmaus2::util::ArgParser const arg(argc,argv);

		uint64_t const numthreads = libmaus2::parallel::NumCpus::getNumLogicalProcessors();

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
			libmaus2::rank::DNARank::unique_ptr_type Prank(libmaus2::rank::DNARank::loadFromRunLength(memfn,numthreads));
		}

		libmaus2::timing::RealTimeClock rtc;
		uint64_t R0[4];
		uint64_t R1[4];
		double t;

		std::string const fn = arg[0];
		libmaus2::rank::DNARank::unique_ptr_type Prank(libmaus2::rank::DNARank::loadFromRunLength(fn,numthreads));

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
		//return 0;
		// Prank->testBackwardSearchConsistency(8);

		std::string const hfn = arg[1];
		//uint64_t const numthreads = libmaus2::parallel::NumCpus::getNumLogicalProcessors();
		libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type hptr(libmaus2::wavelet::ImpCompactHuffmanWaveletTree::load(hfn,numthreads));
		libmaus2::autoarray::AutoArray<char> C(Prank->size());
		#if defined(_OPENMP)
		#pragma omp parallel for
		#endif
		for ( uint64_t i = 0; i < Prank->size(); ++i )
			C[i] = (*Prank)[i];

		libmaus2::autoarray::AutoArray<uint64_t> RA(256*1024*1024,false);
		for ( uint64_t i = 0; i < RA.size(); ++i )
			RA[i] = libmaus2::random::Random::rand64() % Prank->size();

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
				assert ( Prank->rankm(i,j) == R0[j] );
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

		std::string const fn1 = arg[2];
		libmaus2::rank::DNARankBiDir::unique_ptr_type Pbidir(libmaus2::rank::DNARankBiDir::loadFromRunLength(fn,fn1,numthreads));
		Pbidir->testSearch(8,numthreads);
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
