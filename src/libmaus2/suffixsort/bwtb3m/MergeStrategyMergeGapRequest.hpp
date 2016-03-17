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
#if ! defined(LIBMAUS2_SUFFIXSORT_BWTB3M_MERGESTRATEGYMERGEGAPREQUEST_HPP)
#define LIBMAUS2_SUFFIXSORT_BWTB3M_MERGESTRATEGYMERGEGAPREQUEST_HPP

#include <libmaus2/suffixsort/BwtMergeZBlock.hpp>
#include <libmaus2/suffixsort/bwtb3m/MergeStrategyBlock.hpp>

namespace libmaus2
{
	namespace suffixsort
	{
		namespace bwtb3m
		{
			struct MergeStrategyMergeGapRequest
			{
				typedef MergeStrategyMergeGapRequest this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				std::vector<MergeStrategyBlock::shared_ptr_type> const * pchildren;
				uint64_t into;
				std::vector < ::libmaus2::suffixsort::BwtMergeZBlock > zblocks;

				MergeStrategyMergeGapRequest() : pchildren(0), into(0), zblocks() {}
				MergeStrategyMergeGapRequest(
					std::vector<MergeStrategyBlock::shared_ptr_type> const * rpchildren,
					uint64_t const rinto)
				: pchildren(rpchildren), into(rinto), zblocks()
				{
				}

				std::vector<uint64_t> getQueryPositions(uint64_t const t) const
				{
					assert ( pchildren );
					assert ( pchildren->size() );
					assert ( into != pchildren->size()-1 );
					assert ( t );

					std::vector<MergeStrategyBlock::shared_ptr_type> const & children = *pchildren;
					std::vector<uint64_t> Q;

					// absolute low pos of inserted blocks
					uint64_t const abslow = children[into+1]->low;
					// absolute high pos of inserted blocks
					uint64_t const abshigh = children[children.size()-1]->high;
					// acc size of inserted blocks
					uint64_t const abssize = abshigh-abslow;
					assert ( abssize );
					// distance between query points
					uint64_t const absdif = (abssize + t - 1)/t;

					// count query positions
					uint64_t n = 0;
					for ( uint64_t i = 0; i < t; ++i )
						if ( i * absdif < abssize )
							++n;

					// store query points (falling positions)
					Q.resize(n);
					for ( uint64_t i = 0; i < t; ++i )
						if ( i * absdif < abssize )
							Q[i] = abshigh - i * absdif;

					// check query points
					for ( uint64_t i = 0; i < Q.size(); ++i )
						assert ( Q[i] > abslow && Q[i] <= abshigh );

					return Q;
				}

				void getQueryPositionObjects(
					std::vector < MergeStrategyMergeGapRequestQueryObject > & VV,
					uint64_t const t
				)
				{
					std::vector<uint64_t> const Q = getQueryPositions(t);

					for ( uint64_t i = 0; i < Q.size(); ++i )
						VV.push_back(MergeStrategyMergeGapRequestQueryObject(Q[i],0,this));
				}

				libmaus2::autoarray::AutoArray<MergeStrategyMergeGapRequestQueryObject> getQueryPositionObjects(uint64_t const t)
				{
					std::vector<uint64_t> const Q = getQueryPositions(t);
					libmaus2::autoarray::AutoArray<MergeStrategyMergeGapRequestQueryObject> VV(Q.size());

					for ( uint64_t i = 0; i < Q.size(); ++i )
						VV[i] = MergeStrategyMergeGapRequestQueryObject(Q[i],0,this);

					return VV;
				}
			};

			std::ostream & operator<<(std::ostream & out, MergeStrategyMergeGapRequest const & G);
		}
	}
}
#endif
