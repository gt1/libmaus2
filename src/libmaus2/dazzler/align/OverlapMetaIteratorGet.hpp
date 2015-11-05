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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_OVERLAPMETAITERATORGET_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_OVERLAPMETAITERATORGET_HPP

#include <libmaus2/dazzler/align/OverlapIndexer.hpp>
#include <libmaus2/util/iterator.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct OverlapMetaIteratorGet
			{
				typedef OverlapMetaIteratorGet this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				typedef libmaus2::index::ExternalMemoryIndexDecoder<OverlapMeta,OverlapIndexerBase::base_level_log,OverlapIndexerBase::inner_level_log> index_type;

				std::vector<std::string> const V;
				std::vector<std::string> IV;
				libmaus2::autoarray::AutoArray<index_type::unique_ptr_type> I;
				uint64_t t;
				int64_t minaread;
				int64_t maxaread;

				OverlapMetaIteratorGet(std::vector<std::string> const & rV)
				: V(rV), IV(V.size()), I(V.size()), t(0), minaread(std::numeric_limits<int64_t>::max()), maxaread(-1)
				{
					for ( uint64_t i = 0; i < V.size(); ++i )
					{
						IV[i] = libmaus2::dazzler::align::OverlapIndexer::getIndexName(V[i]);
						index_type::unique_ptr_type Tptr(new index_type(IV[i]));
						I[i] = UNIQUE_PTR_MOVE(Tptr);
						t += I[i]->size();

						int64_t const fileminaread = libmaus2::dazzler::align::OverlapIndexer::getMinimumARead(V[i]);

						if ( fileminaread >= 0 )
						{
							int64_t const filemaxaread = libmaus2::dazzler::align::OverlapIndexer::getMaximumARead(V[i]);

							minaread = std::min(minaread,fileminaread);
							maxaread = std::max(maxaread,filemaxaread);
						}
					}
				}

				uint64_t get(uint64_t const i) const
				{
					uint64_t s = 0;
					for ( uint64_t j = 0; j < I.size(); ++j )
					{
						libmaus2::index::ExternalMemoryIndexDecoderFindLargestSmallerResult<OverlapMeta> const R =
							I[j]->findLargestSmaller(OverlapMeta(i,0,0,0,0,0,0),true /* cache only */);
						s += R.blockid;
					}

					return s;
				}

				typedef libmaus2::util::ConstIterator<OverlapMetaIteratorGet,uint64_t> iterator_type;

				iterator_type begin()
				{
					return iterator_type(this,minaread);
				}

				iterator_type end()
				{
					return iterator_type(this,maxaread+1);
				}

				std::vector<uint64_t> getBlockStarts(uint64_t const numblocks)
				{
					std::vector<uint64_t> Q(numblocks+1);

					if ( maxaread >= minaread )
					{
						iterator_type const ita = begin();
						iterator_type const ite = end();

						uint64_t const range = t;
						uint64_t const q = numblocks ? ((range + numblocks - 1) / numblocks) : 0;

						for ( uint64_t i = 0; i < numblocks; ++i )
							Q[i] = std::lower_bound(ita,ite,i*q).i;

						Q[numblocks] = ite.i; // upper bound

						// sanity check
						for ( uint64_t i = 1; i < Q.size(); ++i )
						{
							if ( ! (Q[i-1] <= Q[i]) )
							{
								libmaus2::exception::LibMausException lme;
								lme.getStream() << "OverlapMetaIteratorGet::getBlockStarts(): monotonicity check failed on vector ";
								for ( uint64_t j = 0; j < Q.size(); ++j )
									lme.getStream() << Q[j] << ";";
								lme.getStream() << std::endl;
								lme.getStream() << "minaread=" << minaread << " maxaread=" << maxaread << std::endl;
								lme.finish();
								throw lme;
							}
						}
					}

					return Q;
				}
			};
		}
	}
}
#endif
