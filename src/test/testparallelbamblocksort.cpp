/*
    libmaus
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2015 Genome Research Limited

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
#include <libmaus/bambam/parallel/BlockMergeControl.hpp>
#include <libmaus/bambam/parallel/BlockSortControl.hpp>

#include <config.h>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferPosComparator.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferQueryNameComparator.hpp>
#include <libmaus/parallel/NumCpus.hpp>

#include <libmaus/digest/Digests.hpp>

#if defined(LIBMAUS_HAVE_SHA2_ASSEMBLY)
#include <libmaus/digest/DigestFactory_SHA2_ASM.hpp>
#endif

#if defined(LIBMAUS_HAVE_SMMINTRIN_H) && defined(HAVE_SSE4)
#include <libmaus/digest/DigestFactory_CRC32C_SSE42.hpp>
#endif

#include <libmaus/digest/DigestFactoryContainer.hpp>

template<typename order_type, 
         typename heap_element_type,
         bool create_dup_mark_info>
int parallelbamblocksort(libmaus::util::ArgInfo const & arginfo,
	bool const fixmates,
        bool const dupmarksupport,
        std::string const & sortorder,
        bool const bamindex
)
{
	// typedef libmaus::bambam::parallel::FragmentAlignmentBufferPosComparator order_type;
	
	uint64_t const numlogcpus = arginfo.getValue<int>("threads",libmaus::parallel::NumCpus::getNumLogicalProcessors());
	
	#if 0
	if ( 
		libmaus::util::GetFileSize::fileExists("fragmergeinfo.ser")
		&&
		libmaus::util::GetFileSize::fileExists("pairmergeinfo.ser")
	)
	{
		libmaus::parallel::PosixSpinLock lock;

		std::vector< ::libmaus::bambam::ReadEndsBlockDecoderBaseCollectionInfoBase > FMI = libmaus::bambam::parallel::BlockSortControl<order_type>::loadMergeInfo("fragmergeinfo.ser");
		
		// libmaus::bambam::parallel::BlockSortControl<order_type>::verifyReadEndsFragments(FMI,lock);

		std::vector < std::vector< std::pair<uint64_t,uint64_t> > > FSMI =
			libmaus::bambam::ReadEndsBlockDecoderBaseCollection<true>::getShortMergeIntervals(FMI,numlogcpus,false /* check */);

		std::vector< ::libmaus::bambam::ReadEndsBlockDecoderBaseCollectionInfoBase > PMI = libmaus::bambam::parallel::BlockSortControl<order_type>::loadMergeInfo("pairmergeinfo.ser");

		// libmaus::bambam::parallel::BlockSortControl<order_type>::verifyReadEndsPairs(PMI,lock);

		std::vector < std::vector< std::pair<uint64_t,uint64_t> > > PSMI =
			libmaus::bambam::ReadEndsBlockDecoderBaseCollection<true>::getShortMergeIntervals(PMI,numlogcpus,false /* check */);

		return 0;
	}
	#endif

	libmaus::timing::RealTimeClock progrtc; progrtc.start();
	// typedef libmaus::bambam::parallel::FragmentAlignmentBufferNameComparator order_type;
	
	libmaus::timing::RealTimeClock rtc;
	
	rtc.start();
	libmaus::aio::PosixFdInputStream in(STDIN_FILENO,256*1024);
	std::string const tmpfilebase = arginfo.getUnparsedValue("tmpfile",arginfo.getDefaultTmpFileName());
	int const templevel = arginfo.getValue<int>("level",1);
	std::string const hash = arginfo.getValue<std::string>("hash","crc32prod");
	std::string const filehash = arginfo.getValue<std::string>("filehash","md5");

	std::string const sinputformat = arginfo.getUnparsedValue("inputformat","bam");
	libmaus::bambam::parallel::BlockSortControlBase::block_sort_control_input_enum inform = libmaus::bambam::parallel::BlockSortControlBase::block_sort_control_input_bam;
	
	if ( sinputformat == "bam" )
	{
		inform = libmaus::bambam::parallel::BlockSortControlBase::block_sort_control_input_bam;
	}
	else if ( sinputformat == "sam" )
	{
		inform = libmaus::bambam::parallel::BlockSortControlBase::block_sort_control_input_sam;			
	}
	else
	{
		libmaus::exception::LibMausException lme;
		lme.getStream() << "Unknown input format " << sinputformat << std::endl;
		lme.finish();
		throw lme;				
	}
				
	libmaus::parallel::SimpleThreadPool STP(numlogcpus);
	typename libmaus::bambam::parallel::BlockSortControl<order_type,create_dup_mark_info>::unique_ptr_type VC(
		new libmaus::bambam::parallel::BlockSortControl<order_type,create_dup_mark_info>(
			inform,STP,in,templevel,tmpfilebase,hash,fixmates,dupmarksupport
		)
	);
	VC->enqueReadPackage();
	VC->waitDecodingFinished();
	// VC->printChecksums(std::cerr);
	VC->printChecksumsForBamHeader(std::cerr);
	VC->printSizes(std::cerr);
	VC->printPackageFreeListSizes(std::cerr);
	#if defined(AUTOARRAY_TRACE)
	libmaus::autoarray::autoArrayPrintTraces(std::cerr);
	#endif
	VC->freeBuffers();
	
	if ( create_dup_mark_info )
	{
		VC->flushReadEndsLists();
		#if 0
		VC->saveFragMergeInfo("fragmergeinfo.ser");
		VC->savePairMergeInfo("pairmergeinfo.ser");
		exit ( 0 );
		#endif
		VC->mergeReadEndsLists(std::cerr /* metrics stream */,"testparallelbamblocksort");
	}

	std::vector<libmaus::bambam::parallel::GenericInputControlStreamInfo> const BI = VC->getBlockInfo();
	libmaus::bitio::BitVector::unique_ptr_type Pdupvec;
	
	if ( create_dup_mark_info )
	{
		libmaus::bitio::BitVector::unique_ptr_type Tdupvec(VC->releaseDupBitVector());
		Pdupvec = UNIQUE_PTR_MOVE(Tdupvec);
	}
	libmaus::bambam::BamHeader::unique_ptr_type Pheader(VC->getHeader());
	::libmaus::bambam::BamHeader::unique_ptr_type uphead(libmaus::bambam::BamHeaderUpdate::updateHeader(arginfo,*Pheader,"testparallelbamblocksort",PACKAGE_VERSION));
	uphead->changeSortOrder(sortorder /* "coordinate" */);

	libmaus::bambam::parallel::BlockMergeControlTypeBase::block_merge_output_format_t oformat = libmaus::bambam::parallel::BlockMergeControlTypeBase::output_format_bam;
	
	if ( arginfo.getUnparsedValue("outputformat","bam") == "sam" )
		oformat = libmaus::bambam::parallel::BlockMergeControlTypeBase::output_format_sam;
	else if ( arginfo.getUnparsedValue("outputformat","bam") == "cram" )
		oformat = libmaus::bambam::parallel::BlockMergeControlTypeBase::output_format_cram;

	std::string const reference = arginfo.getUnparsedValue("reference",std::string());
	if ( oformat == libmaus::bambam::parallel::BlockMergeControlTypeBase::output_format_cram )
	{
		try
		{
			uphead->checkSequenceChecksums(reference);
			
			if ( ! uphead->checkSequenceChecksumsCached(false /* throw */) )
			{
				char const * refcache = getenv("REF_CACHE");
				
				if ( (! refcache) || (!*refcache) )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "Sequence cache is missing sequences but REF_CACHE is not set" << std::endl;
					lme.finish();
					throw lme;	
				}
				
				// try to fill cache
				uphead->getSequenceURSet(true);
			}

			if ( ! uphead->checkSequenceChecksumsCached(true /* throw */) )
			{
				libmaus::exception::LibMausException lme;
				lme.getStream() << "Sequence cache is missing sequences" << std::endl;
				lme.finish();
				throw lme;
			}
		}
		catch(...)
		{
			STP.terminate();
			STP.join();
			throw;
		}
	}
		
	std::ostringstream hostr;
	uphead->serialise(hostr);
	std::string const hostrstr = hostr.str();
	libmaus::autoarray::AutoArray<char> sheader(hostrstr.size(),false);
	std::copy(hostrstr.begin(),hostrstr.end(),sheader.begin());		
	VC.reset();
				
	std::cerr << "[V] blocks generated in time " << rtc.formatTime(rtc.getElapsedSeconds()) << std::endl;
	
	rtc.start();
	uint64_t const inputblocksize = 1024*1024;
	uint64_t const inputblocksperfile = 8;
	uint64_t const mergebuffersize = arginfo.getValueUnsignedNumeric("mergebuffersize", 256*1024*1024);
	uint64_t const mergebuffers = arginfo.getValueUnsignedNumeric("mergebuffers", 4);
	uint64_t const complistsize = 32;
	int const level = arginfo.getValue<int>("level",Z_DEFAULT_COMPRESSION);

	libmaus::digest::DigestInterface::unique_ptr_type Pdigest(libmaus::digest::DigestFactoryContainer::construct(filehash));

	bool const computerefidintervals = (oformat == libmaus::bambam::parallel::BlockMergeControlTypeBase::output_format_cram) && (sortorder == "coordinate");

	try
	{
		// typedef libmaus::bambam::parallel::GenericInputControlMergeHeapEntryCoordinate heap_element_type;
		libmaus::bambam::parallel::BlockMergeControl<heap_element_type> BMC(
			STP,std::cout,sheader,BI,Pdupvec.get(),level,inputblocksize,inputblocksperfile /* blocks per channel */,mergebuffersize /* merge buffer size */,mergebuffers /* number of merge buffers */, complistsize /* number of bgzf preload blocks */,hash,tmpfilebase,Pdigest.get(),oformat,bamindex,computerefidintervals);
		BMC.addPending();			
		BMC.waitWritingFinished();		

		std::cerr << "[D]\t" << filehash << "\t" << BMC.getFileDigest() << std::endl;

		std::cerr << "[V] blocks merged in time " << rtc.formatTime(rtc.getElapsedSeconds()) << std::endl;
	}
	catch(...)
	{
		STP.terminate();
		STP.join();
		throw;
	}

	STP.terminate();
	STP.join();

	std::cerr << "[V] run time " << progrtc.formatTime(progrtc.getElapsedSeconds()) << " (" << progrtc.getElapsedSeconds() << " s)" << "\t" << libmaus::util::MemUsage() << std::endl;

	return EXIT_SUCCESS;
}

