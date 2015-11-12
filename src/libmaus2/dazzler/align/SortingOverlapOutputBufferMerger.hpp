/*
    libmaus2
    Copyright (C) 2015 German Tischler

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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_SORTINGOVERLAPOUTPUTBUFFERMERGER_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_SORTINGOVERLAPOUTPUTBUFFERMERGER_HPP

#include <libmaus2/dazzler/align/SortingOverlapBlockInput.hpp>
#include <queue>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			template<typename comparator_type>
			struct SortingOverlapOutputBufferMerger
			{
				typedef SortingOverlapOutputBufferMerger this_type;
				typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

				libmaus2::aio::InputStreamInstance ISI;
				bool const small;
				std::vector< std::pair<uint64_t,uint64_t> > const blocks;
				libmaus2::autoarray::AutoArray < typename SortingOverlapBlockInput<comparator_type>::unique_ptr_type > B;
				comparator_type comparator;
				std::priority_queue<
					std::pair<uint64_t,libmaus2::dazzler::align::Overlap>,
					std::vector< std::pair<uint64_t,libmaus2::dazzler::align::Overlap> >,
					OverlapHeapComparator<comparator_type>
				> Q;

				SortingOverlapOutputBufferMerger(
					std::string const & filename,
					bool const rsmall,
					std::vector< std::pair<uint64_t,uint64_t> > const & rblocks,
					uint64_t const inbufsize = 1024,
					comparator_type rcomparator = comparator_type())
				: ISI(filename), small(rsmall), blocks(rblocks), B(blocks.size()), comparator(rcomparator), Q(comparator)
				{
					for ( uint64_t i = 0; i < B.size(); ++i )
					{
						typename SortingOverlapBlockInput<comparator_type>::unique_ptr_type T(new SortingOverlapBlockInput<comparator_type>(ISI,small,blocks[i],inbufsize,comparator));
						B[i] = UNIQUE_PTR_MOVE(T);
					}
					for ( uint64_t id = 0; id < B.size(); ++id )
					{
						libmaus2::dazzler::align::Overlap NOVL;
						if ( B[id]->getNext(NOVL) )
							Q.push(std::pair<uint64_t,libmaus2::dazzler::align::Overlap>(id,NOVL));
					}
				}

				bool getNext(libmaus2::dazzler::align::Overlap & OVL)
				{
					if ( Q.size() )
					{
						std::pair<uint64_t,libmaus2::dazzler::align::Overlap> const P = Q.top();
						Q.pop();
						uint64_t const id = P.first;
						OVL = P.second;

						libmaus2::dazzler::align::Overlap NOVL;
						if ( B[id]->getNext(NOVL) )
							Q.push(std::pair<uint64_t,libmaus2::dazzler::align::Overlap>(id,NOVL));

						return true;
					}
					else
					{
						return false;
					}
				}
			};
		}
	}
}
#endif
