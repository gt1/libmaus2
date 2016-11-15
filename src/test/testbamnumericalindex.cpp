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

		return 0;
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
