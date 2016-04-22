/**
    libmaus2
    Copyright (C) 2009-2016 German Tischler
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
**/
#if ! defined(LIBMAUS2_SUFFIXSORT_BWTB3M_BWTMERGEOPTIONS_HPP)
#define LIBMAUS2_SUFFIXSORT_BWTB3M_BWTMERGEOPTIONS_HPP

#include <libmaus2/util/ArgInfo.hpp>
#include <libmaus2/parallel/NumCpus.hpp>
#include <libmaus2/util/OutputFileNameTools.hpp>
#include <libmaus2/math/numbits.hpp>

namespace libmaus2
{
	namespace suffixsort
	{
		namespace bwtb3m
		{
			struct BwtMergeSortOptions
			{
				enum bwt_merge_input_type {
					bwt_merge_input_type_bytestream,
					bwt_merge_input_type_compactstream,
					bwt_merge_input_type_pac,
					bwt_merge_input_type_pacterm,
					bwt_merge_input_type_lz4,
					bwt_merge_input_type_utf_8
				};

				uint64_t numthreads;
				std::string fn;
				uint64_t wordsperthread;
				bool bwtonly;
				uint64_t mem;
				std::string tmpfilenamebase;
				std::string sparsetmpfilenamebase;
				uint64_t isasamplingrate;
				uint64_t sasamplingrate;
				bool copyinputtomemory;
				bool computeTermSymbolHwt;
				uint64_t maxblocksize;
				uint64_t maxpreisasamplingrate;
				std::string defoutfn;
				std::string outfn;
				std::string sinputtype;
				bwt_merge_input_type inputtype;
				uint64_t largelcpthres;
				uint64_t verbose;

				// parse input type from string form to enum
				static bwt_merge_input_type parseInputType(std::string const & sinputtype)
				{
					typedef std::pair<char const *, bwt_merge_input_type> pair_type;
					char const * cnull = 0;

					pair_type valid[] = {
						// byte sequence (alphabet 0-255)
						std::pair<char const *, bwt_merge_input_type>("bytestream",bwt_merge_input_type_bytestream),
						// compact array (block code) with up to 8 bits per symbol (see CompactArrayWriter for creating such files)
						std::pair<char const *, bwt_merge_input_type>("compactstream",bwt_merge_input_type_compactstream),
						// BWA pac format
						std::pair<char const *, bwt_merge_input_type>("pac",bwt_merge_input_type_pac),
						// BWA pac format with explicit terminator symbol
						std::pair<char const *, bwt_merge_input_type>("pacterm",bwt_merge_input_type_pacterm),
						// lz4 compressed file (see libmaus2::lz::Lz4Compress for creating such files)
						std::pair<char const *, bwt_merge_input_type>("lz4",bwt_merge_input_type_lz4),
						// utf-8 coded unicode sequence (alphabet can contain symbols > 255)
						std::pair<char const *, bwt_merge_input_type>("utf-8",bwt_merge_input_type_utf_8),
						// array terminator (not a file format)
						std::pair<char const *, bwt_merge_input_type>(cnull,bwt_merge_input_type_bytestream),
					};

					for ( pair_type const * p = &valid[0]; p->first; ++p )
						if ( std::string(p->first) == sinputtype )
							return p->second;

					libmaus2::exception::LibMausException lme;
					lme.getStream() << "libmaus2::suffixsort::bwtb3m::BwtMergeSortOptions::parseInputType: unknown input type " << sinputtype << std::endl;
					lme.finish();
					throw lme;
				}

				// default input type: bytestream
				static std::string getDefaultInputType()
				{
					return "bytestream";
				}

				// default sa sampling rate 32
				static uint64_t getDefaultSaSamplingRate()
				{
					return 32;
				}

				// default isa sampling rate 256k
				static uint64_t getDefaultIsaSamplingRate()
				{
					return 256*1024;
				}

				// default copyinputtomemory: keep file where it is
				static bool getDefaultCopyInputToMemory()
				{
					return false;
				}

				// default bwtonly setting false: compute BWT and sampled SA/ISA
				static bool getDefaultBWTOnly()
				{
					return false;
				}

