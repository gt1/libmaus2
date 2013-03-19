#include <iostream>
#include <cstdlib>
#include <libmaus/util/ArgInfo.hpp>
#include <libmaus/util/OutputFileNameTools.hpp>
#include <libmaus/util/GetFileSize.hpp>
#include <libmaus/fastx/FastAReader.hpp>

int main(int argc, char * argv[])
{
	try
	{
		::libmaus::util::ArgInfo arginfo(argc,argv);
		::std::vector<std::string> const & inputfilenames = arginfo.restargs;
		char const * fasuffixes[] = { ".fa", ".fasta", 0 };
		std::string defoutname = libmaus::util::OutputFileNameTools::endClipLcp(inputfilenames,&fasuffixes[0]) + ".fa";
		while ( ::libmaus::util::GetFileSize::fileExists(defoutname) )
			defoutname += "_";
		std::string const outfilename = arginfo.getValue<std::string>("outfilename",defoutname);
		
		std::cerr << "output file name " << defoutname << std::endl;
		
		::std::vector< ::libmaus::fastx::FastAReader::RewriteInfo > const info = ::libmaus::fastx::FastAReader::rewriteFiles(inputfilenames,outfilename);
		
		for ( uint64_t i = 0; i < info.size(); ++i )
			std::cerr << info[i].valid << "\t" << info[i].idlen << "\t" << info[i].seqlen << "\t" << info[i].getIdPrefix() << std::endl;
	
		return EXIT_SUCCESS;
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
