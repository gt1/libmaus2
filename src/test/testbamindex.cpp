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

#include <libmaus2/bambam/BamIndex.hpp>
#include <libmaus2/bambam/BamIndexGenerator.hpp>
#include <libmaus2/lz/BgzfInflate.hpp>
#include <libmaus2/bambam/BamDecoder.hpp>

libmaus2::aio::InputStreamInstance::unique_ptr_type openFile(std::string const & fn)
{
	libmaus2::aio::InputStreamInstance::unique_ptr_type bamCIS(new libmaus2::aio::InputStreamInstance(fn));
	return UNIQUE_PTR_MOVE(bamCIS);
}

int main(int argc, char * argv[])
{
	try
	{
		libmaus2::util::ArgInfo const arginfo(argc,argv);
		std::string const fn = arginfo.stringRestArg(0);

		std::ostringstream indexostr;
		if ( !libmaus2::util::GetFileSize::fileExists(fn+".bai") )
		{
			libmaus2::bambam::BamIndexGenerator indexgen("indextmp",true,true,false/*debug*/);
			libmaus2::aio::InputStreamInstance::unique_ptr_type bamCIS(openFile(fn));
			// libmaus2::lz::BgzfInflate<libmaus2::aio::InputStreamInstance> bgzfin(*bamCIS);
			libmaus2::lz::BgzfInflate<std::istream> bgzfin(*bamCIS);
		
			libmaus2::lz::BgzfInflateInfo P;
			libmaus2::autoarray::AutoArray<uint8_t> B(libmaus2::lz::BgzfConstants::getBgzfMaxBlockSize(),false);
			while ( !(P=bgzfin.readAndInfo(reinterpret_cast<char *>(B.begin()), B.size())).streameof )
				indexgen.addBlock(B.begin(),P.compressed,P.uncompressed);
			indexgen.flush(indexostr);
			bamCIS.reset();
			
			libmaus2::aio::OutputStreamInstance COS(fn+".bai");
			std::string const & index = indexostr.str();
			COS.write(index.c_str(),index.size());
			COS.flush();
		}
		
		libmaus2::autoarray::AutoArray<char> const indexA = libmaus2::autoarray::AutoArray<char>::readFile(fn+".bai");
		indexostr.write(indexA.begin(),indexA.size());
		std::istringstream indexistr(indexostr.str());
		libmaus2::bambam::BamIndex index(indexistr);

		libmaus2::bambam::BamDecoder::unique_ptr_type pbamdec(new libmaus2::bambam::BamDecoder(fn));
		libmaus2::bambam::BamHeader::unique_ptr_type bamheader = pbamdec->getHeader().uclone();
		pbamdec.reset();
		
		assert ( index.getRefs().size() == bamheader->getNumRef() );

		libmaus2::bambam::BamDecoderResetableWrapper BDRW(fn, *bamheader);

		for ( uint64_t r = 0; r < index.getRefs().size(); ++r )
		{
			libmaus2::bambam::BamIndexRef const & ref = index.getRefs()[r];
			libmaus2::bambam::BamIndexLinear const & lin = ref.lin;
			for ( uint64_t i = 0; i < lin.intervals.size(); ++i )
			{
				
			}
			
			for ( uint64_t i = 0; i < ref.bin.size(); ++i )
			{
				libmaus2::bambam::BamIndexBin const & bin = ref.bin[i];

				for ( uint64_t j = 0; j < bin.chunks.size(); ++j )
				{
					libmaus2::bambam::BamIndexBin::Chunk const & c = bin.chunks[j];
					std::cerr 
						<< "refid=" << r 
						<< " bin=" << bin.bin 
						<< " chunk=" 
						<< "(" << (c.first>>16) << "," << (c.first&((1ull<<16)-1)) << ")"
						<< ","
						<< "(" << (c.second>>16) << "," << (c.second&((1ull<<16)-1)) << ")"
						<< std::endl;
					BDRW.resetStream(c.first,c.second);
					while ( BDRW.getDecoder().readAlignment() )
					{
					
					}
				}
			}
		}
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
