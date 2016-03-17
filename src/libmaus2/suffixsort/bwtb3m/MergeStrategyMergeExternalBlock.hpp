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
#if ! defined(LIBMAUS2_SUFFIXSORT_BWTB3M_MERGESTRATEGYMERGEEXTERNALBLOCK_HPP)
#define LIBMAUS2_SUFFIXSORT_BWTB3M_MERGESTRATEGYMERGEEXTERNALBLOCK_HPP

#include <libmaus2/suffixsort/bwtb3m/MergeStrategyMergeBlock.hpp>

namespace libmaus2
{
	namespace suffixsort
	{
		namespace bwtb3m
		{
			struct MergeStrategyMergeExternalBlock : public MergeStrategyMergeBlock
			{
				// std::vector<MergeStrategyBlock::shared_ptr_type> children;

				MergeStrategyMergeExternalBlock()
				: MergeStrategyMergeBlock()
				{
				}

				std::ostream & print(std::ostream & out, uint64_t const indent) const
				{
					return MergeStrategyMergeBlock::print(out,indent,"MergeStrategyMergeExternalBlock");
				}

				static MergeStrategyBlock::shared_ptr_type construct(
					std::vector<MergeStrategyBlock::shared_ptr_type> const children
				)
				{
					MergeStrategyMergeExternalBlock * pobj = new MergeStrategyMergeExternalBlock();
					MergeStrategyBlock::shared_ptr_type sobj(pobj);
					MergeStrategyMergeBlock::construct(pobj,children);
					return sobj;
				}
			};

			std::ostream & operator<<(std::ostream & out, MergeStrategyBlock const & MSB)
			{
				return MSB.print(out,0);
			}
		}
	}
}
#endif
