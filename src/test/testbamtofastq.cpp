#include <iostream>
#include <libmaus/util/ArgInfo.hpp>
#include <libmaus/bambam/CollatingBamDecoder.hpp>
#include <libmaus/util/TempFileRemovalContainer.hpp>

int main(int argc, char * argv[])
{
	uint64_t cnt = 0;

	try
	{
		::libmaus::util::ArgInfo const arginfo(argc,argv);
		
		#if 0
		::libmaus::bambam::BamFormatAuxiliary auxiliary;
		std::string const tmpfilename = arginfo.getDefaultTmpFileName();
		::libmaus::util::TempFileRemovalContainer::setup();
		::libmaus::util::TempFileRemovalContainer::addTempFile(tmpfilename);
		::libmaus::bambam::CollatingBamDecoder bamdec(std::cin,tmpfilename);
		::libmaus::bambam::CollatingBamDecoder::alignment_ptr_type algn;
		::libmaus::autoarray::AutoArray<char> B;
		uint64_t c = 0;
		
		while ( (algn=bamdec.get()) )
		{
			uint64_t const fqlen = algn->getFastQLength();
			if ( fqlen > B.size() )
				B = ::libmaus::autoarray::AutoArray<char>(fqlen);
			char * pe = algn->putFastQ(B.begin());
			
			// std::cout.write(B.begin(),pe-B.begin());
			
			assert ( pe == B.begin() + fqlen );
			
			std::string const reffq = algn->formatFastq(auxiliary);
			// std::cerr << "expecting " << fqlen << " got " << reffq.size() << std::endl;
			assert ( reffq.size() == fqlen );
			for ( uint64_t i = 0; i < fqlen; ++i )
				assert ( reffq[i] == B[i] );
		
			if ( ++c % (1024*1024) == 0 )
			{
				std::cerr << "[V] " << c/(1024*1024) << std::endl;
			}
		}
		#else
		::libmaus::bambam::BamDecoder reader(std::cin);
		::libmaus::bambam::BamHeader & header = reader.bamheader;
		::libmaus::bambam::BamFormatAuxiliary aux;
		
		// std::cout << header.text;
		while ( reader.readAlignment() )
		{
			// std::cout << reader.alignment.formatAlignment(header,aux) << std::endl;

			if ( ++cnt % (1024*1024) == 0 )
				std::cerr << "[V] " << cnt/(1024*1024) << std::endl;
		}		
		#endif
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		std::cerr << "cnt=" << cnt << std::endl;	
	}
}
