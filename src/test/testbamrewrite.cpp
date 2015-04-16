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

#include <libmaus2/util/ArgInfo.hpp>
#include <libmaus2/bambam/BamParallelRewrite.hpp>

int main(int argc, char * argv[])
{
	try
	{
		libmaus2::util::ArgInfo const arginfo(argc,argv);
		uint64_t const numthreads = arginfo.getValue<uint64_t>("numthreads",8);

		libmaus2::bambam::BamParallelRewrite BPR(std::cin,std::cout,Z_DEFAULT_COMPRESSION,numthreads,4 /* blocks per thread */);
		libmaus2::bambam::BamAlignmentDecoder & dec = BPR.getDecoder();
		libmaus2::bambam::BamParallelRewrite::writer_type & writer = BPR.getWriter();

		libmaus2::bambam::BamAlignment const & algn = dec.getAlignment();
		uint64_t cnt = 0;
		for ( ; dec.readAlignment(); ++cnt )
		{
			if ( cnt % (1024*1024) == 0 )
				std::cerr << "[V] " << cnt << std::endl;
			algn.serialise(writer.getStream());	
		}

		std::cerr << "[V] " << cnt << std::endl;
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
