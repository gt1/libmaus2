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
#include <libmaus2/lcs/HashContainer2.hpp>
#include <libmaus2/fastx/FastAReader.hpp>
#include <libmaus2/util/ArgInfo.hpp>

#include <algorithm>

static std::string loadFirstPattern(std::string const & filename)
{
	::libmaus2::fastx::FastAReader fa(filename);
	::libmaus2::fastx::FastAReader::pattern_type pattern;
	if ( fa.getNextPatternUnlocked(pattern) )
	{
		return pattern.spattern;
	}
	else
	{
		::libmaus2::exception::LibMausException se;
		se.getStream() << "Failed to read first pattern from file " << filename << std::endl;
		se.finish();
		throw se;
	}
}

int main(int argc, char * argv[])
{
	try
	{
		::libmaus2::util::ArgInfo const arginfo(argc,argv);
		std::string const a = loadFirstPattern(arginfo.getRestArg<std::string>(0));
		std::string const b = loadFirstPattern(arginfo.getRestArg<std::string>(1));
		bool const inva = arginfo.getValue<bool>("inva",false);
		bool const invb = arginfo.getValue<bool>("invb",false);
		#if 0
		double const maxindelfrac = arginfo.getValue<bool>("maxindelfrac",0.05);
		double const maxsubstfrac = arginfo.getValue<bool>("maxsubstfrac",0.05);
		#endif

		std::string const A = inva ? ::libmaus2::fastx::reverseComplementUnmapped(a) : a;
		std::string const B = invb ? ::libmaus2::fastx::reverseComplementUnmapped(b) : b;

		libmaus2::lcs::HashContainer2 HC(4 /* kmer size */,A.size());
		HC.process(A.begin());

		::libmaus2::lcs::SuffixPrefixResult SPR = HC.match(B.begin(),B.size(),A.begin());

		HC.printAlignmentLines(
			std::cerr,
			A.begin(),
			B.begin(),
			B.size(),
			SPR,
			80
		);
		
		std::cerr << SPR << std::endl;

	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
