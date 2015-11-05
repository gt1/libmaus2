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
#include <libmaus2/rank/ImpCacheLineRank.hpp>
#include <libmaus2/util/Histogram.hpp>

uint64_t writeMappedFile(std::string const & bamfn, std::string const & rankfn, libmaus2::util::Histogram & binhist)
{
	libmaus2::rank::ImpCacheLineRank::WriteContextExternal WCE(rankfn);
	libmaus2::bambam::BamDecoder bamdec(bamfn);
	libmaus2::bambam::BamHeader const & header = bamdec.getHeader();
	libmaus2::bambam::BamAlignment & algn = bamdec.getAlignment();
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

void getBinHistogram(std::string const & bamfn, libmaus2::util::Histogram & binhist)
{
	libmaus2::bambam::BamDecoder bamdec(bamfn);
	libmaus2::bambam::BamHeader const & header = bamdec.getHeader();
	libmaus2::bambam::BamAlignment & algn = bamdec.getAlignment();
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
		libmaus2::util::ArgInfo const arginfo(argc,argv);
		std::string const fn = arginfo.stringRestArg(0);

		#if 0
		std::cerr << "[D] computing mapped reads bit vector and bin histogram...";
		std::string const tmpprefix = arginfo.getValue("tmpdir",arginfo.getDefaultTmpFileName());
		std::string const mappedfile = tmpprefix + ".mapped";
		libmaus2::util::TempFileRemovalContainer::addTempFile(mappedfile);
		libmaus2::util::Histogram binhist;
		uint64_t const n = writeMappedFile(fn,mappedfile,binhist);

		libmaus2::aio::InputStreamInstance mappedCIS(mappedfile);
		libmaus2::rank::ImpCacheLineRank mappedrank(mappedCIS);
		mappedCIS.close();
		uint64_t const m = n ? mappedrank.rank1(n-1) : 0;
		std::cerr << "done, n=" << n << " m=" << m << std::endl;
		#endif

		std::cerr << "[V] Computing bin histogram by straight scan...";
		libmaus2::util::Histogram binhist;
		getBinHistogram(fn,binhist);
		std::cerr << ", done." << std::endl;

		std::cerr << "[V] Computing bin histogram using index...";
		libmaus2::bambam::BamIndex index(fn);
		libmaus2::bambam::BamDecoder bamdec(fn);
		libmaus2::bambam::BamHeader const & bamheader = bamdec.getHeader();

		libmaus2::bambam::BamDecoderResetableWrapper BDRW(fn, bamheader);
		libmaus2::util::Histogram readhist;

		for ( uint64_t r = 0; r < index.getRefs().size(); ++r )
		{
			std::cerr << "(" << static_cast<double>(r) / index.getRefs().size() << ")";

			libmaus2::bambam::BamIndexRef const & ref = index.getRefs()[r];
			libmaus2::bambam::BamIndexLinear const & lin = ref.lin;
			std::cerr << "[L" << lin.intervals.size() << "]";
			for ( uint64_t i = 0; i < lin.intervals.size(); ++i )
			{
				std::cerr << "L[" << i << "]=" << lin.intervals[i] << std::endl;
			}

			for ( uint64_t i = 0; i < ref.bin.size(); ++i )
			{
				libmaus2::bambam::BamIndexBin const & bin = ref.bin[i];

				if ( bin.bin < 37450 )
				{
					for ( uint64_t j = 0; j < bin.chunks.size(); ++j )
					{
						libmaus2::bambam::BamIndexBin::Chunk const & c = bin.chunks[j];
						BDRW.resetStream(c.first,c.second);
						libmaus2::bambam::BamAlignment const & algn = BDRW.getDecoder().getAlignment();
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
