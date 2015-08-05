/*
    libmaus2
    Copyright (C) 2015 German Tischler

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
#include <libmaus2/dazzler/align/AlignmentFile.hpp>
#include <libmaus2/aio/OutputStreamInstance.hpp>
#include <libmaus2/util/ArgInfo.hpp>

int main(int argc, char * argv[])
{
	try
	{
		libmaus2::util::ArgInfo const arginfo(argc,argv);

		int64_t const from = arginfo.getRestArg<int>(0);
		int64_t const to = arginfo.getRestArg<int>(1);

		libmaus2::dazzler::align::AlignmentFile algn(std::cin);
		algn.serialiseHeader(std::cout);

		libmaus2::dazzler::align::Overlap OVL;
		while ( algn.getNextOverlap(std::cin,OVL) )
		{
			if ( OVL.aread == from )
				OVL.aread = to;
			if ( OVL.bread == from )
				OVL.bread = to;
			OVL.serialiseWithPath(std::cout,algn.small);
		}
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
