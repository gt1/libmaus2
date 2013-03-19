#include <libmaus/lz/BufferedGzipStream.hpp>
#include <libmaus/random/Random.hpp>
#include <libmaus/bambam/BamDecoder.hpp>
#include <libmaus/bambam/BamWriter.hpp>

int main()
{
	try
	{
		::libmaus::bambam::BamDecoder bamdec(std::cin);
		// ::libmaus::lz::BufferedGzipStream bgs(std::cin);
		::libmaus::autoarray::AutoArray<char> C(32*1024*1024,false);
		// double const errfreq = 1e-6;
		// srand(time(0));
		::libmaus::random::Random::setup();
		::libmaus::bambam::BamWriter writer(std::cout,bamdec.bamheader);

		uint64_t red = 0;			
		uint64_t total = 0;
		while ( (red = bamdec.GZ.read(C.begin(),C.size())) )
		{
			uint64_t off = 0;
				
			while ( off < red )
			{
				uint64_t const skip =
					std::min(red-off, ::libmaus::random::Random::rand64() % (64*1024) );
				 // ::libmaus::random::Random::rand64() % (red-off+1);
					
				C [ off + skip ] = ::libmaus::random::Random::rand8();
				off += skip;
				
				// std::cerr << "changed offset " << total + off << std::endl;
			}
				
				
			writer.bgzfos.write(C.begin(),red);		
			total += red;
		}		
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
	}
}
