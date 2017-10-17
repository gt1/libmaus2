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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_LASINTERVALS_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_LASINTERVALS_HPP

#include <libmaus2/dazzler/align/AlignmentFileCat.hpp>
#include <libmaus2/geometry/RangeSet.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct LasIntervals
			{
				struct LasIntervalsIndexEntry
				{
					uint64_t from;
					uint64_t to;
					uint64_t index;

					LasIntervalsIndexEntry()
					{

					}

					LasIntervalsIndexEntry(uint64_t const rfrom, uint64_t const rto, uint64_t const rindex)
					: from(rfrom), to(rto), index(rindex)
					{

					}

					uint64_t getFrom() const
					{
						return from;
					}

					uint64_t getTo() const
					{
						return to;
					}
				};

				std::vector < std::string > Vin;
				std::vector < libmaus2::math::IntegerInterval<int64_t> > VI;
				std::vector < LasIntervalsIndexEntry > IV;
				libmaus2::geometry::RangeSet<LasIntervalsIndexEntry> R;

				std::vector<uint64_t> getIds(uint64_t const from, uint64_t const to) const
				{
					std::vector<LasIntervalsIndexEntry const *> V = R.search(
						LasIntervalsIndexEntry(from,to,0)
					);

					std::vector<uint64_t> R;
					for ( uint64_t i = 0; i < V.size(); ++i )
						R.push_back(V[i]->index);

					std::sort(R.begin(),R.end());

					return R;
				}


				libmaus2::dazzler::align::AlignmentFileCat::unique_ptr_type openRange(uint64_t const from, uint64_t const to) const
				{
					std::vector<uint64_t> const R = getIds(from,to);

					libmaus2::dazzler::align::AlignmentFileCat::unique_ptr_type ptr(
						new libmaus2::dazzler::align::AlignmentFileCat(
							Vin,R,from,to
						)
					);

					return UNIQUE_PTR_MOVE(ptr);
				}

				LasIntervals(std::vector<std::string> const & rVin, uint64_t const nreads, std::ostream & errOSI)
				: Vin(rVin), R(nreads)
				{
					std::vector < libmaus2::math::IntegerInterval<int64_t> > VI;
					for ( uint64_t i = 0; i < Vin.size(); ++i )
					{
						if (
							! libmaus2::util::GetFileSize::fileExists(libmaus2::dazzler::align::OverlapIndexer::getIndexName(Vin[i]))
							||
							libmaus2::util::GetFileSize::isOlder(
								libmaus2::dazzler::align::OverlapIndexer::getIndexName(Vin[i]),
								Vin[i]
							)
						)
						{
							libmaus2::dazzler::align::OverlapIndexer::constructIndex(Vin[i]);
							errOSI << "[V] constructing index for " << Vin[i] << "\n";

						}

						int64_t const mini = libmaus2::dazzler::align::OverlapIndexer::getMinimumARead(Vin[i]);
						int64_t const maxi = libmaus2::dazzler::align::OverlapIndexer::getMaximumARead(Vin[i]);

						if ( mini < 0 )
						{
							VI.push_back(libmaus2::math::IntegerInterval<int64_t>::empty());
						}
						else
						{
							VI.push_back(libmaus2::math::IntegerInterval<int64_t>(mini,maxi));
						}
					}

					// collect non empty intervals
					std::vector < libmaus2::math::IntegerInterval<int64_t> > VIE;
					for ( uint64_t i = 0; i < VI.size(); ++i )
						if ( !VI[i].isEmpty() )
							VIE.push_back(VI[i]);
					for ( uint64_t i = 1; i < VIE.size(); ++i )
						if ( !(VIE[i-1].to < VIE[i].from) )
						{
							libmaus2::exception::LibMausException lme;
							lme.getStream() << "[E] input LAS files are not in A-read order" << std::endl;
							lme.finish();
							throw lme;
						}

					for ( uint64_t i = 0; i < VI.size(); ++i )
						if ( !VI[i].isEmpty() )
						{
							IV.push_back(LasIntervalsIndexEntry(VI[i].from,VI[i].to+1,i));
						}

					if ( IV.size() )
					{
						IV[0].from = 0;

						for ( uint64_t i = 1; i < IV.size(); ++i )
							IV[i-1].to = IV[i].from;

						IV.back().to = nreads;
					}

					for ( uint64_t i = 0; i < IV.size(); ++i )
						R.insert(IV[i]);
				}
			};
		}
	}
}
#endif
