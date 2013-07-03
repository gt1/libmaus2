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
#include <libmaus/bambam/BamWriter.hpp>
#include <libmaus/bambam/BamFlagBase.hpp>

#include <libmaus/bambam/BamDecoder.hpp>

void testInplaceReverseComplement()
{
	::libmaus::bambam::BamHeader header;
	header.addChromosome("chr1",2000);
	
	std::ostringstream ostr;
	{
	::libmaus::bambam::BamWriter bamwriter(ostr,header);

	bamwriter.encodeAlignment("name",0,1000,30,
		::libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FUNMAP, "", 0, 1500, -1, "ACGTATGCA", "HGHGHGHGH"
	);
	bamwriter.commit();
	}
	
	std::istringstream istr(ostr.str());
	libmaus::bambam::BamDecoder bamdec(istr);
	
	while ( bamdec.readAlignment() )
	{
		libmaus::bambam::BamAlignment & algn = bamdec.getAlignment();
		
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
			
				libmaus::bambam::BamAlignment::unique_ptr_type ualgn = UNIQUE_PTR_MOVE(algn.uclone());
			
				// std::cout << algn.formatAlignment(header) << std::endl;
			
				ualgn->reverseComplementInplace();
			
				// std::cout << ualgn->formatAlignment(header) << std::endl;
			
				assert ( algn.getReadRC() == ualgn->getRead() );
			}
			
			std::cerr << "k=" << k << std::endl;
		}
	}
}

int main()
{
	try
	{
		::libmaus::bambam::BamHeader header;
		header.addChromosome("chr1",2000);
		
		// ::libmaus::bambam::BamWriter bamwriter(std::cout,header);
		::libmaus::bambam::BamParallelWriter bamwriter(std::cout,8,header);
		
		for ( uint64_t i = 0; i < 64*1024; ++i )
		{
			std::ostringstream rnstr;
			rnstr << "r" << "_" << std::setw(6) << std::setfill('0') << i;
			std::string const rn = rnstr.str();
			
			bamwriter.encodeAlignment(rn,0,1000,30, ::libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FPAIRED | ::libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FREAD1, "9M", 0,1500, -1, "ACGTATGCA", "HGHGHGHGH");
			bamwriter.putAuxString("XX","HelloWorld");
			bamwriter.commit();
			
			bamwriter.encodeAlignment(rn,0,1500, 20,::libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FPAIRED | ::libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FREAD2,"4=1X4=",0,1000,-1,"TTTTATTTT","HHHHAHHHH");
			bamwriter.putAuxNumber("XY",'i',42);
			bamwriter.putAuxNumber("XZ",'f',3.141592);
			bamwriter.commit();				
		}
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
