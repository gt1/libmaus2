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
#if ! defined(LIBMAUS2_LCS_CHAINALIGNMENT_HPP)
#define LIBMAUS2_LCS_CHAINALIGNMENT_HPP

#include <libmaus2/lcs/NNPAlignResult.hpp>

namespace libmaus2
{
	namespace lcs
	{
		struct ChainAlignment
		{
			libmaus2::lcs::NNPAlignResult res;
			uint64_t seedposa;
			uint64_t seedposb;
			#if 0
			libmaus2::lcs::NNPTraceContainer::shared_ptr_type nnptrace;
			#endif

			ChainAlignment() {}
			ChainAlignment(libmaus2::lcs::NNPAlignResult const & rres, uint64_t const rseedposa, uint64_t const rseedposb)
			: res(rres), seedposa(rseedposa), seedposb(rseedposb)
			{

			}
			#if 0
			ChainAlignment(libmaus2::lcs::NNPAlignResult const & rres, libmaus2::lcs::NNPTraceContainer const & rnnptrace)
			: res(rres), nnptrace(rnnptrace.sclone()) {}
			#endif

			bool operator<(ChainAlignment const & O) const
			{
				if ( res.abpos != O.res.abpos )
					return res.abpos < O.res.abpos;
				else if ( res.aepos != O.res.aepos )
					return res.aepos < O.res.aepos;
				else if ( res.bbpos != O.res.bbpos )
					return res.bbpos < O.res.bbpos;
				else if ( res.bepos != O.res.bepos )
					return res.bepos < O.res.bepos;
				else
					return false;
			}

			bool operator==(ChainAlignment const & O) const
			{
				return
					!(*this < O) &&
					!(O < *this);
			}
		};
	}
}
#endif
