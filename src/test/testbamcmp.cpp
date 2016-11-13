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
#include <libmaus2/bambam/BamDecoder.hpp>
#include <libmaus2/util/ArgInfo.hpp>
#include <libmaus2/bambam/BamNumericalIndexGenerator.hpp>
#include <libmaus2/bambam/BamNumericalIndexDecoder.hpp>
#include <libmaus2/parallel/NumCpus.hpp>
#include <libmaus2/lz/BgzfInflateFile.hpp>

int main(int argc, char * argv[])
{
	try
	{
		libmaus2::util::ArgInfo const arginfo(argc,argv);

		std::string const fna = arginfo.getUnparsedRestArg(0);
		std::string const fnb = arginfo.getUnparsedRestArg(1);

		{
			std::string const indexfn = libmaus2::bambam::BamNumericalIndexGenerator::indexFileCheck(fna,1024,libmaus2::parallel::NumCpus::getNumLogicalProcessors(),true);
			// std::string const indexfn = libmaus2::bambam::BamNumericalIndexGenerator::indexFileCheck(fna,1024,1,true);
			libmaus2::bambam::BamNumericalIndexDecoder indexdec(indexfn);

			std::cerr << indexdec.getAlignmentCount() << std::endl;
			std::cerr << indexdec.getBlockSize() << std::endl;

			for ( uint64_t i = 0; i < indexdec.size(); ++i )
				std::cerr << "index[" << i << "]=(" << indexdec[i].first << "," << indexdec[i].second << ")" << std::endl;

			libmaus2::bambam::BamAlignment algn;
			for ( uint64_t i = 0; i < 4096; ++i )
			{
				libmaus2::lz::BgzfInflateFile::unique_ptr_type tptr(indexdec.getStreamAt(fna,i));
				libmaus2::bambam::BamAlignmentDecoder::readAlignmentGz(*tptr,algn);
				std::cerr << "name " << algn.getName() << std::endl;
			}
		}

		return 0;


		libmaus2::bambam::BamDecoder bama(fna);
		libmaus2::bambam::BamDecoder bamb(fnb);

		libmaus2::bambam::BamAlignment const & ala = bama.getAlignment();
		libmaus2::bambam::BamAlignment const & alb = bamb.getAlignment();

		libmaus2::bambam::BamHeader const & heada = bama.getHeader();
		libmaus2::bambam::BamHeader const & headb = bamb.getHeader();

		::libmaus2::bambam::BamFormatAuxiliary aux;
		uint64_t alcnt = 0;
		bool eq = true;
		uint64_t const mod = 16*1024*1024;

		while ( bama.readAlignment() )
		{
			if ( ! bamb.readAlignment() )
			{
				libmaus2::exception::LibMausException se;
				se.getStream() << "EOF on " << fnb << std::endl;
				se.finish();
				throw se;
			}

			if (
				(ala.blocksize != alb.blocksize)
				||
				memcmp(ala.D.begin(),alb.D.begin(),ala.blocksize)
			)
			{
				std::string const sama = ala.formatAlignment(heada,aux);
				std::string const samb = alb.formatAlignment(headb,aux);

				if ( sama != samb )
				{
					std::cerr << "--- Difference ---" << std::endl;
					std::cerr << sama << std::endl;
					std::cerr << samb << std::endl;
					eq = false;
				}

			}

			if ( (++alcnt) % (mod) == 0 )
				std::cerr << "[V] " << alcnt/(mod) << std::endl;
		}

		if ( bama.readAlignment() )
		{
			libmaus2::exception::LibMausException se;
			se.getStream() << "EOF on " << fna << std::endl;
			se.finish();
			throw se;
		}

		std::cerr << (eq?"equal":"different") << std::endl;
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
