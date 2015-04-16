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
#include <libmaus2/LibMausConfig.hpp>

#if defined(LIBMAUS2_HAVE_KMLOCAL)
#include <libmaus2/fastx/CompactFastQMultiBlockReader.hpp>
#include <libmaus2/fastx/CompactFastQBlockGenerator.hpp>
#include <libmaus2/fastx/CompactFastQContainerDictionaryCreator.hpp>
#include <libmaus2/fastx/CompactFastQContainer.hpp>
#include <libmaus2/fastx/FqWeightQuantiser.hpp>
#include <libmaus2/fastx/CompactFastEncoder.hpp>

int kmlocalmain(::libmaus2::util::ArgInfo const & arginfo)
{
	try
	{
		// ::libmaus2::fastx::FqWeightQuantiser::statsRun(arginfo,arginfo.restargs,std::cerr);	
		std::string scont = ::libmaus2::fastx::CompactFastQBlockGenerator::encodeCompactFastQContainer(arginfo.restargs,0,3);
		std::istringstream textistr(scont);
		::libmaus2::fastx::CompactFastQContainer CFQC(textistr);
		::libmaus2::fastx::CompactFastQContainer::pattern_type cpat;
		
		for ( uint64_t i = 0; i < CFQC.size(); ++i )
		{
			CFQC.getPattern(cpat,i);
			std::cout << cpat;
		}

		#if 1
		std::ostringstream ostr;
		::libmaus2::fastx::CompactFastQBlockGenerator::encodeCompactFastQFile(arginfo.restargs,0,256,6/* qbits */,ostr);
		std::istringstream istr(ostr.str());
		// CompactFastQSingleBlockReader<std::istream> CFQSBR(istr);
		::libmaus2::fastx::CompactFastQMultiBlockReader<std::istream> CFQMBR(istr);
		::libmaus2::fastx::CompactFastQMultiBlockReader<std::istream>::pattern_type pattern;
		while ( CFQMBR.getNextPatternUnlocked(pattern) )
		{
			std::cout << pattern;
		}
		#endif

		// ::libmaus2::fastx::FqWeightQuantiser::rephredFastq(arginfo.restargs,arginfo);
		return EXIT_SUCCESS;
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
#endif

#include <libmaus2/util/ArgInfo.hpp>

int main(int argc, char * argv[])
{
	::libmaus2::util::ArgInfo arginfo(argc,argv);
	
	#if defined(LIBMAUS2_HAVE_KMLOCAL)
	return kmlocalmain(arginfo);
	#else
	std::cerr << "kmlocal is not available." << std::endl;
	return EXIT_FAILURE;
	#endif
}
