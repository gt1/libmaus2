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
#include <libmaus2/aio/ArrayFile.hpp>
#include <libmaus2/suffixsort/bwtb3m/BwtMergeSort.hpp>
#include <libmaus2/util/ArgParser.hpp>

template<typename iterator>
void transform(iterator ita, iterator ite, bool timediv)
{
	double divsufsorttime = 0;

	if ( timediv )
	{
		/**
		 * run divsufsort for speed comparison, not because we use the result for the BWT construction
		 **/
		std::cerr << "[V] running divsufsort...";

		libmaus2::timing::RealTimeClock rtc;
		rtc.start();
		libmaus2::autoarray::AutoArray<uint8_t> A(ite-ita,false);
		std::copy(ita,ite,A.begin());
		typedef ::libmaus2::suffixsort::DivSufSort<64,uint8_t *,uint8_t const *,int64_t *,int64_t const *,256,false /* parallel */> sort_type;
		libmaus2::autoarray::AutoArray<int64_t> SA(ite-ita,false);
		sort_type::divsufsort(A.begin(),SA.begin(),ite-ita);
		divsufsorttime = rtc.getElapsedSeconds();

		std::cerr << std::endl;
	}

	// create file from array
	libmaus2::aio::ArrayFile<iterator> AF(ita,ite);

	{
		// check file
		libmaus2::aio::InputStreamInstance::unique_ptr_type ISI(AF.open());
		for ( uint64_t i = 0; i < ite-ita; ++i )
			assert ( ISI->peek() != std::istream::traits_type::eof() && static_cast<char>(ISI->get()) == ita[i] );
	}

	uint64_t const numthreads = libmaus2::suffixsort::bwtb3m::BwtMergeSortOptions::getDefaultNumThreads();
	libmaus2::suffixsort::bwtb3m::BwtMergeSortOptions options(
		AF.getURL(),
		16*1024ull*1024ull*1024ull, /* memory 16GB */
		numthreads,
		"bytestream",
		false /* bwtonly */,
		std::string("mem:tmp_"),
		std::string(), // sparse
		std::string("mem:file.bwt"),
		32 /* isa */,
		32 /* sa */
	);

	// construct BWT, SA and ISA
	libmaus2::suffixsort::bwtb3m::BwtMergeSortResult res = libmaus2::suffixsort::bwtb3m::BwtMergeSort::computeBwt(options,&std::cerr);

	// load LF object
	libmaus2::lf::ImpCompactHuffmanWaveletLF::unique_ptr_type PLF = res.loadLF("mem://tmp_",numthreads);
	uint64_t const n = PLF->W->size();

	// construct FM object
	libmaus2::fm::FM<libmaus2::lf::ImpCompactHuffmanWaveletLF>::unique_ptr_type PFM(res.loadFM("mem://tmp",numthreads));

	// extract text
	std::string r(n,' ');
	PFM->extractIteratorParallel(0,n,r.begin(),numthreads);

	// check text
	assert ( ::std::equal(ita,ite,r.begin()) );

	#if 0
	// print the BWT
	for ( uint64_t i = 0; i < n; ++i )
		std::cerr << static_cast<char>((*PLF)[i]);
	std::cerr << std::endl;

	// print the text
	libmaus2::fm::SampledISA<libmaus2::lf::ImpCompactHuffmanWaveletLF>::unique_ptr_type SISA(res.loadInverseSuffixArray(PLF.get()));
	for ( uint64_t i = 0; i < n; ++i )
		std::cerr << static_cast<char>((*PLF)[ (*SISA)[(i+1)%n] ]);
	std::cerr << std::endl;
	#endif

	libmaus2::suffixtree::CompressedSuffixTree::unique_ptr_type CST(res.loadSuffixTree(numthreads,"mem://tmp",32*1024*1024,&(std::cerr)));

	if ( timediv )
		std::cerr << "[S] time for divsufsort " << divsufsorttime << std::endl;
}

int main(int argc, char * argv[])
{
	try
	{
		libmaus2::util::ArgParser arg(argc,argv);

		bool const timediv = arg.uniqueArgPresent("timediv");

		std::string const s = "hello world, hello moon";
		transform(s.begin(),s.end(),timediv);

		for ( uint64_t i = 0; i < arg.size(); ++i )
		{
			libmaus2::autoarray::AutoArray<char> const A = libmaus2::autoarray::AutoArray<char>::readFile(arg[i]);
			transform(A.begin(),A.end(),timediv);
		}
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