				// default words per thread: 64k*8 = 512k (for sparse file handling)
				static uint64_t getDefaultWordsPerThread()
				{
					return 64ull*1024ull;
				}

				// default memory setting: 2GB
				static uint64_t getDefaultMem()
				{
					return 2ull * 1024ull * 1024ull * 1024ull;
				}

				// default number of threads to use
				static uint64_t getDefaultNumThreads()
				{
					return libmaus2::parallel::NumCpus::getNumLogicalProcessors();
				}

				static uint64_t getDefaultLargeLCPThres()
				{
					return 16ull*1024ull;
				}

				static uint64_t getDefaultVerbose()
				{
					return 0;
				}

				// compute default output file name from input file name
				static std::string computeDefaultOutputFileName(std::string const & fn)
				{
					std::vector<std::string> endClipS;
					endClipS.push_back(".txt");
					endClipS.push_back(".compact");
					libmaus2::autoarray::AutoArray<char const *> endClipC(endClipS.size()+1);
					for ( uint64_t i = 0; i < endClipS.size(); ++i )
						endClipC[i] = endClipS[i].c_str();
					endClipC[endClipS.size()] = 0;
					std::string const defoutfn = libmaus2::util::OutputFileNameTools::endClip(fn,endClipC.begin()) + ".bwt";
					return defoutfn;
				}

				BwtMergeSortOptions() {}

				/**
				 * constructor given single parameters
				 *
				 * @param rfn input text file name
				 * @param rmem memory usage guide in bytes (default 2GB)
				 * @param rnumthreads number of threads used during BWT/sampled SA/sampled ISA construction
				 * @param rsinputtype input file format (see parseInputType function above for possible choices)
				 * @param rbwtonly construct BWT only and not sampled SA/sampled ISA if set.
				 *                 This avoid loading the final BWT to memory in the form of a
				 *                 huffman shaped wavelet tree for constructing the sampled suffix array and sampled inverse suffix array
				 * @param rtmpfilenamebase prefix of temporary files produced (input directory if not set)
				 * @param rsparsetmpfilenamebase prefix of temporary files produced for sparse gap array (same as rtmpfilenamebase if unset)
				 * @param routfn file name for run length encoded output BWT (see libmaus2::huffman::RLDecoder for reading it)
				 * @param risasamplingrate sampling rate for sampled inverse suffix array (only relevant for rbwtonly=false)
				 * @param rsasamplingrate sampling rate for sampled suffix array (only relevant for rbwtonly=false)
				 * @param rmaxpreisasamplingrate maximum pre isa sampling rate. This influences the construction
				 *                               of a quasi sampled inverse suffix array during BWT construction. Constructing a sampled SA/ISA
				 *                               based on scans of the BWT in external memory uses up to rmaxpreisasamplingrate scans. In the case
				 *                               of rbwtonly=true the quasi sampled inverse suffix array is used to obtain starting points for
				 *                               parallel construction of the sampled SA/ISA in memory, so sampling can be much more sparse,
				 *                               as we only need numthreads sample points. By default this is 64 if rbwtonly=true and 256k otherwise.
				 * @param rcopyinputtomemory copy input text to a memory file for processing if true (may speed up construction, but uses more memory)
				 * @param rmaxblocksize maximum block size for base blocks in BWT construction
				 * @param rcomputeTermSymbolHwt do not set
				 * @param rwordsperthread words used per thread for buffering output in sparse gap array construction
				 **/
				BwtMergeSortOptions(
					std::string rfn,
					uint64_t rmem = getDefaultMem(),
					uint64_t rnumthreads = getDefaultNumThreads(),
					std::string rsinputtype = getDefaultInputType(),
					bool const rbwtonly = getDefaultBWTOnly(),
					std::string rtmpfilenamebase = std::string(),
					std::string rsparsetmpfilenamebase = std::string(),
					std::string routfn = std::string(),
					uint64_t risasamplingrate = getDefaultIsaSamplingRate(),
					uint64_t rsasamplingrate = getDefaultSaSamplingRate(),
					int64_t rmaxpreisasamplingrate = -1,
					bool rcopyinputtomemory = getDefaultCopyInputToMemory(),
					uint64_t rmaxblocksize = std::numeric_limits<uint64_t>::max(),
					bool rcomputeTermSymbolHwt = false,
					uint64_t rwordsperthread = getDefaultWordsPerThread(),
					uint64_t rlargelcpthres = getDefaultLargeLCPThres(),
					uint64_t rverbose = getDefaultVerbose()
				) :
				  numthreads(rnumthreads),
				  fn(rfn),
				  wordsperthread(rwordsperthread),
				  bwtonly(rbwtonly),
				  mem(rmem),
				  tmpfilenamebase(rtmpfilenamebase.size() ? rtmpfilenamebase : (fn + "_tmp")),
				  sparsetmpfilenamebase(rsparsetmpfilenamebase.size() ? rsparsetmpfilenamebase : tmpfilenamebase),
				  isasamplingrate(risasamplingrate),
				  sasamplingrate(rsasamplingrate),
				  copyinputtomemory(rcopyinputtomemory),
				  computeTermSymbolHwt(rcomputeTermSymbolHwt),
				  maxblocksize(rmaxblocksize),
				  maxpreisasamplingrate((rmaxpreisasamplingrate <= 0) ? (bwtonly ? 64 : 256*1024) : rmaxpreisasamplingrate ),
				  defoutfn(computeDefaultOutputFileName(computeDefaultOutputFileName(fn))),
				  outfn(routfn.size() ? routfn : defoutfn),
				  sinputtype(rsinputtype),
				  inputtype(parseInputType(sinputtype)),
				  largelcpthres(rlargelcpthres),
				  verbose(rverbose)
				{

				}

