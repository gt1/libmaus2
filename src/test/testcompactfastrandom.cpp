#include <libmaus/fastx/CompactFastDecoder.hpp>
#include <libmaus/util/ArgInfo.hpp>

int main(int argc, char * argv[])
{
	try
	{
		libmaus::util::ArgInfo const arginfo(argc,argv);

		std::vector<std::string> const fn = arginfo.restargs;
		
		libmaus::fastx::CompactFastConcatRandomAccessAdapter CFCRAA(fn);
		libmaus::fastx::CompactFastConcatDecoder CFCD(fn);

		libmaus::fastx::CompactFastConcatDecoder::pattern_type pat;
		
		for ( uint64_t id = 0; CFCD.getNextPatternUnlocked(pat); ++id )
		{
			/* 
			std::cerr << pat.spattern << std::endl;
			std::cerr << CFCRAA[id] << std::endl;
			*/
			
			assert ( pat.spattern == CFCRAA[id] );
			
			if ( id % 1024 == 0 )
				std::cerr << id << std::endl;
		}
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
