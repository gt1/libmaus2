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
#include <libmaus/bitio/CompactDecoderBuffer.hpp>
#include <libmaus/util/TempFileRemovalContainer.hpp>
#include <libmaus/bitio/CompactArrayWriter.hpp>
#include <libmaus/bitio/Clz.hpp>

void testctz()
{
	for ( uint64_t i = 1, j = 0; i; i <<=1, ++j )
		assert ( ::libmaus::bitio::Ctz::ctz(i) == j );
}

void testcompact()
{
	std::string const fn("tmpfile");
	#if 0
	std::string const fn2("tmpfile2");
	std::string const fnm("tmpfile.merged");
	#endif

	::libmaus::util::TempFileRemovalContainer::setup();
	::libmaus::util::TempFileRemovalContainer::addTempFile(fn);
	
	uint64_t n = 1024*1024;
	unsigned int const b = 3;
	::libmaus::bitio::CompactArray CA(n,b);
	::libmaus::bitio::CompactArrayWriter CAW(fn,n,b);
	srand(time(0));
	for ( uint64_t i = 0; i < n; ++i )
	{
		CA.set(i,rand() & ((1ull<<b)-1));
		CAW.put(CA.get(i));
	}
	CAW.flush();
	#if 0
	::libmaus::aio::CheckedOutputStream COS(fn);
	CA.serialize(COS);
	COS.flush();
	COS.close();
	#endif
	
	::libmaus::aio::CheckedInputStream CIS(fn);
	std::cerr << "compact file size is " << ::libmaus::util::GetFileSize::getFileSize(CIS) << std::endl;
	assert ( CIS.tellg() == static_cast< ::std::streampos >(0) );
	assert ( CIS.get() >= 0 );
	
	::libmaus::bitio::CompactDecoderWrapper W(fn,4096);
	
	W.seekg(0,std::ios::end);
	int64_t const fs = W.tellg();
	W.seekg(0,std::ios::beg);
	W.clear();
	
	assert ( fs == static_cast<int64_t>(n) );
	
	std::cerr << "n=" << n << " fs=" << fs << std::endl;
	
	for ( uint64_t i = 0; i < n; ++i )
	{
		assert ( W.tellg() == static_cast< ::std::streampos >(i) );
		int const v = W.get();
		assert ( v == static_cast<int>(CA[i]) );
		// std::cerr << static_cast<int>(W.get()) << " " << CA[i] << std::endl;
	}
	
	for ( uint64_t i = 0; i < n; i += (rand() % 256) )
	{
		W.clear();
		W.seekg(i);
		
		std::cerr << "seek to " << W.tellg() << std::endl;
		
		for ( uint64_t j = i; j < n; ++j )
		{
			assert ( W.tellg() == static_cast< ::std::streampos >(j) );
			int const v = W.get();
			assert ( v == static_cast<int>(CA[j]) );
		}
		
		uint64_t ii = n-i;
		W.clear();
		W.seekg(ii);

		for ( uint64_t j = ii; j < n; ++j )
		{
			assert ( W.tellg() == static_cast< ::std::streampos >(j) );
			int const v = W.get();
			assert ( v == static_cast<int>(CA[j]) );
		}
	}
}

/**
 * compute character histogram in parallel
 **/
