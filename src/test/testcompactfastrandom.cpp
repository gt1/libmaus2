#include <libmaus2/fastx/CompactFastDecoder.hpp>
#include <libmaus2/util/ArgInfo.hpp>

int main(int argc, char * argv[])
{
	try
	{
		libmaus2::util::ArgInfo const arginfo(argc,argv);

		std::vector<std::string> const fn = arginfo.restargs;

		libmaus2::fastx::CompactFastConcatRandomAccessAdapter CFCRAA(fn);
		libmaus2::fastx::CompactFastConcatDecoder CFCD(fn);

		libmaus2::fastx::CompactFastConcatDecoder::pattern_type pat;

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
