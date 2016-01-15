/*
    fasttools
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#include <libmaus2/fastx/FastAIndex.hpp>
#include <libmaus2/aio/InputStreamInstance.hpp>
#include <libmaus2/util/ArgInfo.hpp>
#include <libmaus2/bambam/BamAlignmentDecoderFactory.hpp>

int main(int argc, char * argv[])
{
	try
	{
		libmaus2::util::ArgInfo const arginfo(argc,argv);
		std::string const fafn = arginfo.restargs.at(0);
		std::string const faidxfn = fafn + ".fai";
		std::string const srange = arginfo.restargs.at(1);
		libmaus2::bambam::CramRange range(srange);
		libmaus2::aio::InputStreamInstance idxCIS(faidxfn);
		libmaus2::fastx::FastAIndex faidx(idxCIS);

		std::map<std::string,uint64_t> seqtoid;
		for ( uint64_t i = 0; i < faidx.sequences.size(); ++i )
		{
			libmaus2::fastx::FastAIndexEntry const & ie = faidx.sequences[i];
			std::string const & seqname = ie.name;
			seqtoid[seqname] = i;
		}

		std::string const & rangeseq = range.rangeref;

		if ( seqtoid.find(rangeseq) == seqtoid.end() )
		{
			libmaus2::exception::LibMausException lme;
			lme.getStream() << "Cannot find sequence " << rangeseq << std::endl;
			lme.finish();
			throw lme;
		}

		libmaus2::aio::InputStreamInstance faistr(fafn);
		libmaus2::autoarray::AutoArray<char> const seq = faidx.readSequence(faistr, seqtoid.find(rangeseq)->second);

		int64_t const zstart = static_cast<int64_t>(range.rangestart)-1;
		int64_t const zend = static_cast<int64_t>(range.rangeend)-1;
		int64_t const zlen = zend-zstart+1;

		if ( zlen < 0 || zstart < 0 || zend >= static_cast<int64_t>(seq.size()) )
		{
			libmaus2::exception::LibMausException lme;
			lme.getStream() << "Invalid input range" << std::endl;
			lme.finish();
			throw lme;
		}

		std::cout << ">" << srange << "\n";
		std::cout.write(seq.begin()+zstart,zlen);
		std::cout.put('\n');
		std::cout.flush();

		// std::cerr << "rangeseq=" << rangeseq << " rangestart=" << range.rangestart << " rangeend=" << range.rangeend << std::endl;

		// rangestart,rangestart
		// libmaus2::autoarray::AutoArray<char> readSequence(std::istream & in, int64_t const seqid)
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
