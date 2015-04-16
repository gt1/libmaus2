/*
    libmaus
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
#include <libmaus/aio/PosixFdInputStream.hpp>
#include <libmaus/bambam/BamAlignmentDecoder.hpp>
#include <libmaus/bambam/BamHeaderLowMem.hpp>
#include <libmaus/lz/BgzfDeflate.hpp>
#include <libmaus/lz/BgzfInflateStream.hpp>

#include <config.h>

/*
 * compute bit vector of used sequences
 */
static ::libmaus::bitio::IndexedBitVector::unique_ptr_type getUsedSeqVector(std::istream & in)
{
	libmaus::lz::BgzfInflateStream bgzfin(in);

	libmaus::bambam::BamHeaderLowMem::unique_ptr_type PBHLM ( libmaus::bambam::BamHeaderLowMem::constructFromBAM(bgzfin));

	::libmaus::bambam::BamAlignment algn;
	::libmaus::bitio::IndexedBitVector::unique_ptr_type Psqused(new ::libmaus::bitio::IndexedBitVector(PBHLM->getNumRef()));
	::libmaus::bitio::IndexedBitVector & sqused = *Psqused;
	uint64_t c = 0;
	while ( 
		libmaus::bambam::BamAlignmentDecoder::readAlignmentGz(bgzfin,algn)
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
	libmaus::util::ArgInfo const & arginfo,
	std::istream & in,
	::libmaus::bitio::IndexedBitVector const & IBV,
	std::ostream & out
)
{
	libmaus::lz::BgzfInflateStream bgzfin(in);
	libmaus::bambam::BamHeaderLowMem::unique_ptr_type PBHLM ( libmaus::bambam::BamHeaderLowMem::constructFromBAM(bgzfin));


	libmaus::lz::BgzfDeflate<std::ostream> bgzfout(out,1);
	PBHLM->serialiseSequenceSubset(bgzfout,IBV,"testlowmembamheaderparser" /* id */,"testlowmembamheaderparser" /* pn */,
		arginfo.commandline /* pgCL */, PACKAGE_VERSION /* pgVN */
	);

	::libmaus::bambam::BamAlignment algn;
	uint64_t c = 0;
	while ( libmaus::bambam::BamAlignmentDecoder::readAlignmentGz(bgzfin,algn) )
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
		libmaus::util::ArgInfo const arginfo(argc,argv);
		
		if ( ! arginfo.hasArg("I") )
		{
			libmaus::exception::LibMausException lme;
			lme.getStream() << "Mandatory key I (input file) is not set.\n";
			lme.finish();
			throw lme;
		}
		
		std::string const fn = arginfo.restargs.at(0);
		
		::libmaus::bitio::IndexedBitVector::unique_ptr_type PIBV;
		
		// compute vector of used sequences
		{
			libmaus::aio::PosixFdInputStream in(fn);
			::libmaus::bitio::IndexedBitVector::unique_ptr_type TIBV(getUsedSeqVector(in));
			PIBV = UNIQUE_PTR_MOVE(TIBV);
		}
		
		// filter file and remove all unused sequences from header
		{
			libmaus::aio::PosixFdInputStream in(fn);
			filterBamUsedSequences(arginfo,in,*PIBV,std::cout);
		}
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
