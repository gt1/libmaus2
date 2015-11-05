#include <libmaus2/lz/BufferedGzipStream.hpp>
#include <libmaus2/random/Random.hpp>
#include <libmaus2/bambam/BamDecoder.hpp>
#include <libmaus2/bambam/BamWriter.hpp>

int main()
{
	try
	{
		::libmaus2::bambam::BamDecoder bamdec(std::cin);
		// ::libmaus2::lz::BufferedGzipStream bgs(std::cin);
		::libmaus2::autoarray::AutoArray<char> C(32*1024*1024,false);
		// double const errfreq = 1e-6;
		// srand(time(0));
		::libmaus2::random::Random::setup();
		::libmaus2::bambam::BamWriter writer(std::cout,bamdec.getHeader());

		uint64_t red = 0;
		uint64_t total = 0;
		while ( (bamdec.getStream().read(C.begin(),C.size())) && (red=bamdec.getStream().gcount()) )
		{
			uint64_t off = 0;

			while ( off < red )
			{
				uint64_t const skip =
					std::min(red-off, ::libmaus2::random::Random::rand64() % (64*1024) );
				 // ::libmaus2::random::Random::rand64() % (red-off+1);

				C [ off + skip ] = ::libmaus2::random::Random::rand8();
				off += skip;

				// std::cerr << "changed offset " << total + off << std::endl;
			}


			writer.getStream().write(C.begin(),red);
			total += red;
		}
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
	}
}
