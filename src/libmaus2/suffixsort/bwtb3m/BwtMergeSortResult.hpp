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

				BwtMergeSortResult()
				{

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
