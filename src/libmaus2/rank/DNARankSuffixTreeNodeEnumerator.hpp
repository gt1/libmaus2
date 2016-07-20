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
#if ! defined(LIBMAUS2_RANK_DNARANKSUFFIXTREENODEENUMERATOR_HPP)
#define LIBMAUS2_RANK_DNARANKSUFFIXTREENODEENUMERATOR_HPP

#include <libmaus2/rank/DNARankSuffixTreeNodeEnumeratorQueueElement.hpp>
#include <libmaus2/rank/DNARank.hpp>
#include <libmaus2/util/SimpleQueue.hpp>

namespace libmaus2
{
	namespace rank
	{
		struct DNARankSuffixTreeNodeEnumerator
		{
			typedef DNARankSuffixTreeNodeEnumerator this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			libmaus2::rank::DNARank & rank;
			libmaus2::util::SimpleQueue<DNARankSuffixTreeNodeEnumeratorQueueElement> Q;
			libmaus2::rank::DNARankBiDirRange O[LIBMAUS2_RANK_DNARANK_SIGMA+1];
			libmaus2::rank::DNARankBiDirRange OO[LIBMAUS2_RANK_DNARANK_SIGMA];
			libmaus2::rank::DNARankBiDirRangeSizeComparator const comp;

			DNARankSuffixTreeNodeEnumerator(libmaus2::rank::DNARank & rrank) : rank(rrank), Q(), comp()
			{
				if ( rank.size() )
					Q.push_back(DNARankSuffixTreeNodeEnumeratorQueueElement(rank.epsilonBi(),0));
				O[LIBMAUS2_RANK_DNARANK_SIGMA].size = 0;
			}

			DNARankSuffixTreeNodeEnumerator(libmaus2::rank::DNARank & rrank, DNARankSuffixTreeNodeEnumeratorQueueElement const & E) : rank(rrank), Q(), comp()
			{
				Q.push_back(E);
				O[LIBMAUS2_RANK_DNARANK_SIGMA].size = 0;
			}

			bool getNext(DNARankSuffixTreeNodeEnumeratorQueueElement & E)
			{
				if ( !Q.empty() )
				{
					// get next element from stack
					E = Q.pop_back();

					// compute possible extensions on the left
					rank.backwardExtendBi(E.intv, &O[0]);

					// sort descending by range size
					std::stable_sort(&O[0],&O[LIBMAUS2_RANK_DNARANK_SIGMA],comp);

					for ( uint64_t i = 0; O[i].size; ++i )
					{
						// compute possible extensions on the right
						rank.forwardExtendBi(O[i], &OO[0]);

						uint64_t nz = 0;
						for ( uint64_t j = 0; j < LIBMAUS2_RANK_DNARANK_SIGMA; ++j )
							nz += (OO[j].size != 0);

						// if more than one extension on the right is possible
						if ( nz > 1 )
						{
							// push element
							Q.push_back(DNARankSuffixTreeNodeEnumeratorQueueElement(O[i],E.sdepth+1));
						}
					}

					return true;
				}
				else
				{
					return false;
				}
			}

			bool getNextBFS(DNARankSuffixTreeNodeEnumeratorQueueElement & E)
			{
				if ( !Q.empty() )
				{
					// get next element from queue
					E = Q.pop_front();

					// compute possible extensions on the left
					rank.backwardExtendBi(E.intv, &O[0]);

					// sort descending by range size
					std::stable_sort(&O[0],&O[LIBMAUS2_RANK_DNARANK_SIGMA],comp);

					for ( uint64_t i = 0; O[i].size; ++i )
					{
						// compute possible extensions on the right
						rank.forwardExtendBi(O[i], &OO[0]);

						uint64_t nz = 0;
						for ( uint64_t j = 0; j < LIBMAUS2_RANK_DNARANK_SIGMA; ++j )
							nz += (OO[j].size != 0);

						// if more than one extension on the right is possible
						if ( nz > 1 )
						{
							// push element
							Q.push_back(DNARankSuffixTreeNodeEnumeratorQueueElement(O[i],E.sdepth+1));
						}
					}

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
#endif
