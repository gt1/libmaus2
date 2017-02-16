/*
    libmaus2
    Copyright (C) 2017 German Tischler

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
#include <libmaus2/fastx/KmerCount.hpp>
#include <libmaus2/parallel/NumCpus.hpp>
#include <libmaus2/util/ArgParser.hpp>
#include <libmaus2/util/Histogram.hpp>

static uint64_t getDefaultNumThreads()
{
	return libmaus2::parallel::NumCpus::getNumLogicalProcessors();
}

static uint64_t getDefaultK()
{
	return 16;
}

int main(int argc, char * argv[])
{
	try
	{
		libmaus2::util::ArgParser arg(argc,argv);
		std::string const fn = arg[0];
		uint64_t const k = arg.uniqueArgPresent("k") ? arg.getUnsignedNumericArg<uint64_t>("k") : getDefaultK();
		uint64_t const numthreads = arg.uniqueArgPresent("t") ? arg.getUnsignedNumericArg<uint64_t>("t") : getDefaultNumThreads();
		std::string const tmpfilebase = arg.uniqueArgPresent("T") ? arg["T"] : libmaus2::util::ArgInfo::getDefaultTmpFileName(arg.progname);

		libmaus2::fastx::KmerCount K(k);

		K.fill(fn,numthreads,tmpfilebase);

		K.print(std::cerr,256);

		std::string memfn = tmpfilebase + ".ser";
		libmaus2::util::TempFileRemovalContainer::addTempFile(memfn);
		std::cerr << "[V] serialising...";
		K.serialise(memfn);
		std::cerr << "done." << std::endl;

		libmaus2::util::Histogram H;
		libmaus2::fastx::KmerCount::StreamingDecoder SD(memfn);
		assert ( SD.n == K.A8.size() );
		for ( uint64_t i = 0; i < SD.n; ++i )
		{
			uint64_t v;
			bool const ok = SD.getNext(v);
			assert ( ok );
			uint64_t const e = K.get(i);

			if ( v != e )
				std::cerr << i << " " << e << " " << v << std::endl;

			assert ( v == e );

			if ( v )
				H(v);

			if ( (i & (16*1024*1024-1)) == 0 )
				std::cerr << static_cast<double>(i+1)/SD.n << std::endl;
		}
		std::cerr << static_cast<double>(SD.n)/SD.n << std::endl;
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
	}
}