				/**
				 * constructor based on arginfo object
				 **/
				BwtMergeSortOptions(libmaus2::util::ArgInfo const & arginfo) :
				  numthreads(arginfo.getValueUnsignedNumeric<unsigned int>("numthreads", getDefaultNumThreads())),
				  fn(arginfo.getUnparsedRestArg(0)),
				  wordsperthread(std::max(static_cast<uint64_t>(1),arginfo.getValueUnsignedNumeric<uint64_t>("wordsperthread",getDefaultWordsPerThread()))),
				  bwtonly(arginfo.getValue<unsigned int>("bwtonly",getDefaultBWTOnly())),
				  mem(std::max(static_cast<uint64_t>(1),arginfo.getValueUnsignedNumeric<uint64_t>("mem",getDefaultMem()))),
				  tmpfilenamebase(arginfo.getUnparsedValue("tmpprefix",arginfo.getDefaultTmpFileName())),
				  sparsetmpfilenamebase(arginfo.getUnparsedValue("sparsetmpprefix",tmpfilenamebase)),
				  isasamplingrate(::libmaus2::math::nextTwoPow(arginfo.getValueUnsignedNumeric<uint64_t>("isasamplingrate",getDefaultIsaSamplingRate()))),
				  sasamplingrate(::libmaus2::math::nextTwoPow(arginfo.getValueUnsignedNumeric<uint64_t>("sasamplingrate",getDefaultSaSamplingRate()))),
				  copyinputtomemory(arginfo.getValue<uint64_t>("copyinputtomemory",getDefaultCopyInputToMemory())),
				  computeTermSymbolHwt(arginfo.getValue<int>("computeTermSymbolHwt",false)),
				  maxblocksize(arginfo.getValueUnsignedNumeric<uint64_t>("maxblocksize", std::numeric_limits<uint64_t>::max())),
				  maxpreisasamplingrate(::libmaus2::math::nextTwoPow(arginfo.getValueUnsignedNumeric<uint64_t>("preisasamplingrate",bwtonly ? 64 : 256*1024))),
				  defoutfn(computeDefaultOutputFileName(computeDefaultOutputFileName(fn))),
				  outfn(arginfo.getValue<std::string>("outputfilename",defoutfn)),
				  sinputtype(arginfo.getValue<std::string>("inputtype",getDefaultInputType())),
				  inputtype(parseInputType(sinputtype)),
				  largelcpthres(arginfo.getValueUnsignedNumeric<uint64_t>("largelcpthres", getDefaultLargeLCPThres())),
				  verbose(arginfo.getValueUnsignedNumeric<uint64_t>("verbose", getDefaultVerbose()))
				{

				}
			};
		}
	}
}
#endif
