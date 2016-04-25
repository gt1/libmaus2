/*
    libmaus2
    Copyright (C) 2009-2015 German Tischler
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
#if ! defined(LIBMAUS2_SUFFIXSORT_BWTMERGEBLOCKSORTREQUEST_HPP)
#define LIBMAUS2_SUFFIXSORT_BWTMERGEBLOCKSORTREQUEST_HPP

#include <libmaus2/bitio/Clz.hpp>
#include <libmaus2/huffman/HuffmanTree.hpp>
#include <libmaus2/huffman/huffman.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/suffixsort/BwtMergeZBlock.hpp>
#include <libmaus2/suffixsort/BwtMergeZBlockRequest.hpp>
#include <libmaus2/suffixsort/BwtMergeZBlockRequestVector.hpp>
#include <libmaus2/suffixsort/BwtMergeBlockSortRequestBase.hpp>
#include <libmaus2/suffixsort/BwtMergeBlockSortResult.hpp>
#include <libmaus2/util/Histogram.hpp>
#include <libmaus2/util/TempFileNameGenerator.hpp>
#include <libmaus2/lf/DArray.hpp>
#include <libmaus2/timing/RealTimeClock.hpp>
#include <libmaus2/bitio/BitVectorOutput.hpp>
#include <libmaus2/util/CountPutObject.hpp>
#include <libmaus2/util/FileTempFileContainer.hpp>
#include <libmaus2/wavelet/Utf8ToImpCompactHuffmanWaveletTree.hpp>
#include <libmaus2/wavelet/ImpExternalWaveletGeneratorCompactHuffman.hpp>
#include <libmaus2/wavelet/RlToHwtTermRequest.hpp>
#include <libmaus2/suffixsort/ByteInputTypes.hpp>
#include <libmaus2/suffixsort/CompactInputTypes.hpp>
#include <libmaus2/suffixsort/Lz4InputTypes.hpp>
#include <libmaus2/suffixsort/PacInputTypes.hpp>
#include <libmaus2/suffixsort/PacTermInputTypes.hpp>
#include <libmaus2/suffixsort/Utf8InputTypes.hpp>
#include <libmaus2/aio/StreamLock.hpp>

namespace libmaus2
{
	namespace suffixsort
	{
		struct BwtMergeBlockSortRequest : public libmaus2::suffixsort::BwtMergeBlockSortRequestBase
		{
			bwt_merge_sort_input_type inputtype;
			std::string fn; // file name of complete file
			uint64_t fs; // size of complete file
			std::string chistfilename; // file name of global character histogram
			std::string huftreefilename; // file name of global huffman tree
			uint64_t bwtterm; // bwt term symbol (!= any other symbol)
			uint64_t maxsym; // maximal appearing symbol
			std::string tmpfilenamesser; // temp file names for this block
			std::string tmpfilenamebase; // temp file name base for files temporary to the computation of this block
			uint64_t rlencoderblocksize; // block size for run length encoder
			uint64_t isasamplingrate; // sampling rate for inverse suffix array
			uint64_t blockstart; // start of this block (in symbols)
			uint64_t cblocksize; // size of this block (in symbols)
			::libmaus2::suffixsort::BwtMergeZBlockRequestVector zreqvec; // vector of positions in file where rank in this block is requested
			bool computeTermSymbolHwt;
			uint64_t lcpnext;
			uint64_t numdownstreamthreads;
			uint64_t verbose;

			static bwt_merge_sort_input_type decodeInputType(uint64_t const i)
			{
				switch ( i )
				{
					case bwt_merge_input_type_byte:
						return bwt_merge_input_type_byte;
					case bwt_merge_input_type_compact:
						return bwt_merge_input_type_compact;
					case bwt_merge_input_type_pac:
						return bwt_merge_input_type_pac;
					case bwt_merge_input_type_pac_term:
						return bwt_merge_input_type_pac_term;
					case bwt_merge_input_type_utf8:
						return bwt_merge_input_type_utf8;
					default:
					{
						::libmaus2::exception::LibMausException ex;
						ex.getStream() << "Number " << i << " is not a valid input type designator." << std::endl;
						ex.finish();
						throw ex;
					}
				}
			}

			template<typename stream_type>
			void serialise(stream_type & stream) const
			{
				::libmaus2::util::NumberSerialisation::serialiseNumber(stream,static_cast<int>(inputtype));
				::libmaus2::util::StringSerialisation::serialiseString(stream,fn);
				::libmaus2::util::NumberSerialisation::serialiseNumber(stream,fs);
				::libmaus2::util::StringSerialisation::serialiseString(stream,chistfilename);
				::libmaus2::util::StringSerialisation::serialiseString(stream,huftreefilename);
				::libmaus2::util::NumberSerialisation::serialiseNumber(stream,bwtterm);
				::libmaus2::util::NumberSerialisation::serialiseNumber(stream,maxsym);
				::libmaus2::util::StringSerialisation::serialiseString(stream,tmpfilenamesser);
				::libmaus2::util::StringSerialisation::serialiseString(stream,tmpfilenamebase);
				::libmaus2::util::NumberSerialisation::serialiseNumber(stream,rlencoderblocksize);
				::libmaus2::util::NumberSerialisation::serialiseNumber(stream,isasamplingrate);
				::libmaus2::util::NumberSerialisation::serialiseNumber(stream,blockstart);
				::libmaus2::util::NumberSerialisation::serialiseNumber(stream,cblocksize);
				zreqvec.serialise(stream);
				::libmaus2::util::NumberSerialisation::serialiseNumber(stream,computeTermSymbolHwt);
				::libmaus2::util::NumberSerialisation::serialiseNumber(stream,lcpnext);
				::libmaus2::util::NumberSerialisation::serialiseNumber(stream,numdownstreamthreads);
				::libmaus2::util::NumberSerialisation::serialiseNumber(stream,verbose);
			}

			std::string serialise() const
			{
				std::ostringstream ostr;
				serialise(ostr);
				return ostr.str();
			}

			BwtMergeBlockSortRequest()
			{
			}

			template<typename stream_type>
			BwtMergeBlockSortRequest(stream_type & stream)
			:
				inputtype(decodeInputType(::libmaus2::util::NumberSerialisation::deserialiseNumber(stream))),
				fn(::libmaus2::util::StringSerialisation::deserialiseString(stream)),
				fs(::libmaus2::util::NumberSerialisation::deserialiseNumber(stream)),
				chistfilename(::libmaus2::util::StringSerialisation::deserialiseString(stream)),
				huftreefilename(::libmaus2::util::StringSerialisation::deserialiseString(stream)),
				bwtterm(::libmaus2::util::NumberSerialisation::deserialiseNumber(stream)),
				maxsym(::libmaus2::util::NumberSerialisation::deserialiseNumber(stream)),
				tmpfilenamesser(::libmaus2::util::StringSerialisation::deserialiseString(stream)),
				tmpfilenamebase(::libmaus2::util::StringSerialisation::deserialiseString(stream)),
				rlencoderblocksize(::libmaus2::util::NumberSerialisation::deserialiseNumber(stream)),
				isasamplingrate(::libmaus2::util::NumberSerialisation::deserialiseNumber(stream)),
				blockstart(::libmaus2::util::NumberSerialisation::deserialiseNumber(stream)),
				cblocksize(::libmaus2::util::NumberSerialisation::deserialiseNumber(stream)),
				zreqvec(stream),
				computeTermSymbolHwt(::libmaus2::util::NumberSerialisation::deserialiseNumber(stream)),
				lcpnext(::libmaus2::util::NumberSerialisation::deserialiseNumber(stream)),
				numdownstreamthreads(::libmaus2::util::NumberSerialisation::deserialiseNumber(stream)),
				verbose(::libmaus2::util::NumberSerialisation::deserialiseNumber(stream))
			{
			}

			BwtMergeBlockSortRequest(
				bwt_merge_sort_input_type rinputtype,
				std::string rfn,
				uint64_t rfs,
				std::string rchistfilename,
				std::string rhuftreefilename,
				uint64_t rbwtterm,
				uint64_t rmaxsym,
				std::string rtmpfilenamesser,
				std::string rtmpfilenamebase,
				uint64_t rrlencoderblocksize,
				uint64_t risasamplingrate,
				uint64_t rblockstart,
				uint64_t rcblocksize,
				::libmaus2::suffixsort::BwtMergeZBlockRequestVector const & rzreqvec,
				bool const rcomputeTermSymbolHwt,
				uint64_t const rlcpnext,
				uint64_t const rnumdownstreamthreads,
				uint64_t const rverbose
			)
			:
				inputtype(rinputtype),
				fn(rfn),
				fs(rfs),
				chistfilename(rchistfilename),
				huftreefilename(rhuftreefilename),
				bwtterm(rbwtterm),
				maxsym(rmaxsym),
				tmpfilenamesser(rtmpfilenamesser),
				tmpfilenamebase(rtmpfilenamebase),
				rlencoderblocksize(rrlencoderblocksize),
				isasamplingrate(risasamplingrate),
				blockstart(rblockstart),
				cblocksize(rcblocksize),
				zreqvec(rzreqvec),
				computeTermSymbolHwt(rcomputeTermSymbolHwt),
				lcpnext(rlcpnext),
				numdownstreamthreads(rnumdownstreamthreads),
				verbose(rverbose)
			{
			}

			static BwtMergeBlockSortRequest load(std::string const & s)
			{
				std::istringstream istr(s);
				return BwtMergeBlockSortRequest(istr);
			}


			static ::libmaus2::huffman::HuffmanTree::unique_ptr_type loadCompactHuffmanTree(std::string const & huftreefilename)
			{
				libmaus2::aio::InputStreamInstance::unique_ptr_type CIN(new libmaus2::aio::InputStreamInstance(huftreefilename));
				::libmaus2::huffman::HuffmanTree::unique_ptr_type tH(new ::libmaus2::huffman::HuffmanTree(*CIN));
				// CIN->close();
				CIN.reset();

				return UNIQUE_PTR_MOVE(tH);
			}

			static ::libmaus2::huffman::HuffmanTreeNode::shared_ptr_type loadHuffmanTree(std::string const & huftreefilename)
			{
				// deserialise symbol frequences
				libmaus2::aio::InputStreamInstance::unique_ptr_type chistCIN(new libmaus2::aio::InputStreamInstance(huftreefilename));
				::libmaus2::huffman::HuffmanTreeNode::shared_ptr_type shnode =
					::libmaus2::huffman::HuffmanTreeNode::deserialize(*chistCIN);
				// chistCIN->close();
				chistCIN.reset();

				return shnode;
			}

			template<typename input_types_type, typename rl_encoder>
			::libmaus2::suffixsort::BwtMergeBlockSortResult sortBlock(std::ostream * logstr) const
			{
				// typedef typename input_types_type::base_input_stream base_input_stream;
				// typedef typename base_input_stream::char_type char_type;
				// typedef typename ::libmaus2::util::UnsignedCharVariant<char_type>::type unsigned_char_type;

				if ( verbose >= 3 && logstr )
				{
					libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
					(*logstr) << "[SB1] [" << blockstart << "," << cblocksize << ") " << libmaus2::util::MemUsage() << "," << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
				}

				std::ostringstream tmpfilenamedirstr;
				tmpfilenamedirstr
					<< tmpfilenamebase << "_sortblock_"
					<< std::setw(10) << std::setfill('0') << blockstart
					<< std::setw(0) << "_"
					<< std::setw(10) << std::setfill('0') << cblocksize
					;
				std::string const tmpfilenamedir = tmpfilenamedirstr.str();
				::libmaus2::util::TempFileNameGenerator tmpgen(tmpfilenamedir,3);

				if ( verbose >= 3 && logstr )
				{
					libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
					(*logstr) << "[SB2] [" << blockstart << "," << cblocksize << ") " << libmaus2::util::MemUsage() << "," << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
				}

				::libmaus2::suffixsort::BwtMergeTempFileNameSet const tmpfilenames =
					::libmaus2::suffixsort::BwtMergeTempFileNameSet::load(tmpfilenamesser);

				if ( verbose >= 3 && logstr )
				{
					libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
					(*logstr) << "[SB3] [" << blockstart << "," << cblocksize << ") " << libmaus2::util::MemUsage() << "," << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
				}

				// result
				::libmaus2::suffixsort::BwtMergeBlockSortResult result;
				// copy request values
				result.setBlockStart( blockstart );
				result.setCBlockSize( cblocksize );
				result.setTempFileSet( tmpfilenames );

				if ( verbose >= 3 && logstr )
				{
					libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
					(*logstr) << "[SB4] [" << blockstart << "," << cblocksize << ") " << libmaus2::util::MemUsage() << "," << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
				}

				// set up huffman tree
				unsigned int const albits = maxsym ? (8*sizeof(uint64_t) - ::libmaus2::bitio::Clz::clz(maxsym)) : 0;

				// symbol before block
				int64_t const presym = input_types_type::linear_wrapper::getSymbolAtPosition(fn,blockstart ? (blockstart-1) : (fs-1));

				if ( verbose >= 3 && logstr )
				{
					libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
					(*logstr) << "[SB5] [" << blockstart << "," << cblocksize << ") " << libmaus2::util::MemUsage() << "," << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
				}

				// start of next block
				// uint64_t const nextblockstart = (blockstart + cblocksize) % fs;

				// find lcp between this block and start of next
				uint64_t const blcp = lcpnext; // findSplitCommon<input_types_type>(fn,blockstart,cblocksize,nextblockstart,fs);

				if ( verbose >= 3 && logstr )
				{
					libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
					(*logstr) << "[SB6] [" << blockstart << "," << cblocksize << ") " << libmaus2::util::MemUsage() << "," << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
				}

				// size of input string we need to read
				uint64_t const readsize = (cblocksize + blcp + 1);

				//
				typedef typename input_types_type::string_type string_type;
				typedef typename input_types_type::circular_wrapper circular_wrapper;
				typedef typename circular_wrapper::unique_ptr_type circular_wrapper_ptr_type;
				circular_wrapper_ptr_type cwptr(new circular_wrapper(fn,blockstart));
				uint64_t const octetlength = string_type::computeOctetLength(*cwptr,readsize);
				cwptr.reset();

				if ( verbose >= 3 && logstr )
				{
					libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
					(*logstr) << "[SB7] [" << blockstart << "," << cblocksize << ") " << libmaus2::util::MemUsage() << "," << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
				}

				// set up circular reader
				// typename input_types_type::circular_wrapper circ(fn,blockstart);
				circular_wrapper_ptr_type tcwptr(new circular_wrapper(fn,blockstart));
				cwptr = UNIQUE_PTR_MOVE(tcwptr);
				// construct string (read text and preprocess it for random symbol access)
				typename string_type::unique_ptr_type PT(new string_type(*cwptr, octetlength, readsize, verbose));
				string_type & T = *PT;

				if ( verbose >= 3 && logstr )
				{
					libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
					(*logstr) << "[SB8] [" << blockstart << "," << cblocksize << ") " << libmaus2::util::MemUsage() << "," << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
				}

				typedef typename string_type::saidx_t saidx_t;
				::libmaus2::autoarray::AutoArray<saidx_t, static_cast<libmaus2::autoarray::alloc_type>(libmaus2::util::StringAllocTypes::sa_atype)> SA =
					T.computeSuffixArray32();

				if ( verbose >= 3 && logstr )
				{
					libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
					(*logstr) << "[SB9] [" << blockstart << "," << cblocksize << ") " << libmaus2::util::MemUsage() << "," << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
				}

				// compute character histogram
				::libmaus2::util::Histogram hist;
				for ( uint64_t i = 0; i < cblocksize; ++i )
					hist ( T[i] );

				if ( verbose >= 3 && logstr )
				{
					libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
					(*logstr) << "[SB10] [" << blockstart << "," << cblocksize << ") " << libmaus2::util::MemUsage() << "," << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
				}

				std::map<int64_t,uint64_t> const histm = hist.getByType<int64_t>();

				if ( verbose >= 3 && logstr )
				{
					libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
					(*logstr) << "[SB11] [" << blockstart << "," << cblocksize << ") " << libmaus2::util::MemUsage() << "," << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
				}

				::libmaus2::lf::DArray D(histm,bwtterm);
				D.serialise(tmpfilenames.getHist());

				if ( verbose >= 3 && logstr )
				{
					libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
					(*logstr) << "[SB12] [" << blockstart << "," << cblocksize << ") " << libmaus2::util::MemUsage() << "," << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
				}

				// check whether first suffix of next block is smaller or larger than first suffix of this block
				bool gtlast = false;
				for ( uint64_t i = 0; i < SA.size(); ++i )
					// position 0 comes first, first suffix of next block is larger than first suffix of this block
					if ( !SA[i] )
					{
						gtlast = true;
						break;
					}
					// first suffix of next block comes first
					else if ( SA[i] == static_cast<saidx_t>(cblocksize) )
					{
						gtlast = false;
						break;
					}

				if ( verbose >= 3 && logstr )
				{
					libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
					(*logstr) << "[SB13] [" << blockstart << "," << cblocksize << ") " << libmaus2::util::MemUsage() << "," << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
				}

				// remove terminator symbols from suffix array
				saidx_t * out = SA.begin();
				for ( saidx_t * in = SA.begin(); in != SA.end(); ++in )
					if ( *in < static_cast<saidx_t>(cblocksize) )
						*(out++) = *in;
				assert ( out-SA.begin() == static_cast<ptrdiff_t>(cblocksize) );

				if ( verbose >= 3 && logstr )
				{
					libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
					(*logstr) << "[SB14] [" << blockstart << "," << cblocksize << ") " << libmaus2::util::MemUsage() << "," << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
				}

				// search for rank of first position in block
				for ( saidx_t * in = SA.begin(); in != out; ++in )
					if ( ! *in )
						result.setBlockP0Rank( (in-SA.begin()) );

				if ( verbose >= 3 && logstr )
				{
					libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
					(*logstr) << "[SB15] [" << blockstart << "," << cblocksize << ") " << libmaus2::util::MemUsage() << "," << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
				}

				// check if we find the same via binary search
				typename input_types_type::circular_suffix_comparator CSC(fn,fs);
				assert ( result.getBlockP0Rank() == CSC.suffixSearch(SA.begin(), cblocksize, blockstart /* offset */, blockstart) );

				if ( verbose >= 3 && logstr )
				{
					libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
					(*logstr) << "[SB16] [" << blockstart << "," << cblocksize << ") " << libmaus2::util::MemUsage() << "," << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
				}

				// search for rank of first position in complete file
				// result.absp0rank = ::libmaus2::suffixsort::CircularSuffixComparator::suffixSearch(SA.begin(), cblocksize, blockstart, 0, fn, fs);

				// store sampled inverse suffix array
				assert ( ::libmaus2::rank::PopCnt8<sizeof(unsigned long)>::popcnt8(isasamplingrate) == 1 );
				uint64_t const isasamplingmask = isasamplingrate-1;
				::libmaus2::aio::SynchronousGenericOutput<uint64_t> SGOISA(tmpfilenames.getSampledISA(),16*1024);
				for ( uint64_t r = 0; r < cblocksize; ++r )
				{
					// position for rank inside block
					uint64_t const pp = SA[r];
					// absolute position in complete text
					uint64_t const p = pp + blockstart;

					// sampled position?
					if (
						(! (p & isasamplingmask))
						||
						(!pp)
					)
					{
						// rank in block
						SGOISA.put(r);
						// absolute position
						SGOISA.put(p);
					}
				}
				SGOISA.flush();

				if ( verbose >= 3 && logstr )
				{
					libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
					(*logstr) << "[SB17] [" << blockstart << "," << cblocksize << ") " << libmaus2::util::MemUsage() << "," << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
				}

				/**
				 * compute ranks for lf mapping blocks
				 **/
				// std::cerr << "[V] searching for " << zreqvec.size() << " suffixes...";
				::libmaus2::timing::RealTimeClock sufsertc; sufsertc.start();
				result.resizeZBlocks(zreqvec.size());
				// loop not parallelisable because of CSC!
				for ( uint64_t z = 0; z < zreqvec.size(); ++z )
				{
					uint64_t const zabspos = zreqvec[z]; // .zabspos;

					uint64_t const zrank = CSC.suffixSearchTryInternal(
						SA.begin(), T.begin(), T.end(), cblocksize,
						blockstart, zabspos%fs
					);

					#if 0
					uint64_t const zrankext = input_types_type::circular_suffix_comparator::suffixSearch(
						SA.begin(), cblocksize,
						blockstart, zabspos%fs,
						fn, fs
					);
					assert ( zrankext == zrank );
					#endif

					::libmaus2::suffixsort::BwtMergeZBlock zblock(zabspos,zrank);
					result.setZBlock(z,zblock);
				}
				// std::cerr << "done, time " << sufsertc.getElapsedSeconds() << std::endl;

				if ( verbose >= 3 && logstr )
				{
					libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
					(*logstr) << "[SB18] [" << blockstart << "," << cblocksize << ") " << libmaus2::util::MemUsage() << "," << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
				}

				// compute BWT
				::libmaus2::bitio::BitVector::unique_ptr_type pGT(new ::libmaus2::bitio::BitVector(cblocksize+1));
				::libmaus2::bitio::BitVector & GT = *pGT;

				if ( verbose >= 3 && logstr )
				{
					libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
					(*logstr) << "[SB19] [" << blockstart << "," << cblocksize << ") " << libmaus2::util::MemUsage() << "," << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
				}

				bool gtflag = false;
				uint64_t const outcnt = out-SA.begin();
				uint64_t r0 = outcnt;

				// construct modified bwt
				for ( saidx_t * in = SA.begin(); in != out; ++in )
				{
					saidx_t const saval = *in;

					GT [ saval ] = gtflag;

					if ( saval )
					{
						*in = T[saval-1];
					}
					else
					{
						*in = bwtterm;
						// update gt flag
						gtflag = true;
						// set rank of position 0
						r0 = in-SA.begin();
					}
				}

				if ( verbose >= 3 && logstr )
				{
					libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
					(*logstr) << "[SB20] [" << blockstart << "," << cblocksize << ") " << libmaus2::util::MemUsage() << "," << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
				}

				// deallocate text
				PT.reset();

				if ( verbose >= 3 && logstr )
				{
					libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
					(*logstr) << "[SB21] [" << blockstart << "," << cblocksize << ") " << libmaus2::util::MemUsage() << "," << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
				}

				GT [ cblocksize ] = gtlast;

				if ( verbose >= 3 && logstr )
				{
					libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
					(*logstr) << "[SB22] [" << blockstart << "," << cblocksize << ") " << libmaus2::util::MemUsage() << "," << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
				}

				if ( verbose >= 3 && logstr )
				{
					libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
					(*logstr) << "[SB23] [" << blockstart << "," << cblocksize << ") " << libmaus2::util::MemUsage() << "," << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
				}

				// save gt array
				#if 0
				::libmaus2::huffman::HuffmanEncoderFileStd GTHEF(tmpfilenames.getGT());
				for ( int64_t i = cblocksize; i > 0; --i )
					GTHEF.writeBit(GT[i]);
				GTHEF.flush();
				#endif

				for ( uint64_t j = 0; j < tmpfilenames.getGT().size(); ++j )
				{
					uint64_t const gtpartsize = (cblocksize + tmpfilenames.getGT().size() - 1)/tmpfilenames.getGT().size();
					uint64_t const low = std::min(j * gtpartsize,cblocksize);
					uint64_t const high = std::min(low + gtpartsize,cblocksize);
					libmaus2::bitio::BitVectorOutput BVO(tmpfilenames.getGT()[j]);
					for ( int64_t i = cblocksize-low; i > static_cast<int64_t>(cblocksize-high); --i )
						BVO.writeBit(GT[i]);
					BVO.flush();
				}

				#if 0
				libmaus2::bitio::BitVectorOutput BVO(tmpfilenames.getGT());
				for ( int64_t i = cblocksize; i > 0; --i )
					BVO.writeBit(GT[i]);
				BVO.flush();
				#endif

				pGT.reset();

				if ( verbose >= 3 && logstr )
				{
					libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
					(*logstr) << "[SB24] [" << blockstart << "," << cblocksize << ") " << libmaus2::util::MemUsage() << "," << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
				}

				uint64_t const targetbwtfilesize = (outcnt + tmpfilenames.getBWT().size() - 1) / tmpfilenames.getBWT().size();

				for ( uint64_t b = 0; b < tmpfilenames.getBWT().size(); ++b )
				{
					uint64_t const low  = std::min(  b * targetbwtfilesize, outcnt);
					uint64_t const high = std::min(low + targetbwtfilesize, outcnt);

					rl_encoder bwtenc(tmpfilenames.getBWT()[b],albits/* alphabet bits */,high-low,rlencoderblocksize);

					if ( low <= r0 && r0 < high )
					{
						for ( uint64_t i = low; i < r0; ++i )
							bwtenc.encode(SA[i]);
						bwtenc.encode(presym);
						for ( uint64_t i = r0+1; i < high; ++i )
							bwtenc.encode(SA[i]);
					}
					else
					{
						for ( uint64_t i = low; i < high; ++i )
							bwtenc.encode(SA[i]);
					}

					#if 0
					// run-length coding for bwt
					for ( uint64_t i = 0; i < r0; ++i )
						bwtenc.encode(SA[i]);
					bwtenc.encode(presym);
					for ( uint64_t i = r0+1; i < outcnt; ++i )
						bwtenc.encode(SA[i]);
					#endif

					bwtenc.flush();

					if ( verbose >= 3 && logstr )
					{
						libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
						(*logstr) << "[D] [" << blockstart << "," << cblocksize << ") generated "<< tmpfilenames.getBWT()[b] << " with size " << high-low << std::endl;
					}
				}

				if ( verbose >= 3 && logstr )
				{
					libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
					(*logstr) << "[SB25] [" << blockstart << "," << cblocksize << ") " << libmaus2::util::MemUsage() << "," << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
				}

				if ( computeTermSymbolHwt )
				{
					if ( input_types_type::utf8Wavelet() )
					{
						std::string utftmp = tmpfilenames.getHWT() + ".utf8tmp";
						libmaus2::util::TempFileRemovalContainer::addTempFile(utftmp);

						if ( verbose >= 3 && logstr )
						{
							libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
							(*logstr) << "[SB26] [" << blockstart << "," << cblocksize << ") " << libmaus2::util::MemUsage() << "," << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
						}

						// std::cerr << "writing " << utftmp << std::endl;

						::libmaus2::util::CountPutObject CPO;
						for ( uint64_t i = 0; i < outcnt; ++i )
							::libmaus2::util::UTF8::encodeUTF8(SA[i],CPO);
						uint64_t const ucnt = CPO.c;

						if ( verbose >= 3 && logstr )
						{
							libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
							(*logstr) << "[SB27] [" << blockstart << "," << cblocksize << ") " << libmaus2::util::MemUsage() << "," << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
						}

						libmaus2::aio::OutputStreamInstance::unique_ptr_type utfCOS(new libmaus2::aio::OutputStreamInstance(utftmp));
						for ( uint64_t i = 0; i < outcnt; ++i )
							::libmaus2::util::UTF8::encodeUTF8(SA[i],*utfCOS);

						if ( verbose >= 3 && logstr )
						{
							libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
							(*logstr) << "[SB28] [" << blockstart << "," << cblocksize << ") " << libmaus2::util::MemUsage() << "," << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
						}

						utfCOS->flush();
						//utfCOS->close();
						utfCOS.reset();

						if ( verbose >= 3 && logstr )
						{
							libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
							(*logstr) << "[SB29] [" << blockstart << "," << cblocksize << ") " << libmaus2::util::MemUsage() << "," << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
						}

						#if 0
						::libmaus2::autoarray::AutoArray<uint8_t> UT(ucnt,false);
						::libmaus2::util::PutObject<uint8_t *> P(UT.begin());
						for ( uint64_t i = 0; i < outcnt; ++i )
							::libmaus2::util::UTF8::encodeUTF8(SA[i],P);
						#endif

						SA.release();

						if ( verbose >= 3 && logstr )
						{
							libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
							(*logstr) << "[SB30] [" << blockstart << "," << cblocksize << ") " << libmaus2::util::MemUsage() << "," << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
						}

						::libmaus2::autoarray::AutoArray<uint8_t> UT(ucnt,false);
						libmaus2::aio::InputStreamInstance::unique_ptr_type utfCIS(new libmaus2::aio::InputStreamInstance(utftmp));
						{
							char * c  = reinterpret_cast<char *>(UT.begin());
							uint64_t n = ucnt;
							uint64_t const bs = 64*1024;

							// utfCIS->read(reinterpret_cast<char *>(UT.begin()),ucnt,64*1024);
							while ( n )
							{
								uint64_t const toread = std::min(n,bs);
								utfCIS->read(c,toread);
								uint64_t const got = utfCIS->gcount();

								if ( got )
								{
									n -= got;
									c += got;
								}
								else
								{
									libmaus2::exception::LibMausException lme;
									lme.getStream() << "sortBlock: unexpected EOF" << std::endl;
									lme.finish();
									throw lme;
								}
							}
							assert ( ! n );
							assert ( c == reinterpret_cast<char *>(UT.begin()) + ucnt );
						}
						//utfCIS->close();
						utfCIS.reset();

						if ( verbose >= 3 && logstr )
						{
							libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
							(*logstr) << "[SB31] [" << blockstart << "," << cblocksize << ") " << libmaus2::util::MemUsage() << "," << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
						}

						::libmaus2::huffman::HuffmanTree::unique_ptr_type uhnode = loadCompactHuffmanTree(huftreefilename);

						if ( verbose >= 3 && logstr )
						{
							libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
							(*logstr) << "[SB32] [" << blockstart << "," << cblocksize << ") " << libmaus2::util::MemUsage() << "," << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
						}

						std::string const tmpfileprefix = tmpfilenamedir + "_wt";
						::libmaus2::wavelet::Utf8ToImpCompactHuffmanWaveletTree::constructWaveletTree<true>(
							UT,tmpfilenames.getHWT(),tmpfileprefix,uhnode.get(),
							1/* num threads */
						);

						if ( verbose >= 3 && logstr )
						{
							libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
							(*logstr) << "[SB33] [" << blockstart << "," << cblocksize << ") " << libmaus2::util::MemUsage() << "," << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
						}

						libmaus2::aio::FileRemoval::removeFile(utftmp.c_str());

						if ( verbose >= 3 && logstr )
						{
							libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
							(*logstr) << "[SB34] [" << blockstart << "," << cblocksize << ") " << libmaus2::util::MemUsage() << "," << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
						}
					}
					else
					{

						if ( verbose >= 3 && logstr )
						{
							libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
							(*logstr) << "[SB35] [" << blockstart << "," << cblocksize << ") " << libmaus2::util::MemUsage() << "," << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
						}

						::libmaus2::huffman::HuffmanTree::unique_ptr_type uhnode = loadCompactHuffmanTree(huftreefilename);

						if ( verbose >= 3 && logstr )
						{
							libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
							(*logstr) << "[SB36] [" << blockstart << "," << cblocksize << ") " << libmaus2::util::MemUsage() << "," << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
						}

						// construct huffman shaped wavelet tree
						libmaus2::util::FileTempFileContainer FTFC(tmpgen);

						if ( verbose >= 3 && logstr )
						{
							libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
							(*logstr) << "[SB37] [" << blockstart << "," << cblocksize << ") " << libmaus2::util::MemUsage() << "," << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
						}

						::libmaus2::wavelet::ImpExternalWaveletGeneratorCompactHuffman IEWGH(*uhnode,FTFC);

						for ( uint64_t i = 0; i < r0; ++i )
							IEWGH.putSymbol(SA[i]);
						IEWGH.putSymbol(bwtterm);
						for ( uint64_t i = r0+1; i < outcnt; ++i )
							IEWGH.putSymbol(SA[i]);

						if ( verbose >= 3 && logstr )
						{
							libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
							(*logstr) << "[SB38] [" << blockstart << "," << cblocksize << ") " << libmaus2::util::MemUsage() << "," << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
						}

						// create final stream for huffman coded wavelet tree
						::libmaus2::aio::OutputStreamInstance HCOS(tmpfilenames.getHWT());
						IEWGH.createFinalStream(HCOS);
						HCOS.flush();

						if ( verbose >= 3 && logstr )
						{
							libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
							(*logstr) << "[SB39] [" << blockstart << "," << cblocksize << ") " << libmaus2::util::MemUsage() << "," << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
						}
					}
				}
				else
				{
					libmaus2::util::TempFileRemovalContainer::addTempFile(tmpfilenames.getHWTReq());
					libmaus2::aio::OutputStreamInstance hwtReqCOS(tmpfilenames.getHWTReq());
					libmaus2::wavelet::RlToHwtTermRequest::serialise(hwtReqCOS,
						tmpfilenames.getBWT(),
						tmpfilenames.getHWT(),
						tmpfilenamedir,
						huftreefilename,
						bwtterm,
						r0,
						input_types_type::utf8Wavelet(),
						numdownstreamthreads
					);
					hwtReqCOS.flush();
					//hwtReqCOS.close();
				}

				return result;
			}

			template<typename rl_encoder>
			std::string dispatch(std::ostream * logstr) const
			{
				::libmaus2::suffixsort::BwtMergeBlockSortResult result;

				switch (inputtype)
				{
					case bwt_merge_input_type_byte:
						result = sortBlock<libmaus2::suffixsort::ByteInputTypes,rl_encoder>(logstr);
						break;
					case bwt_merge_input_type_compact:
						result = sortBlock<libmaus2::suffixsort::CompactInputTypes,rl_encoder>(logstr);
						break;
					case bwt_merge_input_type_pac:
						result = sortBlock<libmaus2::suffixsort::PacInputTypes,rl_encoder>(logstr);
						break;
					case bwt_merge_input_type_pac_term:
						result = sortBlock<libmaus2::suffixsort::PacTermInputTypes,rl_encoder>(logstr);
						break;
					case bwt_merge_input_type_lz4:
						result = sortBlock<libmaus2::suffixsort::Lz4InputTypes,rl_encoder>(logstr);
						break;
					case bwt_merge_input_type_utf8:
						result = sortBlock<libmaus2::suffixsort::Utf8InputTypes,rl_encoder>(logstr);
						break;
					default:
					{
						::libmaus2::exception::LibMausException ex;
						ex.getStream() << "Number " << inputtype << " is not a valid input type designator." << std::endl;
						ex.finish();
						throw ex;
					}
				}
				return result.serialise();
			}
		};

		std::ostream & operator<<(std::ostream & out, BwtMergeBlockSortRequest const & o);
	}
}
#endif