::libmaus::autoarray::AutoArray<uint64_t> computeCharHist(std::string const & inputfile)
{
	uint64_t const n = ::libmaus::util::GetFileSize::getFileSize(inputfile);
	
	#if defined(_OPENMP)
	uint64_t const numthreads = omp_get_max_threads();
	#else
	uint64_t const numthreads = 1;
	#endif

	uint64_t const packsize = (n + numthreads-1)/numthreads;

	::libmaus::parallel::OMPLock lock;
	::libmaus::autoarray::AutoArray<uint64_t> ghist(256);	
	#if defined(_OPENMP)
	#pragma omp parallel for
	#endif
	for ( int64_t t = 0; t < static_cast<int64_t>(numthreads); ++t )
	{
		uint64_t const low  = std::min(n,t*packsize);
		uint64_t const high = std::min(n,low+packsize);
		uint64_t const range = high-low;
		
		if ( range )
		{
			::libmaus::autoarray::AutoArray<uint64_t> lhist(ghist.size());	
			::libmaus::aio::CheckedInputStream CIS(inputfile);
			CIS.seekg(low);
			uint64_t const blocksize = 8192;
			uint64_t const numblocks = ((range)+blocksize-1)/blocksize;
			::libmaus::autoarray::AutoArray<uint8_t> B(blocksize);
			
			for ( uint64_t b = 0; b < numblocks; ++b )
			{
				uint64_t const llow = std::min(low + b*blocksize,high);
				uint64_t const lhigh = std::min(llow + blocksize,high);
				uint64_t const lrange = lhigh-llow;
				CIS.read ( reinterpret_cast<char *>(B.begin()), lrange );
				assert ( CIS.gcount() == static_cast<int64_t>(lrange) );
				for ( uint64_t i = 0; i < lrange; ++i )
					lhist[B[i]]++;
			}

			lock.lock();
			for ( uint64_t i = 0; i < lhist.size(); ++i )
				ghist[i] += lhist[i];
			lock.unlock();
		}
	}
	
	return ghist;
}

int main(int argc, char * argv[])
{
	try
	{
		::libmaus::util::ArgInfo const arginfo(argc,argv);
		std::string const input = arginfo.getRestArg<std::string>(0);
		std::string const output = arginfo.getRestArg<std::string>(1);
		unsigned int const verbose = arginfo.getValue<unsigned int>("verbose",1);
		unsigned int const addterm = arginfo.getValue<unsigned int>("addterm",0) ? 1 : 0;

		::libmaus::autoarray::AutoArray<uint64_t> const chist = computeCharHist(input);
		uint64_t maxsym = 0;
		for ( uint64_t i = 0; i < chist.size(); ++i )
			if ( chist[i] )
				maxsym = i;
		if ( addterm )
			maxsym += 1;
		unsigned int const b = maxsym ? (64-::libmaus::bitio::Clz::clz(maxsym)) : 0;

		uint64_t const n = std::accumulate(chist.begin(),chist.end(),0ull);
		if ( verbose )
			std::cerr << "[V] n=" << n << " maxsym=" << maxsym << " b=" << b << std::endl;				

		uint64_t const blocksize = 8*1024;
		uint64_t const numblocks = (n+blocksize-1)/blocksize;
		::libmaus::autoarray::AutoArray<uint8_t> B(blocksize);
		::libmaus::aio::CheckedInputStream CIS(input);
		::libmaus::bitio::CompactArrayWriter CAW(output,n+addterm,b);
		int64_t lastperc = -1;
		
		if ( verbose )
			std::cerr << "[V] ";
			
		for ( uint64_t b = 0; b < numblocks; ++b )
		{
			uint64_t const low = std::min(b*blocksize,n);
			uint64_t const high = std::min(low+blocksize,n);
			uint64_t const range = high-low;
			
			CIS.read ( reinterpret_cast<char *>(B.begin()), range );
			assert ( CIS.gcount() == static_cast<int64_t>(range) );
			
			if ( addterm )
				for ( uint64_t i = 0; i < range; ++i )
					B[i] += 1;
			
			CAW.write(B.begin(),range);
			
			int64_t const newperc = (high * 100) / n;
			if ( verbose && newperc != lastperc )
			{
				lastperc = newperc;
				std::cerr << "(" << newperc << ")";
			}
		}
		if ( addterm )
			CAW.put(0);
		if ( verbose )
			std::cerr << std::endl;
		
		CAW.flush();
		
		#if 0
		::libmaus::bitio::CompactDecoderWrapper CDW(output);
		for ( uint64_t i = 0; i < n+addterm; ++i )
			std::cerr << CDW.get();
		std::cerr << std::endl;
		#endif
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
	}
}
