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

#if defined(LIBMAUS2_HAVE_AIO)
#define FASTXASYNC
#endif

#include <libmaus2/aio/AsynchronousReader.hpp>
#include <libmaus2/fastx/Pattern.hpp>
#include <libmaus2/fastx/FastAReader.hpp>
#include <libmaus2/fastx/FastQReader.hpp>
#include <libmaus2/fastx/FastAReaderSplit.hpp>
#include <libmaus2/fastx/FastQReaderSplit.hpp>
#include <libmaus2/fastx/isFastQ.hpp>
#include <libmaus2/fastx/SubSamplingFastReader.hpp>

template<typename reader_type>
void readFastX(std::string const filename)
{
	reader_type AR(filename);

	typename reader_type::pattern_type pattern;
	while ( AR.getNextPatternUnlocked(pattern) )
	{
		std::cerr << pattern.pattern << std::endl;
	}
}

template<typename reader_type>
uint64_t getFastXSequenceCount(std::string const filename)
{
	return reader_type::countPatterns(filename);
}

#if defined(FASTXASYNC)
template<typename reader_type>
void readFastXAsync(std::string const filename)
{
	typedef typename reader_type::block_type block_type;

	reader_type patfile(filename, 0);
	typename reader_type::stream_data_type ad(patfile,64*1024,8u /* number of buffers */);
	typename reader_type::stream_reader_type ar(ad);

	block_type * block;

	while ( (block = ar.getBlock()) )
	{
		ar.returnBlock(block);
	}
}
#endif

template<typename reader_type>
void readFastXTemplate(std::string const & filename)
{
	uint64_t const numseq = getFastXSequenceCount<reader_type>(filename);
	std::cerr << "number of reads is " << numseq << std::endl;
	readFastX<reader_type>(filename);
	#if defined(FASTXASYNC)
	readFastXAsync<reader_type>(filename);
	#endif
}

void readFastXTest(std::string const & filename)
{
	if ( ::libmaus2::fastx::IsFastQ::isFastQ(filename) )
	{
		readFastXTemplate< ::libmaus2::fastx::FastQReaderSplit > (filename);
	}
	else
	{
		readFastXTemplate< ::libmaus2::fastx::FastAReaderSplit > (filename);
	}
}

#include <libmaus2/fastx/SlashStrip.hpp>
#include <libmaus2/fastx/FirstWhiteSpaceSlashStrip.hpp>

int main(int argc, char * argv[])
{
	::libmaus2::fastx::FirstWhiteSpaceSlashStrip SlS;
	std::cerr << SlS("aabaa") << std::endl;
	std::cerr << SlS("aabaa/1") << std::endl;
	std::cerr << SlS("aabaa/1/2") << std::endl;

	std::cerr << SlS("aabaa xxaxx") << std::endl;
	std::cerr << SlS("aabaa/1 xx/1aa") << std::endl;
	std::cerr << SlS("aabaa/1/2 xy/2bb") << std::endl;

	std::string const fn = "../fq/frags_000000.fq";
	std::vector < ::libmaus2::fastx::FastInterval > VV = ::libmaus2::fastx::FastQReader::computeCommonNameAlignedFrags(std::vector<std::string>(1,fn),16,2,SlS);
	::libmaus2::fastx::FastQReader FQO(fn);
	::libmaus2::fastx::FastQReader::pattern_type opattern;
	uint64_t ocnt = 0, icnt = 0;
	while ( FQO.getNextPatternUnlocked(opattern) )
	{
		ocnt++;
	}

	for ( uint64_t i = 0; i < VV.size(); ++i )
	{
		std::cerr << VV[i] << std::endl;
		::libmaus2::fastx::FastQReader FQI(fn,VV[i]);
		::libmaus2::fastx::FastQReader::pattern_type ipattern;

		while ( FQI.getNextPatternUnlocked(ipattern) )
		{
			icnt++;
			std::cout << ipattern;
		}
	}

	std::cerr << "icnt=" << icnt << "ocnt=" << ocnt << std::endl;

	if ( argc < 2 )
	{
		std::cerr << "usage: " << argv[0] << " <in>" << std::endl;
		return EXIT_FAILURE;
	}

	try
	{
		#if 0
		std::cerr << "Indexing...";
		std::vector< ::libmaus2::fastx::FastInterval> ints = ::libmaus2::fastx::FastQReader::parallelIndex(
			std::vector<std::string>(1,std::string(argv[1])),256,2);
		std::cerr << "done." << std::endl;
		#endif
		readFastXTest(argv[1]);
	}
	catch(std::exception const & ex)
	{
		std::cerr << "exception: " << ex.what() << std::endl;
	}
}
