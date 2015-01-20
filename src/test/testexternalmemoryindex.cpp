/*
    libmaus
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2015 Genome Research Limited

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

#include <libmaus/index/ExternalMemoryIndexDecoder.hpp>
#include <libmaus/index/ExternalMemoryIndexGenerator.hpp>
#include <libmaus/bambam/ReadEndsBase.hpp>
#include <libmaus/lz/SnappyOutputStream.hpp>
#include <libmaus/lz/SnappyInputStream.hpp>
#include <sstream>

struct SerialisableUint64
{
	uint64_t i;
	
	SerialisableUint64(uint64_t const ri = 0) : i(ri) {}

	template<typename stream_type>
	uint64_t serialise(stream_type & stream) const
	{
		return libmaus::util::NumberSerialisation::serialiseNumber(stream,i);
	}

	template<typename stream_type>
	void deserialise(stream_type & stream)
	{
		i = libmaus::util::NumberSerialisation::deserialiseNumber(stream);
	}
	
	static uint64_t getSerialisedObjectSize()
	{
		return sizeof(uint64_t);
	}
	
	bool operator<(SerialisableUint64 const & U) const
	{
		return i < U.i;
	}
};

std::ostream & operator<<(std::ostream & out, SerialisableUint64 const & U)
{
	out << U.i;
	return out;
}

int main()
{
	try
	{
		static unsigned int const base_index_log = 10;
		static unsigned int const inner_index_log = 3;
		// libmaus::index::ExternalMemoryIndexGenerator<libmaus::bambam::ReadEndsBase,base_index_log,inner_index_log> index("tmpprefix");
		typedef libmaus::index::ExternalMemoryIndexGenerator<SerialisableUint64,base_index_log,inner_index_log> index_type;
		std::stringstream indexiostr;
		index_type::unique_ptr_type index(new index_type(indexiostr));
		uint64_t ipos = 0;
		
		for ( int z = 0; z < 4; ++z )
		{
			indexiostr.seekp(ipos,std::ios::beg);
			uint64_t const indexpos = index->setup();

			std::stringstream dataiostr;
			libmaus::lz::SnappyOutputStream<std::stringstream> sos(dataiostr,8*1024);
			uint64_t const n = 128*1024*1024;
			for ( uint64_t i = 0; i < n; ++i )
			{
				// SerialisableUint64 U((i*3)/4);
				SerialisableUint64 U((z+1)*i);
				if ( !(i & index_type::base_index_mask) )
					index->put(U,sos.getOffset());
				U.serialise(sos);
			}
			sos.flush();
			dataiostr.flush();
			ipos = index->flush();

			// index.reset();
			
			std::cerr << std::string(80,'-') << std::endl;

			indexiostr.clear();
			indexiostr.seekg(indexpos);
			libmaus::index::ExternalMemoryIndexDecoder<SerialisableUint64,base_index_log,inner_index_log> indexdec(indexiostr);
			// #define CACHE_DEBUG
			#if defined(CACHE_DEBUG)
			indexiostr.seekg(indexpos);
			libmaus::index::ExternalMemoryIndexDecoder<SerialisableUint64,base_index_log,inner_index_log> indexdec0(indexiostr,0);
			#endif
			
			uint64_t maxc = 0;
			for ( uint64_t i = 0; i < n; ++i )
			{
				libmaus::index::ExternalMemoryIndexDecoderFindLargestSmallerResult P = indexdec.findLargestSmaller(i);
				#if defined(CACHE_DEBUG)
				libmaus::index::ExternalMemoryIndexDecoderFindLargestSmallerResult P0 = indexdec0.findLargestSmaller(i);
				assert ( P == P0 );
				#endif
				
				dataiostr.clear();
				dataiostr.seekg(P.P.first);
				libmaus::lz::SnappyInputStream sis(dataiostr);
				sis.ignore(P.P.second);
				
				uint64_t c = 0;			
				while ( sis.peek() >= 0 )
				{
					SerialisableUint64 U;
					U.deserialise(sis);
					// std::cerr << U.i << std::endl;
					
					if ( U.i >= i )
						break;
						
					++c;
				}
				
				maxc = std::max(c,maxc);

				if ( (i & ((1ull<<16)-1)) == 0 )
					std::cerr << "i=" << i << " c=" << c << " P=" << P.P.first << "," << P.P.second << " blockid=" << P.blockid << std::endl;
			}
			
			std::cerr << "maxc=" << maxc << std::endl;
		}
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
