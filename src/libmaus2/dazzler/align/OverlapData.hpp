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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_OVERLAPDATA_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_OVERLAPDATA_HPP

#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/util/LELoad.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct OverlapData
			{
				libmaus2::autoarray::AutoArray<uint8_t> Adata;
				libmaus2::autoarray::AutoArray<uint64_t> Aoffsets;
				libmaus2::autoarray::AutoArray<uint64_t> Alength;
				uint64_t overlapsInBuffer;

				OverlapData() : overlapsInBuffer(0) {}

				std::pair<uint8_t const *, uint8_t const *> getData(uint64_t const i) const
				{
					if ( i < overlapsInBuffer )
					{
						return
							std::pair<uint8_t const *, uint8_t const *>(
								Adata.begin() + Aoffsets[i],
								Adata.begin() + Aoffsets[i] + Alength[i]
							);
					}
					else
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "OverlapParser::getData(): index " << i << " is out of range" << std::endl;
						lme.finish();
						throw lme;
					}
				}

				std::string getDataAsString(uint64_t const i) const
				{
					std::pair<uint8_t const *, uint8_t const *> P = getData(i);
					return std::string(P.first,P.second);
				}

				uint64_t size() const
				{
					return overlapsInBuffer;
				}

				void swap(OverlapData & rhs)
				{
					Adata.swap(rhs.Adata);
					Aoffsets.swap(rhs.Aoffsets);
					Alength.swap(rhs.Alength);
					std::swap(overlapsInBuffer,rhs.overlapsInBuffer);
				}

				static int32_t getTLen(uint8_t const * p)
				{
					return libmaus2::util::loadValueLE4(p + 0*sizeof(int32_t));
				}

				static int32_t getDiffs(uint8_t const * p)
				{
					return libmaus2::util::loadValueLE4(p + 1*sizeof(int32_t));
				}

				static int32_t getABPos(uint8_t const * p)
				{
					return libmaus2::util::loadValueLE4(p + 2*sizeof(int32_t));
				}

				static int32_t getBBPos(uint8_t const * p)
				{
					return libmaus2::util::loadValueLE4(p + 3*sizeof(int32_t));
				}

				static int32_t getAEPos(uint8_t const * p)
				{
					return libmaus2::util::loadValueLE4(p + 4*sizeof(int32_t));
				}

				static int32_t getBEPos(uint8_t const * p)
				{
					return libmaus2::util::loadValueLE4(p + 5*sizeof(int32_t));
				}

				static int32_t getFlags(uint8_t const * p)
				{
					return libmaus2::util::loadValueLE4(p + 6*sizeof(int32_t));
				}

				static int32_t getARead(uint8_t const * p)
				{
					return libmaus2::util::loadValueLE4(p + 7*sizeof(int32_t));
				}

				static int32_t getBRead(uint8_t const * p)
				{
					return libmaus2::util::loadValueLE4(p + 8*sizeof(int32_t));
				}

				static uint8_t const * getTraceData(uint8_t const * p)
				{
					return p + 10*sizeof(int32_t);
				}

				static uint64_t decodeTraceVector(uint8_t const * p, libmaus2::autoarray::AutoArray<std::pair<uint16_t,uint16_t> > & A, bool const small)
				{
					int32_t const tlen = getTLen(p);
					assert ( (tlen & 1) == 0 );

					int32_t const tlen2 = tlen/2;
					if ( tlen2 > static_cast<int64_t>(A.size()) )
						A.resize(tlen2);

					if ( small )
					{
						uint8_t const * tp = getTraceData(p);

						for ( int32_t i = 0; i < tlen2; ++i )
						{
							A[i].first = tp[0];
							A[i].second = tp[1];
							tp += 2;
						}
					}
					else
					{
						uint8_t const * tp = getTraceData(p);

						for ( int32_t i = 0; i < tlen2; ++i )
						{
							A[i].first  = libmaus2::util::loadValueLE2(tp);
							A[i].second = libmaus2::util::loadValueLE2(tp+sizeof(int16_t));
							tp += 2*sizeof(int16_t);
						}
					}

					return tlen2;
				}
			};
		}
	}
}
#endif
