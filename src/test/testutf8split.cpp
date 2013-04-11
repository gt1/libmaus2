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
		assert ( deco.numblocks == index->blockstarts.size() );
		
		for ( uint64_t i = 0; i < deco.numblocks; ++i )
			assert (  deco[i] == index->blockstarts[i] );
	}
	catch(std::exception const & ex)
	{
	
	}
}
