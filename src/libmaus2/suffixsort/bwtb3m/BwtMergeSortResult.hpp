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
			struct BwtMergeSortResult : public BWTB3MBase
			{
				typedef BwtMergeSortResult this_type;

				std::string textfn;
				std::string bwtfn;
				std::string histfn;
				std::string hwtfn;
				std::string safn;
				std::string isafn;
				std::string metafn;
				std::string preisafn;
				std::string succinctlcpfn;
				std::string rmmtreefn;

				BwtMergeSortResult()
				{

				}

				typedef ::libmaus2::lcp::SuccinctLCP<
					::libmaus2::lf::ImpCompactHuffmanWaveletLF,
					::libmaus2::fm::SimpleSampledSA< ::libmaus2::lf::ImpCompactHuffmanWaveletLF >,
					::libmaus2::fm::SampledISA< ::libmaus2::lf::ImpCompactHuffmanWaveletLF >
				> succinct_lcp_type;
				typedef libmaus2::rmq::RMMTree<succinct_lcp_type,3> rmm_tree_type;

				libmaus2::rank::DNARank::unique_ptr_type loadDNARank(uint64_t const numthreads)
				{
					std::map<int64_t,uint64_t> const H = loadSymbolHistogram();
					bool const alvalid = H.empty() || H.rbegin()->first < 4;

					if ( alvalid )
					{
						libmaus2::rank::DNARank::unique_ptr_type Prank(libmaus2::rank::DNARank::loadFromRunLength(bwtfn,numthreads));
						return UNIQUE_PTR_MOVE(Prank);
					}
					else
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "libmaus2::suffixsort::bwtb3m::BwtMergeSortResult::loadDNARank: alphabet not suitable for DNARank index" << std::endl;
						lme.finish();
						throw lme;
					}
				}

				libmaus2::suffixtree::CompressedSuffixTree::unique_ptr_type loadSuffixTree(
					uint64_t const numthreads,
					std::string const & tmpprefix, // = "mem://tmp_lcp_"
					uint64_t const rmmbuildblocksize, // = 8*1024*1024
					std::ostream * logstr // = &std::cerr
				)
				{
					// load lf
					libmaus2::lf::ImpCompactHuffmanWaveletLF::unique_ptr_type LF(loadLF(tmpprefix,numthreads));
					// load sampled suffix array
					libmaus2::fm::SimpleSampledSA<libmaus2::lf::ImpCompactHuffmanWaveletLF>::unique_ptr_type SA(loadSuffixArray(LF.get()));
					// load sampled inverse suffix array
					libmaus2::fm::SampledISA<libmaus2::lf::ImpCompactHuffmanWaveletLF>::unique_ptr_type ISA(loadInverseSuffixArray(LF.get()));

					if ( (! succinctlcpfn.size()) || (! rmmtreefn.size()) )
					{
						libmaus2::lcp::WaveletLCPResult::unique_ptr_type WLCP(computeLCP(LF.get(),numthreads,logstr));

						// build rmm tree using (faster) wavelet lcp result, if rmm tree does not exist yet
						if ( ! rmmtreefn.size() )
						{
							typedef libmaus2::rmq::RMMTree<libmaus2::lcp::WaveletLCPResult,rmm_tree_type::klog> wlcp_rmm_tree_type;
							typedef wlcp_rmm_tree_type::unique_ptr_type wlcp_rmm_tree_ptr_type;
							wlcp_rmm_tree_ptr_type RMM(new wlcp_rmm_tree_type(*WLCP,LF->getN(),numthreads,rmmbuildblocksize,logstr));

							rmmtreefn = ::libmaus2::util::OutputFileNameTools::clipOff(bwtfn,".bwt") + ".rmm";
							libmaus2::aio::OutputStreamInstance::unique_ptr_type OSI(new libmaus2::aio::OutputStreamInstance(rmmtreefn));
							RMM->serialise(*OSI);
							OSI->flush();
							OSI.reset();
						}

						// construct succinct lcp array
						libmaus2::util::TempFileNameGenerator tmpgen(tmpprefix,3);
						libmaus2::util::FileTempFileContainer ftmp(tmpgen);
						succinctlcpfn = ::libmaus2::util::OutputFileNameTools::clipOff(bwtfn,".bwt") + ".slcp";
						::libmaus2::aio::OutputStreamInstance::unique_ptr_type lcpCOS(new libmaus2::aio::OutputStreamInstance(succinctlcpfn));
						succinct_lcp_type::writeSuccinctLCP(*LF,*ISA,*WLCP,*lcpCOS,ftmp,numthreads,logstr);
						lcpCOS->flush();
						lcpCOS.reset();
					}

					// load succinct lcp array
					succinct_lcp_type::unique_ptr_type SLCP(loadSuccinctLCP(LF.get(),SA.get(),ISA.get(),numthreads,tmpprefix,logstr));
					// load rmm tree
					rmm_tree_type::unique_ptr_type RMM(loadRMMTree(SLCP.get(),numthreads,rmmbuildblocksize,logstr));

					libmaus2::suffixtree::CompressedSuffixTree::unique_ptr_type PCST(
						new libmaus2::suffixtree::CompressedSuffixTree(
							LF,
							SA,
							ISA,
							SLCP,
							RMM
						)
					);

					return UNIQUE_PTR_MOVE(PCST);
				}

				std::map<int64_t,uint64_t> loadSymbolHistogram() const
				{
					if ( histfn.size() )
					{
						libmaus2::aio::InputStreamInstance ISI(histfn);
						std::map<int64_t,uint64_t> chistnoterm
							= ::libmaus2::util::NumberMapSerialisation::deserialiseMap<std::istream,int64_t,uint64_t>(ISI);
						return chistnoterm;
					}
					else
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "libmaus2::suffixsort::bwtb3m::BwtMergeSortResult::loadSymbolHistogram: symbol histogram has not been constructed" << std::endl;
						lme.finish();
						throw lme;
					}
				}

				libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type constructHwt(
					std::string const tmpfilenamebase,
					uint64_t const numthreads
				)
				{
					std::map<int64_t,uint64_t> const hist = loadSymbolHistogram();
					bool const largealphabet = hist.size() && (hist.rbegin()->first > 255);

					std::string const outhwt = ::libmaus2::util::OutputFileNameTools::clipOff(bwtfn,".bwt") + ".hwt";
					libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type pICHWT;
					if ( largealphabet )
					{
						libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type tICHWT(
							libmaus2::wavelet::RlToHwtBase<true,rl_decoder>::rlToHwt(bwtfn, outhwt, tmpfilenamebase+"_finalhwttmp",numthreads)
						);
						pICHWT = UNIQUE_PTR_MOVE(tICHWT);
					}
					else
					{
						libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type tICHWT(
							libmaus2::wavelet::RlToHwtBase<false,rl_decoder>::rlToHwt(bwtfn, outhwt, tmpfilenamebase+"_finalhwttmp",numthreads)
						);
						pICHWT = UNIQUE_PTR_MOVE(tICHWT);
					}

					hwtfn = outhwt;

					return UNIQUE_PTR_MOVE(pICHWT);
				}

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
				)
				{
					libmaus2::suffixsort::bwtb3m::BwtComputeSSA::computeSSA(
						bwtfn,
						sasamplingrate,
						isasamplingrate,
						tmpfilenamebase,
						copyinputtomemory,
						numthreads,
						maxsortmem,
						maxtmpfiles,
						logstr,
						ref_isa_fn,
						ref_sa_fn
					);

					safn = ::libmaus2::util::OutputFileNameTools::clipOff(bwtfn,".bwt") + ".sa";
					isafn = ::libmaus2::util::OutputFileNameTools::clipOff(bwtfn,".bwt") + ".isa";
				}

				libmaus2::fm::SimpleSampledSA<libmaus2::lf::ImpCompactHuffmanWaveletLF>::unique_ptr_type loadSuffixArray(libmaus2::lf::ImpCompactHuffmanWaveletLF const * lf)
				{
					if ( safn.size() )
					{
						libmaus2::fm::SimpleSampledSA<libmaus2::lf::ImpCompactHuffmanWaveletLF>::unique_ptr_type ptr(
							libmaus2::fm::SimpleSampledSA<libmaus2::lf::ImpCompactHuffmanWaveletLF>::load(lf,safn)
						);
						return UNIQUE_PTR_MOVE(ptr);
					}
					else
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "libmaus2::suffixsort::bwtb3m::BwtMergeSortResult::loadSuffixArray: suffix array has not been constructed" << std::endl;
						lme.finish();
						throw lme;
					}
				}

				libmaus2::fm::SampledISA<libmaus2::lf::ImpCompactHuffmanWaveletLF>::unique_ptr_type loadInverseSuffixArray(libmaus2::lf::ImpCompactHuffmanWaveletLF const * lf)
				{
					if ( isafn.size() )
					{
						libmaus2::fm::SampledISA<libmaus2::lf::ImpCompactHuffmanWaveletLF>::unique_ptr_type ptr(
							libmaus2::fm::SampledISA<libmaus2::lf::ImpCompactHuffmanWaveletLF>::load(lf,isafn)
						);
						return UNIQUE_PTR_MOVE(ptr);
					}
					else
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "libmaus2::suffixsort::bwtb3m::BwtMergeSortResult::loadInverseSuffixArray: inverse suffix array has not been constructed" << std::endl;
						lme.finish();
						throw lme;
					}
				}

				libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type loadWaveletTree(
					std::string const tmpfilenamebase,
					uint64_t const numthreads
				)
				{
					if ( hwtfn.size() )
					{
						libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type hptr(libmaus2::wavelet::ImpCompactHuffmanWaveletTree::load(hwtfn,numthreads));
						return UNIQUE_PTR_MOVE(hptr);
					}
					else
					{
						libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type hptr(constructHwt(tmpfilenamebase,numthreads));
						return UNIQUE_PTR_MOVE(hptr);
					}
				}

				libmaus2::lf::ImpCompactHuffmanWaveletLF::unique_ptr_type loadLF(
					std::string const tmpfilenamebase,
					uint64_t const numthreads
				)
				{
					libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type hptr(loadWaveletTree(tmpfilenamebase,numthreads));
					libmaus2::lf::ImpCompactHuffmanWaveletLF::unique_ptr_type lptr(new libmaus2::lf::ImpCompactHuffmanWaveletLF(hptr));
					return UNIQUE_PTR_MOVE(lptr);
				}

				libmaus2::fm::FM<libmaus2::lf::ImpCompactHuffmanWaveletLF>::unique_ptr_type loadFM(std::string const tmpfilenamebase, uint64_t const numthreads)
				{
					libmaus2::lf::ImpCompactHuffmanWaveletLF::unique_ptr_type PLF(loadLF(tmpfilenamebase,numthreads));
					libmaus2::lf::ImpCompactHuffmanWaveletLF::shared_ptr_type SLF(PLF.release());
					libmaus2::fm::SimpleSampledSA<libmaus2::lf::ImpCompactHuffmanWaveletLF>::unique_ptr_type PSA(loadSuffixArray(SLF.get()));
					libmaus2::fm::SampledISA<libmaus2::lf::ImpCompactHuffmanWaveletLF>::unique_ptr_type PISA(loadInverseSuffixArray(SLF.get()));
					libmaus2::fm::FM<libmaus2::lf::ImpCompactHuffmanWaveletLF>::unique_ptr_type PFM(new
						libmaus2::fm::FM<libmaus2::lf::ImpCompactHuffmanWaveletLF>(SLF,PSA,PISA)
					);
					return UNIQUE_PTR_MOVE(PFM);
				}

				libmaus2::lcp::WaveletLCPResult::unique_ptr_type computeLCP(
					libmaus2::lf::ImpCompactHuffmanWaveletLF const * LF,
					uint64_t const numthreads,
					std::ostream * logstr
				)
				{
					libmaus2::lcp::WaveletLCPResult::unique_ptr_type PLCP(libmaus2::lcp::WaveletLCP::computeLCP(LF,numthreads,false /* zdif */,logstr));
					return UNIQUE_PTR_MOVE(PLCP);
				}

				succinct_lcp_type::unique_ptr_type computeSuccinctLCP(
					libmaus2::lf::ImpCompactHuffmanWaveletLF const * LF,
					libmaus2::fm::SimpleSampledSA<libmaus2::lf::ImpCompactHuffmanWaveletLF> const * SA,
					libmaus2::fm::SampledISA<libmaus2::lf::ImpCompactHuffmanWaveletLF> const * ISA,
					uint64_t const numthreads,
					std::string const & tmpprefix, // = "mem://tmp_lcp_"
					std::ostream * logstr
				)
				{
					libmaus2::lcp::WaveletLCPResult::unique_ptr_type PLCP(libmaus2::lcp::WaveletLCP::computeLCP(LF,numthreads,false /* zdif */,logstr));

					#if 0
					#if defined(_OPENMP)
					#pragma omp parallel for num_threads(numthreads)
					#endif
					for ( uint64_t i = 1; i < LF->getN(); ++i )
					{
						uint64_t const r0 = (*ISA)[i-1];
						uint64_t const r1 = (*ISA)[i];

						int64_t const l0 = (*PLCP)[r0];
						int64_t const l1 = (*PLCP)[r1];

						bool const ok = ( l1 >= l0-1 );
						if ( ! ok )
						{
							std::cerr << "fail " << l0 << " " << l1 << std::endl;
							assert ( ok );
						}
					}
					#endif

					libmaus2::util::TempFileNameGenerator tmpgen(tmpprefix,3);
					libmaus2::util::FileTempFileContainer ftmp(tmpgen);
					succinctlcpfn = ::libmaus2::util::OutputFileNameTools::clipOff(bwtfn,".bwt") + ".slcp";
					::libmaus2::aio::OutputStreamInstance::unique_ptr_type lcpCOS(new libmaus2::aio::OutputStreamInstance(succinctlcpfn));
					succinct_lcp_type::writeSuccinctLCP(*LF,*ISA,*PLCP,*lcpCOS,ftmp,numthreads,logstr);
					lcpCOS->flush();
					lcpCOS.reset();

					libmaus2::aio::InputStreamInstance ISI(succinctlcpfn);
					succinct_lcp_type::unique_ptr_type SLCP(new succinct_lcp_type(ISI,*SA));

					#if 0
					#if defined(_OPENMP)
					#pragma omp parallel for num_threads(numthreads)
					for ( uint64_t i = 0; i < LF->getN(); ++i )
						assert (
							(*SLCP)[i] == (*PLCP)[i]
						);
					#endif
					#endif

					return UNIQUE_PTR_MOVE(SLCP);
				}

				succinct_lcp_type::unique_ptr_type computeSuccinctLCP(
					libmaus2::fm::FM<libmaus2::lf::ImpCompactHuffmanWaveletLF> * FM,
					uint64_t const numthreads,
					std::string const & tmpprefix, // = "mem://tmp_lcp_"
					std::ostream * logstr
				)
				{
					succinct_lcp_type::unique_ptr_type SLCP(computeSuccinctLCP(FM->lf.get(),FM->sa.get(),FM->isa.get(),numthreads,tmpprefix,logstr));
					return UNIQUE_PTR_MOVE(SLCP);
				}

				rmm_tree_type::unique_ptr_type computeRMMTree(succinct_lcp_type const * SLCP, uint64_t const numthreads, uint64_t const rmmbuildblocksize, std::ostream * logstr)
				{
					rmmtreefn = ::libmaus2::util::OutputFileNameTools::clipOff(bwtfn,".bwt") + ".rmm";
					uint64_t const n = SLCP->n;
					rmm_tree_type::unique_ptr_type tree(new rmm_tree_type(*SLCP,n,numthreads,rmmbuildblocksize,logstr));

					libmaus2::aio::OutputStreamInstance::unique_ptr_type POSI(new libmaus2::aio::OutputStreamInstance(rmmtreefn));
					tree->serialise(*POSI);
					POSI->flush();
					POSI.reset();

					return UNIQUE_PTR_MOVE(tree);
				}

				rmm_tree_type::unique_ptr_type loadRMMTree(succinct_lcp_type const * SLCP, uint64_t const numthreads, uint64_t const rmmbuildblocksize, std::ostream * logstr)
				{
					if ( ! rmmtreefn.size() )
					{
						rmm_tree_type::unique_ptr_type tree(computeRMMTree(SLCP,numthreads,rmmbuildblocksize,logstr));
						return UNIQUE_PTR_MOVE(tree);
					}
					else
					{
						libmaus2::aio::InputStreamInstance ISI(rmmtreefn);
						rmm_tree_type::unique_ptr_type tree(new rmm_tree_type(ISI,*SLCP));
						return UNIQUE_PTR_MOVE(tree);
					}
				}

				succinct_lcp_type::unique_ptr_type loadSuccinctLCP(
					libmaus2::lf::ImpCompactHuffmanWaveletLF const * LF,
					libmaus2::fm::SimpleSampledSA<libmaus2::lf::ImpCompactHuffmanWaveletLF> const * SA,
					libmaus2::fm::SampledISA<libmaus2::lf::ImpCompactHuffmanWaveletLF> const * ISA,
					uint64_t const numthreads,
					std::string const & tmpprefix, // = "mem://tmp_lcp_"
					std::ostream * logstr
				)
				{
					if ( ! succinctlcpfn.size() )
					{
						succinct_lcp_type::unique_ptr_type SLCP(computeSuccinctLCP(LF,SA,ISA,numthreads,tmpprefix,logstr));
						return UNIQUE_PTR_MOVE(SLCP);
					}
					else
					{
						libmaus2::aio::InputStreamInstance ISI(succinctlcpfn);
						succinct_lcp_type::unique_ptr_type SLCP(new succinct_lcp_type(ISI,*SA));
						return UNIQUE_PTR_MOVE(SLCP);
					}
				}

				succinct_lcp_type::unique_ptr_type loadSuccinctLCP(
					libmaus2::fm::FM<libmaus2::lf::ImpCompactHuffmanWaveletLF> const * FM,
					uint64_t const numthreads,
					std::string const & tmpprefix, // = "mem://tmp_lcp_"
					std::ostream * logstr
				)
				{
					succinct_lcp_type::unique_ptr_type SLCP(loadSuccinctLCP(FM->lf.get(),FM->sa.get(),FM->isa.get(),numthreads,tmpprefix,logstr));
					return UNIQUE_PTR_MOVE(SLCP);
				}

				static BwtMergeSortResult setupBwtSa(
					std::string const & textfn,
					std::string const & bwtfn,
					std::string const & histfn,
					std::string const & hwtfn,
					std::string const & safn,
					std::string const & isafn
				)
				{
					BwtMergeSortResult B;
					B.textfn = textfn;
					B.bwtfn = bwtfn;
					B.histfn = histfn;
					B.hwtfn = hwtfn;
					B.safn = safn;
					B.isafn = isafn;

					return B;
				}

				static BwtMergeSortResult setupBwtOnly(
					std::string const & textfn,
					std::string const & bwtfn,
					std::string const & histfn,
					std::string const & metafn,
					std::string const & preisafn
				)
				{
					BwtMergeSortResult B;
					B.textfn = textfn;
					B.bwtfn = bwtfn;
					B.histfn = histfn;
					B.metafn = metafn;
					B.preisafn = preisafn;

					return B;
				}
			};
		}
	}
}
#endif
