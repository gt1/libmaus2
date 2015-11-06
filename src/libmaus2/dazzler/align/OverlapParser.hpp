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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_OVERLAPPARSER_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_OVERLAPPARSER_HPP

#include <libmaus2/dazzler/align/Overlap.hpp>
#include <libmaus2/dazzler/align/AlignmentFile.hpp>
#include <libmaus2/util/LELoad.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct OverlapParser
			{
				enum overlap_parser_state {
					overlap_parser_reading_record_length, overlap_parser_reading_record
				};

				int64_t const tspace;
				bool const small;

				overlap_parser_state state;
				unsigned int recordLengthRead;
				uint8_t recordLengthArray[sizeof(int32_t)];
				uint64_t recordLength;
				uint64_t recordRead;
				uint64_t overlapsInBuffer;

				libmaus2::autoarray::AutoArray<uint8_t> Atmp;
				libmaus2::autoarray::AutoArray<uint8_t> Adata;
				libmaus2::autoarray::AutoArray<uint64_t> Aoffsets;
				libmaus2::autoarray::AutoArray<uint64_t> Alength;

				uint8_t * tmpptr;

				OverlapParser(int64_t const rtspace)
				: tspace(rtspace), small(AlignmentFile::tspaceToSmall(tspace)), state(overlap_parser_reading_record_length), recordLengthRead(0), recordLength(0), overlapsInBuffer(0),
				  tmpptr(0)
				{

				}

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

				uint64_t parseBlock(uint8_t const * data, uint8_t const * data_e)
				{
					uint8_t * dataptr = Adata.begin();
					uint64_t * offsetptr = Aoffsets.begin();
					uint64_t * lengthptr = Alength.begin();

					while ( (data != data_e) )
					{
						switch ( state )
						{
							case overlap_parser_reading_record_length:
								assert ( data != data_e );
								if (
									(!recordLengthRead)
									&&
									data_e-data >= static_cast<ptrdiff_t>(sizeof(recordLengthArray))
									&&
									data_e-data >= static_cast<ptrdiff_t>(6 * sizeof(int32_t) + 4 * sizeof(int32_t) + (small ? 1 : 2) * libmaus2::util::loadValueLE4(data))
								)
								{
									uint64_t reclen = 0;

									while (
										data_e-data >= static_cast<ptrdiff_t>(sizeof(recordLengthArray))
										&&
										data_e-data >= static_cast<ptrdiff_t>((reclen=6 * sizeof(int32_t) + 4 * sizeof(int32_t) + (small ? 1 : 2) * libmaus2::util::loadValueLE4(data)))
									)
									{
										while ( (Adata.end() - dataptr) < static_cast<ptrdiff_t>(reclen) )
										{
											uint64_t const dataptroffset = dataptr - Adata.begin();
											uint64_t const newsize = std::max(static_cast<uint64_t>(1),static_cast<uint64_t>(2*Adata.size()));
											Adata.resize(newsize);
											dataptr = Adata.begin() + dataptroffset;
											// std::cerr << "resize data array to " << Adata.size() << std::endl;
										}
										while ( (Aoffsets.end()-offsetptr) < 1 )
										{
											uint64_t const offsetptroffset = offsetptr - Aoffsets.begin();
											uint64_t const newsize = std::max(static_cast<uint64_t>(1),static_cast<uint64_t>(2*Aoffsets.size()));
											Aoffsets.resize(newsize);
											offsetptr = Aoffsets.begin() + offsetptroffset;
											// std::cerr << "resize offsets array to " << Aoffsets.size() << std::endl;
										}
										while ( (Alength.end()-lengthptr) < 1 )
										{
											uint64_t const lengthptroffset = lengthptr - Alength.begin();
											uint64_t const newsize = std::max(static_cast<uint64_t>(1),static_cast<uint64_t>(2*Alength.size()));
											Alength.resize(newsize);
											lengthptr = Alength.begin() + lengthptroffset;
											// std::cerr << "resize length array to " << Alength.size() << std::endl;
										}

										std::copy(data,data+reclen,dataptr);
										*(offsetptr++) = dataptr - Adata.begin();
										*(lengthptr++) = reclen;

										dataptr += reclen;
										data += reclen;
									}
								}
								else
								{
									while ( (data != data_e) && (recordLengthRead < sizeof(recordLengthArray)) )
										recordLengthArray[recordLengthRead++] = *(data++);
									if ( recordLengthRead == sizeof(recordLengthArray) )
									{
										state = overlap_parser_reading_record;
										recordLength =
											5 * sizeof(int32_t) + // path meta - tlen
											4 * sizeof(int32_t) + // overlap meta
											(small ? 1 : 2) * libmaus2::util::loadValueLE4(&recordLengthArray[0]) // path
											;
										recordRead = 0;

										while ( Atmp.size() < recordLength+sizeof(int32_t) )
										{
											uint64_t const size = Atmp.size();
											uint64_t const one = 1;
											uint64_t const newsize = std::max(one,size<<1);
											Atmp.resize(newsize);
										}
										tmpptr = Atmp.begin();

										// std::cerr << "got record length " << recordLength << std::endl;

										std::copy(&recordLengthArray[0],&recordLengthArray[sizeof(recordLengthArray)],tmpptr);
										tmpptr += sizeof(int32_t);
									}
								}
								break;
							case overlap_parser_reading_record:
								while ( (data != data_e) && (recordRead < recordLength) )
								{
									*(tmpptr++) = *(data++);
									recordRead++;
								}
								if ( recordRead == recordLength )
								{
									while ( (Adata.end() - dataptr) < static_cast<ptrdiff_t>(recordLength+sizeof(int32_t)) )
									{
										uint64_t const dataptroffset = dataptr - Adata.begin();
										uint64_t const newsize = std::max(static_cast<uint64_t>(1),static_cast<uint64_t>(2*Adata.size()));
										Adata.resize(newsize);
										dataptr = Adata.begin() + dataptroffset;
										// std::cerr << "resize data array to " << Adata.size() << std::endl;
									}
									while ( (Aoffsets.end()-offsetptr) < 1 )
									{
										uint64_t const offsetptroffset = offsetptr - Aoffsets.begin();
										uint64_t const newsize = std::max(static_cast<uint64_t>(1),static_cast<uint64_t>(2*Aoffsets.size()));
										Aoffsets.resize(newsize);
										offsetptr = Aoffsets.begin() + offsetptroffset;
										// std::cerr << "resize offsets array to " << Aoffsets.size() << std::endl;
									}
									while ( (Alength.end()-lengthptr) < 1 )
									{
										uint64_t const lengthptroffset = lengthptr - Alength.begin();
										uint64_t const newsize = std::max(static_cast<uint64_t>(1),static_cast<uint64_t>(2*Alength.size()));
										Alength.resize(newsize);
										lengthptr = Alength.begin() + lengthptroffset;
										// std::cerr << "resize length array to " << Alength.size() << std::endl;
									}

									assert ( offsetptr < Aoffsets.end() );
									assert ( lengthptr < Alength.end() );
									assert ( dataptr+recordLength+sizeof(int32_t) <= Adata.end() );

									*(offsetptr++) = dataptr - Adata.begin();
									*(lengthptr++) = recordLength + sizeof(int32_t);

									std::copy(Atmp.begin(),Atmp.begin() + recordLength + sizeof(recordLengthArray), dataptr);
									dataptr += recordLength + sizeof(recordLengthArray);

									#if 0
									uint8_t const * recdataptr = dataptr - (recordLength+sizeof(int32_t));
									uint8_t const * recdataptrend = dataptr;
									std::string const recdata(recdataptr,recdataptrend);
									std::stringstream istr(recdata);
									libmaus2::dazzler::align::Overlap OVL;
									uint64_t siz;
									libmaus2::dazzler::align::AlignmentFile::readOverlap(istr,OVL,siz,small);
									std::cerr << OVL << std::endl;
									#endif

									// std::cerr << "got record" << std::endl;
									recordLengthRead = 0;
									state = overlap_parser_reading_record_length;
								}
								break;
						}
					}

					overlapsInBuffer = offsetptr - Aoffsets.begin();

					return overlapsInBuffer;
				}
			};
		}
	}
}
#endif
