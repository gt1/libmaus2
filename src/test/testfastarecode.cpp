/*
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
*/
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
		unsigned int const addterm = arginfo.getValue<unsigned int>("addterm",0);
		unsigned int const termadd = addterm ? 1 : 0;

		::libmaus::util::TempFileRemovalContainer::addTempFile(tempfilename);
		::libmaus::util::TempFileRemovalContainer::addTempFile(indexfilename);
		
		std::cerr << "temp file name " << tempfilename << std::endl;
		std::cerr << "output file name " << outfilename << std::endl;
		
		/* uint64_t const numseq = */ ::libmaus::fastx::FastAReader::rewriteFiles(inputfilenames,tempfilename,indexfilename);
		uint64_t curpos = 0;
		::libmaus::aio::CheckedOutputStream COS(outfilename);
		
		// 0,A,C,G,T,N
		// map forward
		::libmaus::autoarray::AutoArray<char> cmap(256,false);
		std::fill(cmap.begin(),cmap.end(),5+termadd);
		cmap['\n'] = 0 + termadd;
		cmap['a'] = cmap['A'] = 1 + termadd;
		cmap['c'] = cmap['C'] = 2 + termadd;
		cmap['g'] = cmap['G'] = 3 + termadd;
		cmap['t'] = cmap['T'] = 4 + termadd;
		cmap['n'] = cmap['N'] = 5 + termadd;

		// map to reverse complement
		::libmaus::autoarray::AutoArray<char> rmap(256,false);
		std::fill(rmap.begin(),rmap.end(),5+termadd);
		rmap['\n'] = 0 + termadd;
		rmap['a'] = rmap['A'] = 4 + termadd;
		rmap['c'] = rmap['C'] = 3 + termadd;
		rmap['g'] = rmap['G'] = 2 + termadd;
		rmap['t'] = rmap['T'] = 1 + termadd;
		rmap['n'] = rmap['N'] = 5 + termadd;

		// reverse complement for mapped data
		::libmaus::autoarray::AutoArray<char> xmap(256,false);
		std::fill(xmap.begin(),xmap.end(),5+termadd);
		xmap[0] = 0 + termadd;
		xmap[1] = 4 + termadd;
		xmap[2] = 3 + termadd;
		xmap[3] = 2 + termadd;
		xmap[4] = 1 + termadd;
		xmap[5] = 5 + termadd;

		::libmaus::autoarray::AutoArray<char> imap(256,false);
		for ( uint64_t i = 0; i < imap.size(); ++i )
			imap[i] = static_cast<char>(i);
		
		::libmaus::fastx::FastAReader::RewriteInfoDecoder::unique_ptr_type infodec(new ::libmaus::fastx::FastAReader::RewriteInfoDecoder(indexfilename));
		::libmaus::fastx::FastAReader::RewriteInfo info;
		uint64_t maxseqlen = 0;
		while ( infodec->get(info) )
			maxseqlen = std::max(maxseqlen,info.seqlen);
			
		std::cerr << "[V] max seq len " << maxseqlen << std::endl;

		::libmaus::fastx::FastAReader::RewriteInfoDecoder::unique_ptr_type tinfodec(new ::libmaus::fastx::FastAReader::RewriteInfoDecoder(indexfilename));
		infodec = UNIQUE_PTR_MOVE(tinfodec);
		
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
		
		if ( addterm )
			COS.put(0);

		return EXIT_SUCCESS;
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}

