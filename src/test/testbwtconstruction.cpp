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

std::string pointerToString(void * vp)
{
	std::ostringstream protstr;
	protstr << vp;
	std::string prot = protstr.str();
	assert ( prot.size() >= 2 && prot.substr(0,2) == "0x" );
	prot = prot.substr(2);
	for ( uint64_t i = 0; i < prot.size(); ++i )
		if ( ::std::isalpha(prot[i]) )
		{
			prot[i] = ::std::tolower(prot[i]);
			assert ( prot[i] >= 'a' );
			assert ( prot[i] <= 'f' );
			char const dig = prot[i] - 'a' + 10;
			prot[i] = dig + 'a';
		}
		else
		{
			assert ( ::std::isdigit(prot[i]) );
			char const dig = prot[i] - '0';
			prot[i] = dig + 'a';
		}
	return prot;
}

template<typename iterator>
void transform(iterator ita, iterator ite)
{
	// protocol (alpha characters only)
	std::string const prot = std::string("array") + pointerToString(&ita);
	// container
	libmaus2::aio::ArrayFileContainer<iterator> container;
	// add file
	container.add("file",ita,ite);
	// set up factory
	typename libmaus2::aio::ArrayInputStreamFactory<iterator>::shared_ptr_type factory(
		new libmaus2::aio::ArrayInputStreamFactory<iterator>(container));
	// add protocol handler
	libmaus2::aio::InputStreamFactoryContainer::addHandler(prot, factory);

	try
	{
		// file url
		std::string const url = prot + ":file";

		{
		// check file
		libmaus2::aio::InputStreamInstance ISI(url);
		for ( uint64_t i = 0; i < ite-ita; ++i )
			assert ( ISI.peek() != std::istream::traits_type::eof() && ISI.get() == ita[i] );
		}

		uint64_t const numthreads = libmaus2::suffixsort::bwtb3m::BwtMergeSortOptions::getDefaultNumThreads();
		libmaus2::suffixsort::bwtb3m::BwtMergeSortOptions options(
			url,
			libmaus2::suffixsort::bwtb3m::BwtMergeSortOptions::getDefaultMem(),
			numthreads,
			"bytestream",
			false /* bwtonly */,
			std::string("mem:tmp_"),
			std::string(), // sparse
			std::string("mem:file.bwt"),
			32 /* isa */,
			32 /* sa */
		);

		libmaus2::suffixsort::bwtb3m::BwtMergeSortResult res = libmaus2::suffixsort::bwtb3m::BwtMergeSort::computeBwt(options,&std::cerr);

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

		assert ( r == std::string(ita,ite) );

		assert ( ::std::equal(ita,ite,r.begin()) );

		#if 0
		libmaus2::fm::SampledISA<libmaus2::lf::ImpCompactHuffmanWaveletLF>::unique_ptr_type SISA(res.loadInverseSuffixArray(PLF.get()));
		for ( uint64_t i = 0; i < n; ++i )
			std::cerr << static_cast<char>((*PLF)[ (*SISA)[(i+1)%n] ]);
		std::cerr << std::endl;
		#endif

		libmaus2::aio::InputStreamFactoryContainer::removeHandler(prot);
	}
	catch(...)
	{
		libmaus2::aio::InputStreamFactoryContainer::removeHandler(prot);
		throw;
	}
}

void testArrayInput(std::string const & s)
{
	libmaus2::aio::ArrayInputStream<std::string::const_iterator> AIS(s.begin(),s.end());

	for ( uint64_t i = 0; i < s.size(); ++i )
		assert ( AIS.peek() != std::istream::traits_type::eof() && AIS.get() == s[i] );

	for ( uint64_t i = 0; i < s.size(); ++i )
	{
		AIS.clear();
		AIS.seekg(i);
		for ( uint64_t j = i; j < s.size(); ++j )
			assert ( AIS.peek() != std::istream::traits_type::eof() && AIS.get() == s[j] );
	}

	libmaus2::aio::ArrayFileContainer<std::string::const_iterator> container;
	container.add("file",s.begin(),s.end());
	libmaus2::aio::ArrayInputStreamFactory<std::string::const_iterator>::shared_ptr_type factory(
		new libmaus2::aio::ArrayInputStreamFactory<std::string::const_iterator>(container));

	libmaus2::aio::InputStream::unique_ptr_type Pistr(factory->constructUnique("file"));
	for ( uint64_t i = 0; i < s.size(); ++i )
		assert ( Pistr->peek() != std::istream::traits_type::eof() && Pistr->get() == s[i] );
	libmaus2::aio::InputStream::shared_ptr_type Sistr(factory->constructShared("file"));
	for ( uint64_t i = 0; i < s.size(); ++i )
		assert ( Sistr->peek() != std::istream::traits_type::eof() && Sistr->get() == s[i] );

	libmaus2::aio::InputStreamFactoryContainer::addHandler("array", factory);
	libmaus2::aio::InputStreamInstance ISI("array:file");
	for ( uint64_t i = 0; i < s.size(); ++i )
		assert ( ISI.peek() != std::istream::traits_type::eof() && ISI.get() == s[i] );
	libmaus2::aio::InputStreamFactoryContainer::removeHandler("array");
}

int main(int argc, char * argv[])
{
	try
	{
		std::string const s = "hello world, hello moon";
		testArrayInput(s);
		transform(s.begin(),s.end());

		libmaus2::util::ArgParser arg(argc,argv);
		for ( uint64_t i = 0; i < arg.size(); ++i )
		{
			libmaus2::autoarray::AutoArray<char> const A = libmaus2::autoarray::AutoArray<char>::readFile(arg[i]);
			transform(A.begin(),A.end());
		}
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
