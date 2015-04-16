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
#include <libmaus2/util/Utf8BlockIndex.hpp>

libmaus2::util::Utf8BlockIndex::Utf8BlockIndex()
{

}

std::string libmaus2::util::Utf8BlockIndex::serialise() const
{
	::std::ostringstream ostr;
	serialise(ostr);
	return ostr.str();
}

libmaus2::util::Utf8BlockIndex::unique_ptr_type libmaus2::util::Utf8BlockIndex::constructFromSerialised(std::string const & fn)
{
	::libmaus2::aio::CheckedInputStream CIS(fn);

	unique_ptr_type UP(new Utf8BlockIndex);

	UP->blocksize = ::libmaus2::util::NumberSerialisation::deserialiseNumber(CIS);
	UP->lastblocksize = ::libmaus2::util::NumberSerialisation::deserialiseNumber(CIS);
	UP->maxblockbytes = ::libmaus2::util::NumberSerialisation::deserialiseNumber(CIS);
	uint64_t const numblocks = ::libmaus2::util::NumberSerialisation::deserialiseNumber(CIS);
	
	UP->blockstarts = ::libmaus2::autoarray::AutoArray<uint64_t>(numblocks+1,false);
	
	for ( uint64_t i = 0; i < UP->blockstarts.size(); ++i )
		UP->blockstarts[i] = ::libmaus2::util::NumberSerialisation::deserialiseNumber(CIS);
	
	
	return UNIQUE_PTR_MOVE(UP);
}

libmaus2::util::Utf8BlockIndex::unique_ptr_type libmaus2::util::Utf8BlockIndex::constructFromUtf8File(std::string const & fn, uint64_t const rblocksize)
{
	uint64_t const fs = ::libmaus2::util::GetFileSize::getFileSize(fn);
	

	#if defined(_OPENMP)
	uint64_t const numthreads = omp_get_max_threads();
	#else
	uint64_t const numthreads = 1;
	#endif
	uint64_t const numparts = 1024*numthreads;
	uint64_t const bblocksize = (fs + numparts-1)/numparts;
	uint64_t const numbblocks = (fs + bblocksize-1)/bblocksize;

	uint64_t const blocksize = ::libmaus2::math::nextTwoPow(rblocksize);
	unsigned int const blockshift = ::libmaus2::math::numbits(blocksize)-1;
	assert ( (1ull << blockshift) == blocksize );
	uint64_t const blockmask = blocksize-1;

	if ( ! fs )
	{
		unique_ptr_type UP(new Utf8BlockIndex);
		UP->blocksize = blocksize;
		UP->lastblocksize = 0;
		return UNIQUE_PTR_MOVE(UP);
	}

	
	::libmaus2::aio::CheckedInputStream::unique_ptr_type CIS(new ::libmaus2::aio::CheckedInputStream(fn));
	std::vector<uint64_t> preblockstarts;
	
	for ( uint64_t b = 0; b < numbblocks; ++b )
	{
		uint64_t low = b*bblocksize;
		
		CIS->clear();
		CIS->seekg(low);

		int c = -1;

		uint64_t p = low;
		while ( ((c=CIS->get()) >= 0) && ((c & 0xc0) == (0x80)) )
			++p;
			
		preblockstarts.push_back(p);
		/*
		template<typename in_type>
		static uint32_t decodeUTF8(in_type & istr, uint64_t & codelen)
		*/	                        			
	}
	
	// CIS.reset();
	
	preblockstarts.push_back(fs);
	::libmaus2::autoarray::AutoArray<uint64_t> bblocksyms(numbblocks+1,false);
	::libmaus2::autoarray::AutoArray< ::libmaus2::aio::CheckedInputStream::unique_ptr_type > thrstreams(numthreads,false);
	for ( uint64_t t = 0; t < numthreads; ++t )
	{
		::libmaus2::aio::CheckedInputStream::unique_ptr_type thrstreamst
                        (new ::libmaus2::aio::CheckedInputStream(fn));
		thrstreams[t] = UNIQUE_PTR_MOVE(thrstreamst);
	}
	
	#if defined(_OPENMP)
	#pragma omp parallel for
	#endif
	for ( int64_t b = 0; b < static_cast<int64_t>(numbblocks); ++b )
	{
		#if defined(_OPENMP)
		uint64_t const threadid = omp_get_thread_num();
		#else
		uint64_t const threadid = 0;
		#endif
		
		::libmaus2::aio::CheckedInputStream * CIS = thrstreams[threadid].get();
		CIS->clear();
		CIS->seekg(preblockstarts[b]);
		
		uint64_t codelen = 0;
		uint64_t syms = 0;
		uint64_t const bblocksize = preblockstarts[b+1]-preblockstarts[b];
		
		while ( codelen != bblocksize )
		{
			::libmaus2::util::UTF8::decodeUTF8(*CIS, codelen);
			syms++;
		}
		
		assert ( syms <= bblocksize );
		
		bblocksyms[b] = syms;			
	}
	
	bblocksyms.prefixSums();
	
	uint64_t const numsyms = bblocksyms[bblocksyms.size()-1];
	uint64_t const numblocks = (numsyms + blocksize-1)/blocksize;
	
	::libmaus2::autoarray::AutoArray<uint64_t> blockstarts(numblocks+1,false);
	
	#if defined(_OPENMP)
	#pragma omp parallel for
	#endif
	for ( int64_t b = 0; b < static_cast<int64_t>(numbblocks); ++b )
	{
		#if defined(_OPENMP)
		uint64_t const threadid = omp_get_thread_num();
		#else
		uint64_t const threadid = 0;
		#endif
		
		::libmaus2::aio::CheckedInputStream * CIS = thrstreams[threadid].get();
		CIS->clear();
		CIS->seekg(preblockstarts[b]);
		
		uint64_t codelen = 0;
		uint64_t syms = bblocksyms[b];
		uint64_t const bblocksize = preblockstarts[b+1]-preblockstarts[b];
		
		while ( codelen != bblocksize )
		{
			if ( (syms & blockmask) == 0 )
				blockstarts[syms>>blockshift] = preblockstarts[b]+codelen;
		
			::libmaus2::util::UTF8::decodeUTF8(*CIS, codelen);
			syms++;
		}			
	}
	blockstarts[numblocks] = fs;
	
	uint64_t maxblockbytes = 0;
	for ( uint64_t i = 0; i < numblocks; ++i )
		maxblockbytes = std::max(maxblockbytes,blockstarts[i+1]-blockstarts[i]);

	#if defined(UTF8SPLITDEBUG)
	CIS->clear();
	CIS->seekg(0);
	
	int cc = -1;
	uint64_t symcnt = 0;
	uint64_t codelen = 0;
	while ( (cc=CIS->peek()) >= 0 )
	{
		if ( symcnt % blocksize == 0 )
		{
			assert ( blockstarts[symcnt/blocksize] == codelen );
		}
	
		::libmaus2::util::UTF8::decodeUTF8(*CIS, codelen);		
		symcnt += 1;
	}
	#endif

	unique_ptr_type UP(new this_type);
	UP->blockstarts = blockstarts;
	UP->blocksize = blocksize;
	UP->maxblockbytes = maxblockbytes;

	CIS->clear();
	uint64_t codelen = UP->blockstarts[numblocks-1];
	UP->lastblocksize = 0;
	CIS->seekg(codelen);
	while ( codelen != fs )
	{
		::libmaus2::util::UTF8::decodeUTF8(*CIS, codelen);
		++(UP->lastblocksize);	
	}
	
	return UNIQUE_PTR_MOVE(UP);
}
