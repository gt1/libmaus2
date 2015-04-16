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
#include <libmaus2/bambam/BamWriter.hpp>
#include <libmaus2/bambam/BamFlagBase.hpp>

#include <libmaus2/bambam/BamDecoder.hpp>

void testInplaceReverseComplement()
{
	::libmaus2::bambam::BamHeader header;
	header.addChromosome("chr1",2000);
	
	std::ostringstream ostr;
	{
	::libmaus2::bambam::BamWriter bamwriter(ostr,header);

	bamwriter.encodeAlignment("name",0,1000,30,
		::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FUNMAP, "", 0, 1500, -1, "ACGTATGCA", "HGHGHGHGH"
	);
	bamwriter.commit();
	}
	
	std::istringstream istr(ostr.str());
	libmaus2::bambam::BamDecoder bamdec(istr);
	
	while ( bamdec.readAlignment() )
	{
		libmaus2::bambam::BamAlignment & algn = bamdec.getAlignment();
		
		for ( unsigned int k = 0; k <= 10; ++k )
		{
			for ( uint64_t i = 0; i < (1ull << (2*k)); ++i )
			{
				std::string s(k,'A');
				uint64_t t = i;
				for ( uint64_t j = 0; j < k; ++j, t>>=2 )
				{
					switch ( t & 0x3 )
					{
						case 0: s[j] = 'A'; break;
						case 1: s[j] = 'C'; break;
						case 2: s[j] = 'G'; break;
						case 3: s[j] = 'T'; break;
					}
				}
				
				std::reverse(s.begin(),s.end());
				
				algn.replaceSequence(s,std::string(k,'H'));
			
				libmaus2::bambam::BamAlignment::unique_ptr_type ualgn(algn.uclone());
			
				// std::cout << algn.formatAlignment(header) << std::endl;
			
				ualgn->reverseComplementInplace();
			
				// std::cout << ualgn->formatAlignment(header) << std::endl;
			
				assert ( algn.getReadRC() == ualgn->getRead() );
			}
			
			std::cerr << "k=" << k << std::endl;
		}
	}
}

#include <libmaus2/lz/BgzfDeflateOutputCallbackMD5.hpp>
#include <libmaus2/bambam/BgzfDeflateOutputCallbackBamIndex.hpp>

int main()
{
	try
	{
		::libmaus2::bambam::BamHeader header;
		header.addChromosome("chr1",2000);
		
		::libmaus2::lz::BgzfDeflateOutputCallbackMD5 md5cb;
		libmaus2::bambam::BgzfDeflateOutputCallbackBamIndex indexcb("index_tmp",true,true);
		std::vector< ::libmaus2::lz::BgzfDeflateOutputCallback * > cbs;
		cbs.push_back(&md5cb);
		cbs.push_back(&indexcb);
		
		::libmaus2::bambam::BamWriter::unique_ptr_type bamwriter(new ::libmaus2::bambam::BamWriter(std::cout,header,::libmaus2::bambam::BamWriter::getDefaultCompression(),&cbs));
		// ::libmaus2::bambam::BamParallelWriter::unique_ptr_type bamwriter(new ::libmaus2::bambam::BamParallelWriter(std::cout,8,header,::libmaus2::bambam::BamParallelWriter::getDefaultCompression(),&cbs));
		
		for ( uint64_t i = 0; i < 64*1024; ++i )
		{
			std::ostringstream rnstr;
			rnstr << "r" << "_" << std::setw(6) << std::setfill('0') << i;
			std::string const rn = rnstr.str();
			
			bamwriter->encodeAlignment(rn,0,1000+i*100,30, ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FPAIRED | ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FREAD1, "9M", 0,1500, -1, "ACGTATGCA", "HGHGHGHGH");
			bamwriter->putAuxString("XX","HelloWorld");
			bamwriter->commit();
			
			bamwriter->encodeAlignment(rn,0,1000+i*100+50, 20,::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FPAIRED | ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FREAD2,"4=1X4=",0,1000,-1,"TTTTATTTT","HHHHAHHHH");
			bamwriter->putAuxNumber("XY",'i',42);
			bamwriter->putAuxNumber("XZ",'f',3.141592);
			bamwriter->commit();				
		}
		
		bamwriter.reset();
		
		std::cerr << "md5: " << md5cb.getDigest() << std::endl;
		indexcb.flush(std::string("t.bai"));
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
