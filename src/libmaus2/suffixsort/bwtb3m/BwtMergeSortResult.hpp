/*
    libmaus2
    Copyright (C) 2016 German Tischler

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
#if ! defined(LIBMAUS2_SUFFIXSORT_BWTB3M_BWTMERGESORTRESULT_HPP)
#define LIBMAUS2_SUFFIXSORT_BWTB3M_BWTMERGESORTRESULT_HPP

#include <string>
#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/util/shared_ptr.hpp>
#include <libmaus2/wavelet/ImpCompactHuffmanWaveletTree.hpp>
#include <libmaus2/lf/ImpCompactHuffmanWaveletLF.hpp>
#include <libmaus2/util/NumberMapSerialisation.hpp>
#include <libmaus2/suffixsort/bwtb3m/BWTB3MBase.hpp>
#include <libmaus2/fm/SampledSA.hpp>
#include <libmaus2/fm/SampledISA.hpp>
#include <libmaus2/fm/FM.hpp>
#include <libmaus2/lcp/WaveletLCP.hpp>
#include <libmaus2/lcp/SuccinctLCP.hpp>
#include <libmaus2/rmq/RMMTree.hpp>
#include <libmaus2/util/FileTempFileContainer.hpp>
#include <libmaus2/suffixtree/CompressedSuffixTree.hpp>
#include <libmaus2/suffixsort/bwtb3m/BwtComputeSSA.hpp>
#include <libmaus2/rank/DNARank.hpp>

namespace libmaus2
{
	namespace suffixsort
	{
		namespace bwtb3m
		{
			/**
			 * Result object of BwtMergeSort
			 **/
			struct BwtMergeSortResult : public BWTB3MBase
			{
				typedef BwtMergeSortResult this_type;

				// text file name
				std::string textfn;
				// filename of run length compressed BWT
				std::string bwtfn;
				// symbol histogram map filename
				std::string histfn;
				// filename of huffman shaped wavelet tree
				std::string hwtfn;
				// filename of sampled suffix array (if constructed)
				std::string safn;
				// filename of sampled inverse suffix array (if constructed)
				std::string isafn;
				// filename of pre sampled inverse suffix array meta information (if constructed)
				std::string metafn;
				// filename of pre sampled inverse suffix array (if constructed)
				std::string preisafn;
				// filename of succinct longest common prefix array (if constructed)
				std::string succinctlcpfn;
				// filename of RMM tree over LCP array (if constructed)
				std::string rmmtreefn;

				std::ostream & serialise(std::ostream & out) const
				{
					libmaus2::util::StringSerialisation::serialiseString(out,textfn);
					libmaus2::util::StringSerialisation::serialiseString(out,bwtfn);
					libmaus2::util::StringSerialisation::serialiseString(out,histfn);
					libmaus2::util::StringSerialisation::serialiseString(out,hwtfn);
					libmaus2::util::StringSerialisation::serialiseString(out,safn);
					libmaus2::util::StringSerialisation::serialiseString(out,isafn);
					libmaus2::util::StringSerialisation::serialiseString(out,metafn);
					libmaus2::util::StringSerialisation::serialiseString(out,preisafn);
					libmaus2::util::StringSerialisation::serialiseString(out,succinctlcpfn);
					libmaus2::util::StringSerialisation::serialiseString(out,rmmtreefn);
					return out;
				}

				void serialise(std::string const & fn) const
				{
					libmaus2::aio::OutputStreamInstance OSI(fn);
					serialise(OSI);
				}

				std::istream & deserialise(std::istream & in)
				{
					textfn = libmaus2::util::StringSerialisation::deserialiseString(in);
					bwtfn = libmaus2::util::StringSerialisation::deserialiseString(in);
					histfn = libmaus2::util::StringSerialisation::deserialiseString(in);
					hwtfn = libmaus2::util::StringSerialisation::deserialiseString(in);
					safn = libmaus2::util::StringSerialisation::deserialiseString(in);
					isafn = libmaus2::util::StringSerialisation::deserialiseString(in);
					metafn = libmaus2::util::StringSerialisation::deserialiseString(in);
					preisafn = libmaus2::util::StringSerialisation::deserialiseString(in);
					succinctlcpfn = libmaus2::util::StringSerialisation::deserialiseString(in);
					rmmtreefn = libmaus2::util::StringSerialisation::deserialiseString(in);
					return in;
				}

				void deserialise(std::string const & fn)
				{
					libmaus2::aio::InputStreamInstance ISI(fn);
					deserialise(ISI);
				}

				// type of succinct LCP array as defined in CompressedSuffixArray
				typedef libmaus2::suffixtree::CompressedSuffixTree::lcp_type cst_succinct_lcp_type;
				// type of RMM tree as defined in CompressedSuffixArray
				typedef libmaus2::suffixtree::CompressedSuffixTree::rmm_tree_type cst_rmm_tree_type;

				BwtMergeSortResult();
				BwtMergeSortResult(std::istream & in)
				{
					deserialise(in);
				}
				BwtMergeSortResult(std::string const & fn)
				{
					deserialise(fn);
				}

				struct BareSimpleSampledSuffixArray
				{
					uint64_t samplingrate;
					libmaus2::autoarray::AutoArray<uint64_t> A;

					uint64_t operator[](uint64_t const i) const
					{
						return A[i];
					}
				};

				BareSimpleSampledSuffixArray loadBareSimpleSuffixArray() const
				{
					if ( !safn.size() )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "libmaus2::suffixsort::bwtb3m::BwtMergeSortResult::loadBareSimpleSuffixArray(): sampled suffix array has not been constructed" << std::endl;
						lme.finish();
						throw lme;
					}

					libmaus2::aio::InputStreamInstance ISI(safn);
					typedef libmaus2::fm::SimpleSampledSA<libmaus2::lf::ImpCompactHuffmanWaveletLF> sa_type;
					uint64_t s = 0;
					uint64_t const samplingrate = sa_type::readUnsignedInt(ISI,s);
					libmaus2::autoarray::AutoArray<uint64_t> SA(ISI);

					BareSimpleSampledSuffixArray BSSSA;
					BSSSA.samplingrate = samplingrate;
					BSSSA.A = SA;

					return BSSSA;
				}

				struct CompactBareSimpleSampledSuffixArray
				{
					uint64_t samplingrate;
					libmaus2::bitio::CompactArray::shared_ptr_type A;

					uint64_t operator[](uint64_t const i) const
					{
						return (*A)[i];
					}
				};

				CompactBareSimpleSampledSuffixArray loadCompactBareSimpleSuffixArray(uint64_t const n) const
				{
					if ( !safn.size() )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "libmaus2::suffixsort::bwtb3m::BwtMergeSortResult::loadBareSimpleSuffixArray(): sampled suffix array has not been constructed" << std::endl;
						lme.finish();
						throw lme;
					}

					std::string const compactsafn = safn + ".compact";

					if (
						libmaus2::util::GetFileSize::fileExists(compactsafn)
						&&
						(!libmaus2::util::GetFileSize::isOlder(compactsafn,safn))
					)
					{
						libmaus2::aio::InputStreamInstance ISI(compactsafn);
						uint64_t const samplingrate = libmaus2::util::NumberSerialisation::deserialiseNumber(ISI);
						libmaus2::bitio::CompactArray::shared_ptr_type PSA(new libmaus2::bitio::CompactArray(ISI));

						CompactBareSimpleSampledSuffixArray CBSSSA;
						CBSSSA.samplingrate = samplingrate;
						CBSSSA.A = PSA;

						return CBSSSA;
					}
					else
					{
						libmaus2::aio::InputStreamInstance ISI(safn);

						typedef libmaus2::fm::SimpleSampledSA<libmaus2::lf::ImpCompactHuffmanWaveletLF> sa_type;

						uint64_t s = 0;
						uint64_t const samplingrate = sa_type::readUnsignedInt(ISI,s);

						// read length of array
						uint64_t arraylength;
						::libmaus2::serialize::Serialize<uint64_t>::deserialize(ISI,&arraylength);

						uint64_t todo = arraylength;
						uint64_t const bs = 64*1024;
						libmaus2::autoarray::AutoArray<uint64_t> B(bs,false);
						uint64_t const b = n ? libmaus2::math::numbits(n-1) : 0;
						libmaus2::bitio::CompactArray::shared_ptr_type PSA(new libmaus2::bitio::CompactArray(arraylength,b,0 /* pad */,false /* erase */));
						libmaus2::bitio::CompactArray & SA = *PSA;

						uint64_t z = 0;
						while ( todo )
						{
							uint64_t const pack = std::min(todo,bs);
							::libmaus2::serialize::Serialize<uint64_t>::deserializeArray(ISI,B.begin(),pack);
							for ( uint64_t i = 0; i < pack; ++i )
								SA.set(z++,B[i]);
							todo -= pack;
						}

						CompactBareSimpleSampledSuffixArray CBSSSA;
						CBSSSA.samplingrate = samplingrate;
						CBSSSA.A = PSA;

						libmaus2::aio::OutputStreamInstance OSI(compactsafn);
						libmaus2::util::NumberSerialisation::serialiseNumber(OSI,samplingrate);
						PSA->serialize(OSI);
						OSI.flush();

						return CBSSSA;
					}
				}

				/**
				 * Load a DNARank object from the run length encoded BWT. This requires the alphabet of the original text
				 * to be a subset of 0,1,2,3
				 *
				 * @param numthreads number of threads used for building the DNARank object
				 * @return DNARank object
				 **/
				libmaus2::rank::DNARank::unique_ptr_type loadDNARank(uint64_t const numthreads);
				/**
				 * Load a compressed suffix array object. This creates data structures based on the BWT as required, if they
				 * have not been built already.
				 *
				 * @param numthreads number of threads used for construction
				 * @param tmpprefix prefix for temp files used during construction of aux data structures
				 * @param rmmbuildblocksize size of array used for parallel decoding of LCP array during RMM tree construction
				 * @param logstr stream used for printing progress information. Can be set to NULL to obtain a quiet mode
				 * @return compressed suffix tree
				 **/
				libmaus2::suffixtree::CompressedSuffixTree::unique_ptr_type loadSuffixTree(
					uint64_t const numthreads,
					std::string const & tmpprefix, // = "mem://tmp_lcp_"
					uint64_t const rmmbuildblocksize, // = 8*1024*1024
					std::ostream * logstr // = &std::cerr
				);
				/**
				 * Load the symbol histogram
				 **/
				std::map<int64_t,uint64_t> loadSymbolHistogram() const;
				/**
				 * Construct Huffman shaped wavelet tree (HWT) from run length encoded BWT
				 *
				 * @param tmpfilenamebase prefix for temporary files created
				 * @param numthreads number of threads used during HWT construction
				 * @return HWT
				 **/
				libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type constructHwt(
					std::string const tmpfilenamebase,
					uint64_t const numthreads
				);
				/**
				 * Compute sampled suffix array and sampled inverse suffix array based on pre inverse sampled suffix array.
				 * The pre inverse sampled suffix array is computed if bwtonly is set to true during BWT construction.
				 *
				 * @param sasamplingrate sampling rate for sampled suffix array
				 * @param isasamplingrate sampling rate for sampled inverse suffix array
				 * @param tmpfilenamebase prefix for temporary files used
				 * @param copyinputtomemory load run length encoded BWT to a memory file if true
				 * @param numthreads number of threads used for construction
				 * @param maxsortmem maximum internal memory used for sorting
				 * @param maxtmpfiles guide for number of temporary files open at any time
				 * @param logstr stream used for printing progress information. Can be set to NULL to obtain a quiet mode
				 * @param ref_isa_fn file name of pre computed sampled inverse suffix array (used for debugging if set)
				 * @param ref_sa_fn file name of pre computed sampled suffix array (used for debugging if set)
				 **/
				void computeSampledSuffixArray(
					uint64_t const sasamplingrate,
					uint64_t const isasamplingrate,
					std::string const tmpfilenamebase,
					bool const copyinputtomemory,
					uint64_t const numthreads,
					uint64_t const maxsortmem,
					uint64_t const maxtmpfiles,
					std::ostream * logstr,
					std::string const ref_isa_fn = std::string(),
					std::string const ref_sa_fn = std::string()
				);
				libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type loadWaveletTree(
					std::string const tmpfilenamebase,
					uint64_t const numthreads
				);
				libmaus2::lf::ImpCompactHuffmanWaveletLF::unique_ptr_type loadLF(
					std::string const tmpfilenamebase,
					uint64_t const numthreads
				);
				/**
				 * Load a sampled suffix array. Fails if no suffix array was constructed.
				 *
				 * @param lf LF data structure
				 * @return sampled suffix array
				 **/
				libmaus2::fm::SimpleSampledSA<libmaus2::lf::ImpCompactHuffmanWaveletLF>::unique_ptr_type loadSuffixArray(libmaus2::lf::ImpCompactHuffmanWaveletLF const * lf);
				/**
				 * Load a sampled inverse suffix array. Fails if no inverse suffix array was constructed.
				 *
				 * @param lf LF data structure
				 * @return sampled inverse suffix array
				 **/
				libmaus2::fm::SampledISA<libmaus2::lf::ImpCompactHuffmanWaveletLF>::unique_ptr_type loadInverseSuffixArray(libmaus2::lf::ImpCompactHuffmanWaveletLF const * lf);
				/**
				 * Load FM object.
				 *
				 * @param tmpfilenamebase filename prefix for temporary files used during construction
				 * @param numthreads number of threads used for construction
				 **/
				libmaus2::fm::FM<libmaus2::lf::ImpCompactHuffmanWaveletLF>::unique_ptr_type loadFM(std::string const tmpfilenamebase, uint64_t const numthreads);
				libmaus2::lcp::WaveletLCPResult::unique_ptr_type computeLCP(
					libmaus2::lf::ImpCompactHuffmanWaveletLF const * LF,
					uint64_t const numthreads,
					std::ostream * logstr
				);
				/**
				 * Computed succinct longest common prefix array data structure.
				 *
				 * @param LF data structure for LF operations
				 * @param SA sampled suffix array
				 * @param ISA sampled inverse suffix array
				 * @param numthreads number of threads used for construction
				 * @param tmpprefix prefix for temporary files
				 * @param logstr stream used for printing progress information. Can be set to NULL to obtain a quiet mode
				 **/
				cst_succinct_lcp_type::unique_ptr_type computeSuccinctLCP(
					libmaus2::lf::ImpCompactHuffmanWaveletLF const * LF,
					libmaus2::fm::SimpleSampledSA<libmaus2::lf::ImpCompactHuffmanWaveletLF> const * SA,
					libmaus2::fm::SampledISA<libmaus2::lf::ImpCompactHuffmanWaveletLF> const * ISA,
					uint64_t const numthreads,
					std::string const & tmpprefix, // = "mem://tmp_lcp_"
					std::ostream * logstr
				);
				/**
				 * Compute succinct longest common prefix array data structure.
				 *
				 * @param FM data structure obtained by loadFM
				 * @param SA sampled suffix array
				 * @param ISA sampled inverse suffix array
				 * @param numthreads number of threads used for construction
				 * @param tmpprefix prefix for temporary files
				 * @param logstr stream used for printing progress information. Can be set to NULL to obtain a quiet mode
				 **/
				cst_succinct_lcp_type::unique_ptr_type computeSuccinctLCP(
					libmaus2::fm::FM<libmaus2::lf::ImpCompactHuffmanWaveletLF> * FM,
					uint64_t const numthreads,
					std::string const & tmpprefix, // = "mem://tmp_lcp_"
					std::ostream * logstr
				);
				/**
				 * Compute RMM tree for given succinct longest common prefix array
				 *
				 * @param SLCP succinct longest common prefix array
				 * @param numthreads number of threads used for construction
				 * @param rmmbuildblocksize size of array used for parallel decoding of succinct longest common prefix array
				 * @param logstr stream used for printing progress information. Can be set to NULL to obtain a quiet mode
				 **/
				cst_rmm_tree_type::unique_ptr_type computeRMMTree(cst_succinct_lcp_type const * SLCP, uint64_t const numthreads, uint64_t const rmmbuildblocksize, std::ostream * logstr);
				/**
				 * Load RMM tree for given succinct longest common prefix array. Compute RMM tree if not done already.
				 *
				 * @param SLCP succinct longest common prefix array
				 * @param numthreads number of threads used for construction
				 * @param rmmbuildblocksize size of array used for parallel decoding of succinct longest common prefix array
				 * @param logstr stream used for printing progress information. Can be set to NULL to obtain a quiet mode
				 **/
				cst_rmm_tree_type::unique_ptr_type loadRMMTree(cst_succinct_lcp_type const * SLCP, uint64_t const numthreads, uint64_t const rmmbuildblocksize, std::ostream * logstr);

				/**
				 * Load succinct longest common prefix array data structure. Compute it if not done already.
				 *
				 * @param LF data structure for LF operations
				 * @param SA sampled suffix array
				 * @param ISA sampled inverse suffix array
				 * @param numthreads number of threads used for construction
				 * @param tmpprefix prefix for temporary files
				 * @param logstr stream used for printing progress information. Can be set to NULL to obtain a quiet mode
				 **/
				cst_succinct_lcp_type::unique_ptr_type loadSuccinctLCP(
					libmaus2::lf::ImpCompactHuffmanWaveletLF const * LF,
					libmaus2::fm::SimpleSampledSA<libmaus2::lf::ImpCompactHuffmanWaveletLF> const * SA,
					libmaus2::fm::SampledISA<libmaus2::lf::ImpCompactHuffmanWaveletLF> const * ISA,
					uint64_t const numthreads,
					std::string const & tmpprefix, // = "mem://tmp_lcp_"
					std::ostream * logstr
				);
				/**
				 * Load succinct longest common prefix array data structure. Compute it if not done already.
				 *
				 * @param FM data structure obtained by loadFM
				 * @param SA sampled suffix array
				 * @param ISA sampled inverse suffix array
				 * @param numthreads number of threads used for construction
				 * @param tmpprefix prefix for temporary files
				 * @param logstr stream used for printing progress information. Can be set to NULL to obtain a quiet mode
				 **/
				cst_succinct_lcp_type::unique_ptr_type loadSuccinctLCP(
					libmaus2::fm::FM<libmaus2::lf::ImpCompactHuffmanWaveletLF> const * FM,
					uint64_t const numthreads,
					std::string const & tmpprefix, // = "mem://tmp_lcp_"
					std::ostream * logstr
				);

				/**
				 * Set up object in bwtonly=0 mode.
				 **/
				static BwtMergeSortResult setupBwtSa(
					std::string const & textfn,
					std::string const & bwtfn,
					std::string const & histfn,
					std::string const & hwtfn,
					std::string const & safn,
					std::string const & isafn
				);
				/**
				 * Set up object in bwtonly=1 mode.
				 **/
				static BwtMergeSortResult setupBwtOnly(
					std::string const & textfn,
					std::string const & bwtfn,
					std::string const & histfn,
					std::string const & metafn,
					std::string const & preisafn
				);
			};
		}
	}
}
#endif
