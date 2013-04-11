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
#include <libmaus/aio/CircularWrapper.hpp>

#include <libmaus/rank/ImpCacheLineRank.hpp>
#include <libmaus/select/ImpCacheLineSelectSupport.hpp>

struct Utf8String
{
	::libmaus::autoarray::AutoArray<uint8_t> A;
	::libmaus::rank::ImpCacheLineRank::unique_ptr_type I;
	::libmaus::select::ImpCacheLineSelectSupport::unique_ptr_type S;
	
	Utf8String(std::string const & filename)
	{
		::libmaus::aio::CheckedInputStream CIS(filename);
		uint64_t const an = ::libmaus::util::GetFileSize::getFileSize(CIS);
		A = ::libmaus::autoarray::AutoArray<uint8_t>(an);
		CIS.read(reinterpret_cast<char *>(A.begin()),an);
		
		uint64_t const bitalign = 6*64;
		
		I = UNIQUE_PTR_MOVE(::libmaus::rank::ImpCacheLineRank::unique_ptr_type(new ::libmaus::rank::ImpCacheLineRank(((an+(bitalign-1))/bitalign)*bitalign)));

		::libmaus::rank::ImpCacheLineRank::WriteContext WC = I->getWriteContext();
		for ( uint64_t i = 0; i < an; ++i )
			if ( (!(A[i] & 0x80)) || ((A[i] & 0xc0) != 0x80) )
				WC.writeBit(1);
			else
				WC.writeBit(0);
				
		for ( uint64_t i = an; i % bitalign; ++i )
			WC.writeBit(0);
				
		WC.flush();
		
		S = ::libmaus::select::ImpCacheLineSelectSupport::unique_ptr_type(new ::libmaus::select::ImpCacheLineSelectSupport(*I,8));
		
		#if 0
		std::cerr << "A.size()=" << A.size() << std::endl;
		std::cerr << "I.byteSize()=" << I->byteSize() << std::endl;
		#endif
	}
	
	wchar_t operator[](uint64_t const i) const
	{
		uint64_t const p = I->select1(i);
		::libmaus::util::GetObject<uint8_t const *> G(A.begin()+p);
		return ::libmaus::util::UTF8::decodeUTF8(G);
	}
	wchar_t get(uint64_t const i) const
	{
		return (*this)[i];
	}
};

int main(int argc, char * argv[])
{
	try
	{
		::libmaus::util::ArgInfo const arginfo(argc,argv);
		
		std::string const fn = arginfo.getRestArg<std::string>(0);
		
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
		
		#if 0
		std::vector < wchar_t > W;
		::libmaus::util::Utf8DecoderWrapper decwr(fn);
		
		wchar_t w = -1;
		while ( (w=decwr.get()) >= 0 )
			W.push_back(w);
		
		for ( uint64_t i = 0; i <= W.size(); i += std::min(W.size()-i+1,static_cast<size_t>(rand() % 32)) )
		{
			std::cerr << "i=" << i << std::endl;
			
			decwr.clear();
			decwr.seekg(i);
			
			for ( uint64_t j = i; j < W.size(); ++j )
				assert ( decwr.get() == W[j] );
		}
		#endif

		#if 0		
		::libmaus::aio::Utf8CircularWrapper CW(fn);

		wchar_t w = -1;
		while ( (w=CW.get()) >= 0 )
		{
			::libmaus::util::UTF8::encodeUTF8(w,std::cout);
		}
		#endif
		
		Utf8String us(fn);
		
		::libmaus::util::Utf8DecoderWrapper decwr(fn);
		uint64_t const numsyms = ::libmaus::util::GetFileSize::getFileSize(decwr);

		for ( uint64_t i = 0; i < numsyms; ++i )
		{
			assert ( decwr.get() == us[i] );
			// ::libmaus::util::UTF8::encodeUTF8(us[i],std::cout);
		}
	}
	catch(std::exception const & ex)
	{
	
	}
}
