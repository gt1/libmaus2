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
#include <libmaus/rank/ImpCacheLineRank.hpp>
#include <libmaus/util/Histogram.hpp>

uint64_t writeMappedFile(std::string const & bamfn, std::string const & rankfn, libmaus::util::Histogram & binhist)
{
	libmaus::rank::ImpCacheLineRank::WriteContextExternal WCE(rankfn);
	libmaus::bambam::BamDecoder bamdec(bamfn);
	libmaus::bambam::BamHeader const & header = bamdec.getHeader();
	libmaus::bambam::BamAlignment & algn = bamdec.getAlignment();
	uint64_t r = 0;
	
	for ( ; bamdec.readAlignment(); ++r )
	{
		WCE.writeBit(algn.isMapped());
		
		if ( algn.isMapped() )
		{
			binhist( algn.getBin() );
			
			if ( algn.getBin() != algn.computeBin() )
			{
				std::cerr << "stored bin " << algn.getBin() << " != computed bin " << algn.computeBin()
					<< " for alignment " << algn.formatAlignment(header) << std::endl;
			}
		}
		
		if ( ! (r & (1024*1024-1)) )
			std::cerr << "[" << r/(1024*1024)  << "]";
	}
		
	WCE.flush();
	WCE.fillHeader(r);
	
	return r;
}

uint64_t getBinHistogram(std::string const & bamfn, libmaus::util::Histogram & binhist)
{
	libmaus::bambam::BamDecoder bamdec(bamfn);
	libmaus::bambam::BamHeader const & header = bamdec.getHeader();
	libmaus::bambam::BamAlignment & algn = bamdec.getAlignment();
	uint64_t r = 0;
	
	for ( ; bamdec.readAlignment(); ++r )
	{
		if ( algn.isMapped() )
		{
			binhist( algn.getBin() );
			
			if ( algn.getBin() != algn.computeBin() )
			{
				std::cerr << "stored bin " << algn.getBin() << " != computed bin " << algn.computeBin()
					<< " for alignment " << algn.formatAlignment(header) << std::endl;
			}
		}
		
		if ( ! (r & (1024*1024-1)) )
			std::cerr << "[" << r/(1024*1024)  << "]";
	}
}

int main(int argc, char * argv[])
{
	try
	{
		libmaus::util::ArgInfo const arginfo(argc,argv);
		std::string const fn = arginfo.stringRestArg(0);

		#if 0
		std::cerr << "[D] computing mapped reads bit vector and bin histogram...";
		std::string const tmpprefix = arginfo.getValue("tmpdir",arginfo.getDefaultTmpFileName());
		std::string const mappedfile = tmpprefix + ".mapped";
		libmaus::util::TempFileRemovalContainer::addTempFile(mappedfile);
		libmaus::util::Histogram binhist;
		uint64_t const n = writeMappedFile(fn,mappedfile,binhist);
		
		libmaus::aio::CheckedInputStream mappedCIS(mappedfile);
		libmaus::rank::ImpCacheLineRank mappedrank(mappedCIS);
		mappedCIS.close();
		uint64_t const m = n ? mappedrank.rank1(n-1) : 0;
		std::cerr << "done, n=" << n << " m=" << m << std::endl;
		#endif

		std::cerr << "[V] Computing bin histogram by straight scan...";
		libmaus::util::Histogram binhist;
		getBinHistogram(fn,binhist);
		std::cerr << ", done." << std::endl;

		std::cerr << "[V] Computing bin histogram using index...";
		libmaus::bambam::BamIndex index(fn);		
		libmaus::bambam::BamDecoder bamdec(fn);
		libmaus::bambam::BamHeader const & bamheader = bamdec.getHeader();
		
		libmaus::bambam::BamDecoderResetableWrapper BDRW(fn, bamheader);
		libmaus::util::Histogram readhist;

		for ( uint64_t r = 0; r < index.getRefs().size(); ++r )
		{
			std::cerr << "(" << static_cast<double>(r) / index.getRefs().size() << ")";
			
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
						
						if ( algn.isMapped() )
						{
							readhist(algn.getBin());
						}
						
						++z;
					}
					
					if ( brok )
						std::cerr << "[brok total] " << z << std::endl;
				}
			}
		}
		std::cerr << "done." << std::endl;
		
		std::cerr << "[V] Comparing histograms...";
		std::map<uint32_t,uint64_t> const binmap = binhist.getByType<uint32_t>();
		std::map<uint32_t,uint64_t> const readmap = readhist.getByType<uint32_t>();
		std::set<uint32_t> keyset;
		for ( std::map<uint32_t,uint64_t>::const_iterator ita = binmap.begin(); ita != binmap.end(); ++ita )
			keyset.insert(ita->first);
		for ( std::map<uint32_t,uint64_t>::const_iterator ita = readmap.begin(); ita != readmap.end(); ++ita )
			keyset.insert(ita->first);
			
		bool equal = true;
			
		for ( std::set<uint32_t>::const_iterator ita = keyset.begin(); ita != keyset.end(); ++ita )
		{
			uint32_t const key = *ita;
			
			if ( binmap.find(key) == binmap.end() )
			{
				std::cerr << "bin " << key << " is not in bin map" << std::endl;
				equal = false;
			}
			else if ( readmap.find(key) == readmap.end() )
			{
				std::cerr << "bin " << key << " is not in read map" << std::endl;
				equal = false;
			}
			else if ( 
				binmap.find(key)->second != readmap.find(key)->second
			)
			{
				std::cerr <<
					"binmap.find(key)->second=" << binmap.find(key)->second <<
					" != " <<
					"readmap.find(key)->second=" << readmap.find(key)->second << std::endl;
				equal = false;
			}
		}
		std::cerr << "done." << std::endl;
		
		std::cerr << "[R] " << (equal ? "equal" : "not equal") << std::endl;
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
