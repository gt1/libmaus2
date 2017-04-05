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
#if ! defined(LIBMAUS2_SUFFIXSORT_BWTB3M_MERGESTRATEGYMERGEINTERNALBLOCK_HPP)
#define LIBMAUS2_SUFFIXSORT_BWTB3M_MERGESTRATEGYMERGEINTERNALBLOCK_HPP

#include <libmaus2/suffixsort/bwtb3m/MergeStrategyMergeBlock.hpp>

namespace libmaus2
{
	namespace suffixsort
	{
		namespace bwtb3m
		{
			struct MergeStrategyMergeInternalBlock : public MergeStrategyMergeBlock
			{
				// std::vector<MergeStrategyBlock::shared_ptr_type> children;

				MergeStrategyMergeInternalBlock()
				: MergeStrategyMergeBlock()
				{
				}

				MergeStrategyMergeInternalBlock(std::istream & in) : MergeStrategyMergeBlock(in) {}

				bool equal(MergeStrategyBlock const & O) const
				{
					if ( dynamic_cast<MergeStrategyMergeInternalBlock const *>(&O) == 0 )
						return false;

					MergeStrategyMergeBlock const & A = *(dynamic_cast<MergeStrategyMergeBlock const *>(this));
					MergeStrategyMergeBlock const & B = *(dynamic_cast<MergeStrategyMergeBlock const *>(&O));

					if ( A != B )
						return false;

					return true;
				}

				void vserialise(std::ostream & out) const
				{
					libmaus2::util::NumberSerialisation::serialiseNumber(out,MergeStrategyBlock::merge_block_type_internal);
					MergeStrategyMergeBlock::serialise(out);
				}

				std::ostream & print(std::ostream & out, uint64_t const indent) const
				{
					return MergeStrategyMergeBlock::print(out,indent,"MergeStrategyMergeInternalBlock");
				}

				static MergeStrategyBlock::shared_ptr_type construct(
					std::vector<MergeStrategyBlock::shared_ptr_type> const children
				)
				{
					MergeStrategyMergeInternalBlock * pobj = new MergeStrategyMergeInternalBlock();
					MergeStrategyBlock::shared_ptr_type sobj(pobj);
					MergeStrategyMergeBlock::construct(pobj,children);
					return sobj;
				}
			};
		}
	}
}
#endif
