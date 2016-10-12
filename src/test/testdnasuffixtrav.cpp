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
#include <libmaus2/rank/DNARankSuffixTreeNodeEnumerator.hpp>

void transform(std::string const & fn)
{
	// generate 2 bit reprensentation of FastA file
	libmaus2::fastx::FastAToCompact4BigBandBiDir::fastaToCompact4BigBandBiDir(
		std::vector<std::string>(1,fn),
		&(std::cerr),
		false /* single strand */,
		std::string("mem://fasta.compact")
	);

	// generate options object
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

	// generate BWT/SA/ISA
	libmaus2::suffixsort::bwtb3m::BwtMergeSortResult res = libmaus2::suffixsort::bwtb3m::BwtMergeSort::computeBwt(options,&std::cerr);

	libmaus2::rank::DNARank::unique_ptr_type Prank(res.loadDNARank(numthreads));

	libmaus2::rank::DNARankSuffixTreeNodeEnumerator DNAit(*Prank);
	libmaus2::rank::DNARankSuffixTreeNodeEnumeratorQueueElement QE;

	std::cerr << "got sequence of length " << Prank->getN() << std::endl;
	libmaus2::autoarray::AutoArray<char> C;

	while ( DNAit.getNext(QE) )
	{
		C.ensureSize(QE.sdepth);
		Prank->decode(QE.intv.forw,QE.sdepth,C.begin());
		for ( uint64_t i = 0; i < QE.sdepth; ++i )
			C[i] = libmaus2::fastx::remapChar(C[i]);

		std::cout << std::string(C.begin(),C.begin()+QE.sdepth) << std::endl;
	}
}

int main(int argc, char * argv[])
{
	try
	{
		libmaus2::util::ArgParser arg(argc,argv);

		if ( arg.size() < 1 )
		{
			std::cerr << "usage: " << argv[0] << " <in.fasta>" << std::endl;
			return EXIT_FAILURE;
		}

		transform(arg[0]);
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
