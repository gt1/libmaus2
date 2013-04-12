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
#include <libmaus/util/Utf8String.hpp>
#include <libmaus/aio/CircularWrapper.hpp>
#include <libmaus/huffman/huffman.hpp>

void testUtf8String(std::string const & fn)
{
	::libmaus::util::Utf8String::shared_ptr_type us = ::libmaus::util::Utf8String::constructRaw(fn);
	::libmaus::util::Utf8StringPairAdapter usp(us);
	
	::libmaus::util::Utf8DecoderWrapper decwr(fn);
	uint64_t const numsyms = ::libmaus::util::GetFileSize::getFileSize(decwr);
	assert ( us->size() == numsyms );

	for ( uint64_t i = 0; i < numsyms; ++i )
	{
		assert ( static_cast<wchar_t>(decwr.get()) == us->get(i) );
		assert ( 
			us->get(i) ==
			((usp[2*i] << 12) | (usp[2*i+1]))
		);
	}
	
	::std::map<int64_t,uint64_t> const chist = us->getHistogramAsMap();
	::libmaus::huffman::HuffmanTreeNode::shared_ptr_type htree = ::libmaus::huffman::HuffmanBase::createTree(chist);
	::libmaus::huffman::EncodeTable<1> ET(htree.get());
	
	for ( ::std::map<int64_t,uint64_t>::const_iterator ita = chist.begin(); ita != chist.end(); ++ita )
	{
		::libmaus::util::UTF8::encodeUTF8(ita->first,std::cerr);
		std::cerr << "\t" << ita->second << "\t" << ET.printCode(ita->first) << std::endl;
	}
}

void testUtf8Circular(std::string const & fn)
{
	::libmaus::util::Utf8String us(fn);
	::libmaus::aio::Utf8CircularWrapper CW(fn);

	for ( uint64_t i = 0; i < 16*us.size(); ++i )
		assert ( us[i%us.size()] == static_cast<wchar_t>(CW.get()) );
}

void testUtf8Seek(std::string const & fn)
{
	std::vector < wchar_t > W;
	::libmaus::util::Utf8DecoderWrapper decwr(fn);
	int64_t lastperc = -1;
	
	wchar_t w = -1;
	while ( (w=decwr.get()) >= 0 )
		W.push_back(w);
	
	for ( uint64_t i = 0; i <= W.size(); i += std::min(W.size()-i+1,static_cast<size_t>(rand() % 1024)) )
	{
		int64_t const newperc = std::floor((i*100.0)/W.size()+0.5);
		
		if ( newperc != lastperc )
		{
			lastperc = newperc;
			std::cerr << "\r" << std::string(40,' ') << "\r" << newperc << std::flush;
		}
		
		decwr.clear();
		decwr.seekg(i);
		
		for ( uint64_t j = i; j < W.size(); ++j )
			assert ( static_cast<wchar_t>(decwr.get()) == W[j] );
	}

	std::cerr << "\r" << std::string(40,' ') << "\r" << 100 << std::endl;	
}

void testUtf8BlockIndexDecoder(std::string const & fn)
{
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
}

int main(int argc, char * argv[])
{
	try
	{
		::libmaus::util::ArgInfo const arginfo(argc,argv);
		
		std::string const fn = arginfo.getRestArg<std::string>(0);
		
		testUtf8BlockIndexDecoder(fn); // also creates index file
		testUtf8String(fn);		
		testUtf8Circular(fn);
		testUtf8Seek(fn);
	}
	catch(std::exception const & ex)
	{
	
	}
}
