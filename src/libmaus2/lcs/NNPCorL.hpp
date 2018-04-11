/*
    libmaus2
    Copyright (C) 2017 German Tischler

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
#if !defined(LIBMAUS2_LCS_NNPCORL_HPP)
#define LIBMAUS2_LCS_NNPCORL_HPP

#include <libmaus2/lcs/NNPCor.hpp>
#include <libmaus2/lcs/NPLLinMem.hpp>
#include <libmaus2/lcs/NPLinMem.hpp>

namespace libmaus2
{
	namespace lcs
	{
		struct NNPCorL : public libmaus2::lcs::AlignmentTraceContainer
		{
			typedef NNPCorL this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			libmaus2::lcs::NNPCor cor;
			NNPTraceContainer tracecontainer;

			libmaus2::lcs::NPLLinMem o_npl;
			libmaus2::lcs::NPLinMem o_np;

			NNPCorL(
				double const mincor = NNPCor::getDefaultMinCorrelation(),
				double const minlocalcor = NNPCor::getDefaultMinLocalCorrelation(),
				int64_t const rmaxback = NNPCor::getDefaultMaxBack(),
				bool const rfuniquetermval = NNPCor::getDefaultUniqueTermVal(),
				bool const rrunsuffixpositive = NNPCor::getDefaultRunSuffixPositive()
			) : cor(mincor,minlocalcor,rmaxback,rfuniquetermval,rrunsuffixpositive) {}

			void reset()
			{
				cor.reset();
			}

			template<typename iterator>
                        std::pair<uint64_t,uint64_t> alignForward(
                                iterator ab,
                                iterator ae,
                                iterator bb,
                                iterator be
                        )
                        {
                        	libmaus2::lcs::AlignmentTraceContainer::reset();
                        	libmaus2::lcs::NNPAlignResult const nres = cor.alignLinMemForward(ab,ae,bb,be,*this);

                        	assert ( nres.abpos <= nres.aepos );
                        	assert ( static_cast<ptrdiff_t>(nres.aepos) <= ae-ab );
                        	assert ( nres.bbpos <= nres.bepos );
                        	assert ( static_cast<ptrdiff_t>(nres.bepos) <= be-bb );
                        	assert ( nres.abpos == 0 || nres.aepos-nres.abpos == 0 );
                        	assert ( nres.bbpos == 0 || nres.bepos-nres.bbpos == 0 );

				return std::pair<uint64_t,uint64_t>(nres.aepos-nres.abpos,nres.bepos-nres.bbpos);
                        }

			template<typename iterator>
			bool npCheck(
                                iterator ab,
                                iterator ae,
                                iterator bb,
                                iterator be
                        )
                        {
                        	std::pair<uint64_t,uint64_t> SL = alignForward(ab,ae,bb,be);

                        	uint64_t const n_a = ae-ab;
                        	uint64_t const n_b = be-bb;

                        	bool const ok = (SL.first == n_a) && (SL.second == n_b);

				return ok;
			}

			template<typename iterator>
			bool nplCheck(
                                iterator ab,
                                iterator ae,
                                iterator bb,
                                iterator be
                        )
                        {
                        	std::pair<uint64_t,uint64_t> SL = alignForward(ab,ae,bb,be);

                        	uint64_t const n_a = ae-ab;
                        	uint64_t const n_b = be-bb;

                        	bool const ok = (SL.first == n_a) || (SL.second == n_b);

				return ok;
			}

			template<typename iterator>
                        std::pair<uint64_t,uint64_t> np(
                                iterator ab,
                                iterator ae,
                                iterator bb,
                                iterator be
                        )
                        {
                        	std::pair<uint64_t,uint64_t> SL = alignForward(ab,ae,bb,be);

                        	uint64_t const n_a = ae-ab;
                        	uint64_t const n_b = be-bb;

                        	assert ( SL.first <= n_a );
                        	assert ( SL.second <= n_b );

                        	if ( SL.first != n_a || SL.second != n_b )
                        	{
                        		o_np.np(
                        			ab + SL.first, ae,
                        			bb + SL.second,be
                        		);

                        		push(o_np);

                        		std::pair<uint64_t,uint64_t> SL_O = o_np.getStringLengthUsed();
                        		SL.first  += SL_O.first;
                        		SL.second += SL_O.second;
                        	}

                        	return SL;
                        }

			template<typename iterator>
                        std::pair<uint64_t,uint64_t> npl(
                                iterator ab,
                                iterator ae,
                                iterator bb,
                                iterator be
                        )
                        {
                        	std::pair<uint64_t,uint64_t> SL = alignForward(ab,ae,bb,be);

                        	uint64_t const n_a = ae-ab;
                        	uint64_t const n_b = be-bb;

                        	assert ( SL.first <= n_a );
                        	assert ( SL.second <= n_b );

                        	if ( SL.first != n_a && SL.second != n_b )
                        	{
                        		o_npl.np(
                        			ab + SL.first, ae,
                        			bb + SL.second,be
                        		);

                        		push(o_npl);

                        		std::pair<uint64_t,uint64_t> SL_O = o_npl.getStringLengthUsed();
                        		SL.first  += SL_O.first;
                        		SL.second += SL_O.second;
                        	}

                        	return SL;
                        }

                        libmaus2::lcs::AlignmentTraceContainer const & getTraceContainer() const
                        {
                        	return *this;
                        }
		};
	}
}
#endif
