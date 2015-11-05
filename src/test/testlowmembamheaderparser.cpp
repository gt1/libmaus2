/*
    libmaus2
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
#include <libmaus2/aio/PosixFdInputStream.hpp>
#include <libmaus2/bambam/BamAlignmentDecoder.hpp>
#include <libmaus2/bambam/BamHeaderLowMem.hpp>
#include <libmaus2/lz/BgzfDeflate.hpp>
#include <libmaus2/lz/BgzfInflateStream.hpp>

#include <config.h>

/*
 * compute bit vector of used sequences
 */
static ::libmaus2::bitio::IndexedBitVector::unique_ptr_type getUsedSeqVector(std::istream & in)
{
	libmaus2::lz::BgzfInflateStream bgzfin(in);

	libmaus2::bambam::BamHeaderLowMem::unique_ptr_type PBHLM ( libmaus2::bambam::BamHeaderLowMem::constructFromBAM(bgzfin));

	::libmaus2::bambam::BamAlignment algn;
	::libmaus2::bitio::IndexedBitVector::unique_ptr_type Psqused(new ::libmaus2::bitio::IndexedBitVector(PBHLM->getNumRef()));
	::libmaus2::bitio::IndexedBitVector & sqused = *Psqused;
	uint64_t c = 0;
	while (
		libmaus2::bambam::BamAlignmentDecoder::readAlignmentGz(bgzfin,algn)
	)
	{
		if ( algn.isMapped() )
		{
			int64_t const refid = algn.getRefID();
			assert ( refid >= 0 );
			sqused.set(refid,true);
		}
		if ( algn.isPaired() && algn.isMapped() )
		{
			int64_t const refid = algn.getNextRefID();
			assert ( refid >= 0 );
			sqused.set(refid,true);
		}

		if ( ((++c) & (1024*1024-1)) == 0 )
			std::cerr << "[V] " << c/(1024*1024) << std::endl;
	}

	sqused.setupIndex();

	return UNIQUE_PTR_MOVE(Psqused);
}

static void filterBamUsedSequences(
	libmaus2::util::ArgInfo const & arginfo,
	std::istream & in,
	::libmaus2::bitio::IndexedBitVector const & IBV,
	std::ostream & out
)
{
	libmaus2::lz::BgzfInflateStream bgzfin(in);
	libmaus2::bambam::BamHeaderLowMem::unique_ptr_type PBHLM ( libmaus2::bambam::BamHeaderLowMem::constructFromBAM(bgzfin));


	libmaus2::lz::BgzfDeflate<std::ostream> bgzfout(out,1);
	PBHLM->serialiseSequenceSubset(bgzfout,IBV,"testlowmembamheaderparser" /* id */,"testlowmembamheaderparser" /* pn */,
		arginfo.commandline /* pgCL */, PACKAGE_VERSION /* pgVN */
	);

	::libmaus2::bambam::BamAlignment algn;
	uint64_t c = 0;
	while ( libmaus2::bambam::BamAlignmentDecoder::readAlignmentGz(bgzfin,algn) )
	{
		if ( algn.isMapped() )
		{
			int64_t const refid = algn.getRefID();
			assert ( refid >= 0 );
			assert ( IBV.get(refid) );
			algn.putRefId(IBV.rank1(refid)-1);
		}
		else
		{
			algn.putRefId(-1);
		}

		if ( algn.isPaired() && algn.isMapped() )
		{
			int64_t const refid = algn.getNextRefID();
			assert ( refid >= 0 );
			assert ( IBV.get(refid) );
			algn.putNextRefId(IBV.rank1(refid)-1);
		}
		else
		{
			algn.putNextRefId(-1);
		}

		algn.serialise(bgzfout);

		if ( ((++c) & (1024*1024-1)) == 0 )
			std::cerr << "[V] " << c/(1024*1024) << std::endl;
	}

	bgzfout.flush();
	bgzfout.addEOFBlock();
}


int main(int argc, char * argv[])
{
	try
	{
		libmaus2::util::ArgInfo const arginfo(argc,argv);

		if ( ! arginfo.hasArg("I") )
		{
			libmaus2::exception::LibMausException lme;
			lme.getStream() << "Mandatory key I (input file) is not set.\n";
			lme.finish();
			throw lme;
		}

		std::string const fn = arginfo.restargs.at(0);

		::libmaus2::bitio::IndexedBitVector::unique_ptr_type PIBV;

		// compute vector of used sequences
		{
			libmaus2::aio::PosixFdInputStream in(fn);
			::libmaus2::bitio::IndexedBitVector::unique_ptr_type TIBV(getUsedSeqVector(in));
			PIBV = UNIQUE_PTR_MOVE(TIBV);
		}

		// filter file and remove all unused sequences from header
		{
			libmaus2::aio::PosixFdInputStream in(fn);
			filterBamUsedSequences(arginfo,in,*PIBV,std::cout);
		}
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
