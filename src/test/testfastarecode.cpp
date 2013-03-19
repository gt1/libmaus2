#include <iostream>
#include <cstdlib>
#include <libmaus/util/ArgInfo.hpp>
#include <libmaus/util/OutputFileNameTools.hpp>
#include <libmaus/util/GetFileSize.hpp>
#include <libmaus/fastx/FastAReader.hpp>
#include <libmaus/aio/CircularWrapper.hpp>
#include <libmaus/aio/CheckedOutputStream.hpp>
#include <libmaus/util/TempFileRemovalContainer.hpp>

int main(int argc, char * argv[])
{
	try
	{
		::libmaus::util::ArgInfo arginfo(argc,argv);
		::libmaus::util::TempFileRemovalContainer::setup();
		::std::vector<std::string> const & inputfilenames = arginfo.restargs;
		char const * fasuffixes[] = { ".fa", ".fasta", 0 };
		
		std::string deftmpname = libmaus::util::OutputFileNameTools::endClipLcp(inputfilenames,&fasuffixes[0]) + ".fa.tmp";
		while ( ::libmaus::util::GetFileSize::fileExists(deftmpname) )
			deftmpname += "_";
		std::string defoutname = libmaus::util::OutputFileNameTools::endClipLcp(inputfilenames,&fasuffixes[0]) + ".fa.recoded";
		while ( ::libmaus::util::GetFileSize::fileExists(defoutname) )
			defoutname += "_";

		std::string const tempfilename = arginfo.getValue<std::string>("tempfilename",deftmpname);
		std::string const outfilename = arginfo.getValue<std::string>("outputfilename",defoutname);
		std::string const indexfilename = tempfilename + ".index";

		::libmaus::util::TempFileRemovalContainer::addTempFile(tempfilename);
		::libmaus::util::TempFileRemovalContainer::addTempFile(indexfilename);
		
		std::cerr << "temp file name " << tempfilename << std::endl;
		std::cerr << "output file name " << outfilename << std::endl;
		
		/* uint64_t const numseq = */ ::libmaus::fastx::FastAReader::rewriteFiles(inputfilenames,tempfilename,indexfilename);
		uint64_t curpos = 0;
		::libmaus::aio::CheckedOutputStream COS(outfilename);
		
		// 0,A,C,G,T,N
		::libmaus::autoarray::AutoArray<char> cmap(256,false);
		std::fill(cmap.begin(),cmap.end(),5);
		cmap['\n'] = 0;
		cmap['a'] = cmap['A'] = 1;
		cmap['c'] = cmap['C'] = 2;
		cmap['g'] = cmap['G'] = 3;
		cmap['t'] = cmap['T'] = 4;
		cmap['n'] = cmap['N'] = 5;

		::libmaus::autoarray::AutoArray<char> rmap(256,false);
		std::fill(rmap.begin(),rmap.end(),5);
		rmap['\n'] = 0;
		rmap['a'] = rmap['A'] = 4;
		rmap['c'] = rmap['C'] = 3;
		rmap['g'] = rmap['G'] = 2;
		rmap['t'] = rmap['T'] = 1;
		rmap['n'] = rmap['N'] = 5;

		::libmaus::autoarray::AutoArray<char> xmap(256,false);
		std::fill(xmap.begin(),xmap.end(),5);
		xmap[0] = 0;
		xmap[1] = 4;
		xmap[2] = 3;
		xmap[3] = 2;
		xmap[4] = 1;
		xmap[5] = 5;

		::libmaus::autoarray::AutoArray<char> imap(256,false);
		for ( uint64_t i = 0; i < imap.size(); ++i )
			imap[i] = static_cast<char>(i);
		
		::libmaus::fastx::FastAReader::RewriteInfoDecoder::unique_ptr_type infodec(new ::libmaus::fastx::FastAReader::RewriteInfoDecoder(indexfilename));
		::libmaus::fastx::FastAReader::RewriteInfo info;
		uint64_t maxseqlen = 0;
		while ( infodec->get(info) )
			maxseqlen = std::max(maxseqlen,info.seqlen);
			
		std::cerr << "[V] max seq len " << maxseqlen << std::endl;

		infodec = UNIQUE_PTR_MOVE(::libmaus::fastx::FastAReader::RewriteInfoDecoder::unique_ptr_type(new ::libmaus::fastx::FastAReader::RewriteInfoDecoder(indexfilename)));
		
		if ( maxseqlen <= 256*1024 )
		{
			::libmaus::aio::CheckedInputStream CIS(tempfilename);
			::libmaus::autoarray::AutoArray<uint8_t> B(maxseqlen+1,false);

			while ( infodec->get(info) )
			{
				// skip id
				CIS.ignore(info.idlen+2);
				// read sequence plus following terminator
				CIS.read(reinterpret_cast<char *>(B.begin()), info.seqlen+1);
				// map
				for ( uint64_t i = 0; i < info.seqlen+1; ++i )
					B[i] = cmap[B[i]];
				// write
				COS.write(reinterpret_cast<char const *>(B.begin()),info.seqlen+1);
				// remap
				for ( uint64_t i = 0; i < info.seqlen+1; ++i )
					B[i] = xmap[B[i]];
				// reverse
				std::reverse(B.begin(),B.begin()+info.seqlen);
				// write
				COS.write(reinterpret_cast<char const *>(B.begin()),info.seqlen+1);
			}
		}
		else
		{
			while ( infodec->get(info) )
			{
				// std::cerr << info.valid << "\t" << info.idlen << "\t" << info.seqlen << "\t" << info.getIdPrefix() << std::endl;
				uint64_t const seqbeg = curpos + (info.idlen+2);
				uint64_t const seqend = seqbeg + info.seqlen;
				
				::libmaus::aio::CheckedInputStream CIS(tempfilename); CIS.seekg(seqbeg);
				::libmaus::util::GetFileSize::copyMap(CIS,COS,cmap.begin(),seqend-seqbeg+1);
				
				::libmaus::aio::CircularReverseWrapper CRW(tempfilename,seqend);
				::libmaus::util::GetFileSize::copyMap(CRW,COS,rmap.begin(),seqend-seqbeg+1);
				
				curpos += (info.idlen+2) + (info.seqlen+1);
			}		
		}

		return EXIT_SUCCESS;
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
