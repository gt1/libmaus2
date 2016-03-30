/*
    libmaus2
    Copyright (C) 2016 German Tischler

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
#include <libmaus2/aio/ArrayInputStream.hpp>
#include <libmaus2/aio/ArrayFileContainer.hpp>
#include <libmaus2/aio/ArrayInputStreamFactory.hpp>
#include <libmaus2/aio/InputStreamFactoryContainer.hpp>
#include <libmaus2/suffixsort/bwtb3m/BwtMergeSort.hpp>
#include <libmaus2/util/ArgParser.hpp>
#include <libmaus2/fastx/FastAToCompact4BigBandBiDir.hpp>

void transform(std::string const & fn)
{
	libmaus2::fastx::FastAToCompact4BigBandBiDir::fastaToCompact4BigBandBiDir(
		std::vector<std::string>(1,fn),
		&(std::cerr),
		false /* single strand */,
		std::string("mem://fasta.compact")
	);

	uint64_t const numthreads = libmaus2::suffixsort::bwtb3m::BwtMergeSortOptions::getDefaultNumThreads();
	libmaus2::suffixsort::bwtb3m::BwtMergeSortOptions options(
		"mem://fasta.compact",
		16*1024ull*1024ull*1024ull,
		// libmaus2::suffixsort::bwtb3m::BwtMergeSortOptions::getDefaultMem(),
		numthreads,
		"compactstream",
		false /* bwtonly */,
		std::string("mem:tmp_"),
		std::string(), // sparse
		std::string("mem:file.bwt"),
		32 /* isa */,
		32 /* sa */
	);


	libmaus2::suffixsort::bwtb3m::BwtMergeSortResult res = libmaus2::suffixsort::bwtb3m::BwtMergeSort::computeBwt(options,&std::cerr);

	libmaus2::rank::DNARank::unique_ptr_type Prank(res.loadDNARank(numthreads));

	std::cerr << "got sequence of length " << Prank->getN() << std::endl;

	libmaus2::lcp::WaveletLCPResult::unique_ptr_type WLCP(libmaus2::lcp::WaveletLCP::computeLCP(Prank.get(),numthreads,false /* zdif */,&(std::cerr)));

	libmaus2::suffixtree::CompressedSuffixTree::unique_ptr_type CST(res.loadSuffixTree(numthreads,"mem://tmp",32*1024*1024,&(std::cerr)));

	#if defined(_OPENMP)
	#pragma omp parallel for num_threads(numthreads)
	#endif
	for ( uint64_t i = 0; i < Prank->getN(); ++i )
		assert (
			(*WLCP)[i] ==
			(*(CST->LCP))[i]
		);
	std::cerr << "checked lcp" << std::endl;

	#if 0
	libmaus2::lf::ImpCompactHuffmanWaveletLF::unique_ptr_type PLF = res.loadLF("mem://tmp_",numthreads);
	uint64_t const n = PLF->W->size();

	#if 0
	// print the BWT
	for ( uint64_t i = 0; i < n; ++i )
		std::cerr << static_cast<char>((*PLF)[i]);
	std::cerr << std::endl;
	#endif

	libmaus2::fm::FM<libmaus2::lf::ImpCompactHuffmanWaveletLF>::unique_ptr_type PFM(res.loadFM("mem://tmp",numthreads));

	// extract text
	std::string r(n,' ');
	PFM->extractIteratorParallel(0,n,r.begin(),numthreads);

	// assert ( ::std::equal(ita,ite,r.begin()) );

	#if 0
	libmaus2::fm::SampledISA<libmaus2::lf::ImpCompactHuffmanWaveletLF>::unique_ptr_type SISA(res.loadInverseSuffixArray(PLF.get()));
	for ( uint64_t i = 0; i < n; ++i )
		std::cerr << static_cast<char>((*PLF)[ (*SISA)[(i+1)%n] ]);
	std::cerr << std::endl;
	#endif

	libmaus2::suffixtree::CompressedSuffixTree::unique_ptr_type CST(res.loadSuffixTree(numthreads,"mem://tmp",32*1024*1024,&(std::cerr)));
	#endif

	//std::cerr << "[S] time for divsufsort " << divsufsorttime << std::endl;
}

int main(int argc, char * argv[])
{
	try
	{
		libmaus2::util::ArgParser arg(argc,argv);
		for ( uint64_t i = 0; i < arg.size(); ++i )
			transform(arg[i]);
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
