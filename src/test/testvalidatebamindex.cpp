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

#include <libmaus/bambam/BamIndex.hpp>
#include <libmaus/bambam/BamIndexGenerator.hpp>
#include <libmaus/lz/BgzfInflate.hpp>
#include <libmaus/bambam/BamDecoder.hpp>

int main(int argc, char * argv[])
{
	try
	{
		libmaus::util::ArgInfo const arginfo(argc,argv);
		std::string const fn = arginfo.stringRestArg(0);

		libmaus::bambam::BamIndex index(fn);		
		libmaus::bambam::BamDecoder bamdec(fn);
		libmaus::bambam::BamHeader const & bamheader = bamdec.getHeader();
		
		libmaus::bambam::BamDecoderResetableWrapper BDRW(fn, bamheader);

		for ( uint64_t r = 0; r < index.getRefs().size(); ++r )
		{
			libmaus::bambam::BamIndexRef const & ref = index.getRefs()[r];
			libmaus::bambam::BamIndexLinear const & lin = ref.lin;
			for ( uint64_t i = 0; i < lin.intervals.size(); ++i )
			{
				
			}
			
			for ( uint64_t i = 0; i < ref.bin.size(); ++i )
			{
				libmaus::bambam::BamIndexBin const & bin = ref.bin[i];

				for ( uint64_t j = 0; j < bin.chunks.size(); ++j )
				{
					libmaus::bambam::BamIndexBin::Chunk const & c = bin.chunks[j];
					BDRW.resetStream(c.first,c.second);
					libmaus::bambam::BamAlignment const & algn = BDRW.getDecoder().getAlignment();
					uint64_t z = 0;
					bool brok = false;
					while ( BDRW.getDecoder().readAlignment() )
					{
						if ( algn.isMapped() && bin.bin != algn.getBin() )
						{
							std::cerr 
								<< "refid=" << r 
								<< " bin=" << bin.bin 
								<< " chunk=" 
								<< "(" << (c.first>>16) << "," << (c.first&((1ull<<16)-1)) << ")"
								<< ","
								<< "(" << (c.second>>16) << "," << (c.second&((1ull<<16)-1)) << ")"
								<< std::endl;

							std::cerr << bin.bin << "\t" << algn.getBin() << "\t" << algn.computeBin() << " z=" << z << std::endl;
							
							brok = true;
						}
						
						++z;
					}
					
					if ( brok )
						std::cerr << "[brok total] " << z << std::endl;
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
