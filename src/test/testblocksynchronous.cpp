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

#include <libmaus2/aio/BlockSynchronousOutputBuffer8.hpp>
#include <libmaus2/timing/RealTimeClock.hpp>

#include <vector>
#include <map>
#include <cmath>

int testBlockSynchronous()
{
	try
	{
		uint64_t const h = 512;
		uint64_t const s = 1024;
		::libmaus2::aio::BlockSynchronousOutputBuffer8 B("tmp", h, s);

		#if defined(BSOB_DEBUG)
		std::map < uint64_t, std::vector<uint64_t> > M;
		#endif
		
		for ( uint64_t i = 0; i < 1024*1024*1024ull; ++i )
		{
			uint64_t vh = rand() % h;
			uint64_t vv = rand();
			B.put ( vh, vv );
			#if defined(BSOB_DEBUG)
			M[vh].push_back(vv);
			#endif
			
			if ( ! (i & (1024*1024-1)) )
				std::cerr << "(" << i << ")";
		}
		std::cerr << std::endl;
		
		std::cerr << "flush...";
		B.flush();
		std::cerr << "done." << std::endl;
		
		std::cerr << "presorting...";
		B.presort(256*1024);
		std::cerr << "done." << std::endl;

		#if defined(BSOB_DEBUG)
		std::map < uint64_t, std::vector<uint64_t> > M2 = B.extract();
		assert ( M == M2 );
		#endif
			
		std::cerr << "distributing...";	
		std::vector<std::string> const filenames = B.distribute();
		std::cerr << "done." <<std::endl;
		
		for ( uint64_t i = 0; i < filenames.size(); ++i )
		{
			#if defined(BSOB_DEBUG)
			::libmaus2::aio::GenericInput<uint64_t> GI( filenames[i], 16*1024 );
			std::vector <uint64_t> V;
			
			uint64_t v;
			
			while ( GI.getNext(v) )
				V.push_back(v);
				
			assert (V.size() == M[i].size());
			assert (V == M[i]);
			#endif
			
			libmaus2::aio::FileRemoval::removeFile ( filenames[i] );
		}
	
		return EXIT_SUCCESS;
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}

int main(/* int argc, char * argv[] */)
{
	testBlockSynchronous();
}