int main(int argc, char * argv[])
{
	try
	{
		#if defined(LIBMAUS_HAVE_SHA2_ASSEMBLY)
		libmaus::digest::DigestFactoryContainer::addFactories(libmaus::digest::DigestFactory_SHA2_ASM());
		#endif
		#if defined(LIBMAUS_HAVE_SMMINTRIN_H) && defined(HAVE_SSE4)
		libmaus::digest::DigestFactoryContainer::addFactories(libmaus::digest::DigestFactory_CRC32C_SSE42());
		#endif
		
		libmaus::util::ArgInfo const arginfo(argc,argv);
		int r;
		
		if ( arginfo.getUnparsedValue("SO","coordinate") == "queryname" )
		{
			r = parallelbamblocksort<
				libmaus::bambam::parallel::FragmentAlignmentBufferQueryNameComparator,
				libmaus::bambam::parallel::GenericInputControlMergeHeapEntryQueryName,
				false /* create dup mark info */>(arginfo,false /* fix mates */,false /* dup mark support */,
				"queryname",
				false /* bam index */
			);		
		}
		else
		{
			r = parallelbamblocksort<
				libmaus::bambam::parallel::FragmentAlignmentBufferPosComparator,
				libmaus::bambam::parallel::GenericInputControlMergeHeapEntryCoordinate,
				true /* create dup mark info */>(arginfo,true /* fix mates */,true /* dup mark support */,
				"coordinate",
				true /* bam index */
			);
		}

		
		return r;
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
