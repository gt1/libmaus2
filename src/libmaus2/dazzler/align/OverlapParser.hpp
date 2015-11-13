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

#include <libmaus2/dazzler/align/OverlapData.hpp>
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
				typedef OverlapParser this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				enum overlap_parser_state {
					overlap_parser_reading_record_length, overlap_parser_reading_record
				};

				int64_t const tspace;
				bool const small;

				struct State
				{
					overlap_parser_state state;
					unsigned int recordLengthRead;
					uint8_t recordLengthArray[sizeof(int32_t)];
					uint64_t recordLength;
					uint64_t recordRead;
					uint8_t * tmpptr;

					void reset()
					{
						state = overlap_parser_reading_record_length;
						recordLengthRead = 0;
						recordLength = 0;
						recordRead = 0;
						tmpptr = 0;
					}
				};

				State state;
				OverlapData odata;

				libmaus2::autoarray::AutoArray<uint8_t> Atmp;
				uint8_t * tmpptr;

				libmaus2::autoarray::AutoArray<uint8_t> Apushback;
				uint64_t pushbackfill;

				OverlapParser(int64_t const rtspace)
				: tspace(rtspace),
				  small(AlignmentFile::tspaceToSmall(tspace)),
				  odata(),
				  tmpptr(0),
				  pushbackfill(0)
				{
				  state.state = overlap_parser_reading_record_length;
				  state.recordLengthRead = 0;
				  state.recordLength = 0;
				}

				OverlapData & getData()
				{
					return odata;
				}

				bool isIdle() const
				{
					return
						state.state == overlap_parser_reading_record_length &&
						state.recordLengthRead == 0 &&
						pushbackfill == 0;
				}

				enum split_type {
					overlapparser_do_split,
					overlapparser_do_not_split_ab,
					overlapparser_do_not_split_b
				};

				uint64_t parseBlock(uint8_t const * data, uint8_t const * data_e, split_type const splittype)
				{
					uint8_t * dataptr = odata.Adata.begin();
					uint64_t * offsetptr = odata.Aoffsets.begin();
					uint64_t * lengthptr = odata.Alength.begin();

					if ( pushbackfill )
					{
						State statesave = state;
						state.reset();
						parseBlockInternal(Apushback.begin(),Apushback.begin()+pushbackfill,dataptr,offsetptr,lengthptr);
						pushbackfill = 0;
						state = statesave;
					}
					parseBlockInternal(data,data_e,dataptr,offsetptr,lengthptr);

					if ( odata.overlapsInBuffer )
					{
						switch ( splittype )
						{
							case overlapparser_do_not_split_ab:
							{
								int64_t lastindex = static_cast<int64_t>(odata.overlapsInBuffer)-1;
								int32_t const last_a_read = OverlapData::getARead(odata.Adata.begin()+odata.Aoffsets[lastindex]);
								int32_t const last_b_read = OverlapData::getBRead(odata.Adata.begin()+odata.Aoffsets[lastindex]);

								int64_t previndex = lastindex-1;

								while (
									previndex >= 0 &&
									OverlapData::getARead(odata.Adata.begin()+odata.Aoffsets[previndex]) == last_a_read &&
									OverlapData::getBRead(odata.Adata.begin()+odata.Aoffsets[previndex]) == last_b_read
								)
									--previndex;

								assert ( previndex < 0 ||
									OverlapData::getARead(odata.Adata.begin()+odata.Aoffsets[previndex]) != last_a_read ||
									OverlapData::getBRead(odata.Adata.begin()+odata.Aoffsets[previndex]) != last_b_read );
								assert ( previndex >= 0 || previndex == -1 );

								for ( uint64_t i = static_cast<uint64_t>(previndex+1); i < odata.overlapsInBuffer; ++i )
									assert (
										OverlapData::getARead(odata.Adata.begin()+odata.Aoffsets[i]) == last_a_read &&
										OverlapData::getBRead(odata.Adata.begin()+odata.Aoffsets[i]) == last_b_read
									);

								uint64_t copylen = 0;
								uint64_t pullback = 0;
								for ( uint64_t i = static_cast<uint64_t>(previndex+1); i < odata.overlapsInBuffer; ++i )
								{
									copylen += odata.Alength[i];
									pullback += 1;
								}
								uint8_t const * copystartptr = odata.Adata.begin()+odata.Aoffsets[previndex+1];
								if ( Apushback.size() < copylen )
									Apushback.resize(copylen);
								std::copy(copystartptr,copystartptr+copylen,Apushback.begin());
								pushbackfill = copylen;

								odata.overlapsInBuffer -= pullback;

								for ( uint64_t i = 0; i < odata.overlapsInBuffer; ++i )
									assert (
										OverlapData::getARead(odata.Adata.begin()+odata.Aoffsets[i]) != last_a_read ||
										OverlapData::getBRead(odata.Adata.begin()+odata.Aoffsets[i]) != last_b_read
									);
								assert (
									OverlapData::getARead(odata.Adata.begin()+odata.Aoffsets[odata.overlapsInBuffer]) == last_a_read &&
									OverlapData::getBRead(odata.Adata.begin()+odata.Aoffsets[odata.overlapsInBuffer]) == last_b_read
								);
								break;
							}
							case overlapparser_do_not_split_b:
							{
								int64_t lastindex = static_cast<int64_t>(odata.overlapsInBuffer)-1;
								int32_t const last_b_read = OverlapData::getBRead(odata.Adata.begin()+odata.Aoffsets[lastindex]);

								int64_t previndex = lastindex-1;

								while ( previndex >= 0 && OverlapData::getBRead(odata.Adata.begin()+odata.Aoffsets[previndex]) == last_b_read )
									--previndex;

								assert ( previndex < 0 || OverlapData::getBRead(odata.Adata.begin()+odata.Aoffsets[previndex]) != last_b_read );
								assert ( previndex >= 0 || previndex == -1 );

								for ( uint64_t i = static_cast<uint64_t>(previndex+1); i < odata.overlapsInBuffer; ++i )
									assert ( OverlapData::getBRead(odata.Adata.begin()+odata.Aoffsets[i]) == last_b_read );

								uint64_t copylen = 0;
								uint64_t pullback = 0;
								for ( uint64_t i = static_cast<uint64_t>(previndex+1); i < odata.overlapsInBuffer; ++i )
								{
									copylen += odata.Alength[i];
									pullback += 1;
								}
								uint8_t const * copystartptr = odata.Adata.begin()+odata.Aoffsets[previndex+1];
								if ( Apushback.size() < copylen )
									Apushback.resize(copylen);
								std::copy(copystartptr,copystartptr+copylen,Apushback.begin());
								pushbackfill = copylen;

								odata.overlapsInBuffer -= pullback;

								for ( uint64_t i = 0; i < odata.overlapsInBuffer; ++i )
									assert (
										OverlapData::getBRead(odata.Adata.begin()+odata.Aoffsets[i]) != last_b_read
									);
								assert ( OverlapData::getBRead(odata.Adata.begin()+odata.Aoffsets[odata.overlapsInBuffer]) == last_b_read );
								break;
							}
							default:
								break;
						}
					}

					return odata.overlapsInBuffer;
				}

				void parseBlockInternal(
					uint8_t const * data,
					uint8_t const * data_e,
					uint8_t * & dataptr,
					uint64_t * & offsetptr,
					uint64_t * & lengthptr
				)
				{
					while ( (data != data_e) )
					{
						switch ( state.state )
						{
							case overlap_parser_reading_record_length:
								assert ( data != data_e );
								if (
									(!state.recordLengthRead)
									&&
									data_e-data >= static_cast<ptrdiff_t>(sizeof(state.recordLengthArray))
									&&
									data_e-data >= static_cast<ptrdiff_t>(6 * sizeof(int32_t) + 4 * sizeof(int32_t) + (small ? 1 : 2) * libmaus2::util::loadValueLE4(data))
								)
								{
									uint64_t reclen = 0;

									while (
										data_e-data >= static_cast<ptrdiff_t>(sizeof(state.recordLengthArray))
										&&
										data_e-data >= static_cast<ptrdiff_t>((reclen=6 * sizeof(int32_t) + 4 * sizeof(int32_t) + (small ? 1 : 2) * libmaus2::util::loadValueLE4(data)))
									)
									{
										while ( (odata.Adata.end() - dataptr) < static_cast<ptrdiff_t>(reclen) )
										{
											uint64_t const dataptroffset = dataptr - odata.Adata.begin();
											uint64_t const newsize = std::max(static_cast<uint64_t>(1),static_cast<uint64_t>(2*odata.Adata.size()));
											odata.Adata.resize(newsize);
											dataptr = odata.Adata.begin() + dataptroffset;
											// std::cerr << "resize data array to " << odata.Adata.size() << std::endl;
										}
										while ( (odata.Aoffsets.end()-offsetptr) < 1 )
										{
											uint64_t const offsetptroffset = offsetptr - odata.Aoffsets.begin();
											uint64_t const newsize = std::max(static_cast<uint64_t>(1),static_cast<uint64_t>(2*odata.Aoffsets.size()));
											odata.Aoffsets.resize(newsize);
											offsetptr = odata.Aoffsets.begin() + offsetptroffset;
											// std::cerr << "resize offsets array to " << odata.Aoffsets.size() << std::endl;
										}
										while ( (odata.Alength.end()-lengthptr) < 1 )
										{
											uint64_t const lengthptroffset = lengthptr - odata.Alength.begin();
											uint64_t const newsize = std::max(static_cast<uint64_t>(1),static_cast<uint64_t>(2*odata.Alength.size()));
											odata.Alength.resize(newsize);
											lengthptr = odata.Alength.begin() + lengthptroffset;
											// std::cerr << "resize length array to " << odata.Alength.size() << std::endl;
										}

										std::copy(data,data+reclen,dataptr);
										*(offsetptr++) = dataptr - odata.Adata.begin();
										*(lengthptr++) = reclen;

										dataptr += reclen;
										data += reclen;
									}
								}
								else
								{
									while ( (data != data_e) && (state.recordLengthRead < sizeof(state.recordLengthArray)) )
										state.recordLengthArray[state.recordLengthRead++] = *(data++);
									if ( state.recordLengthRead == sizeof(state.recordLengthArray) )
									{
										state.state = overlap_parser_reading_record;
										state.recordLength =
											5 * sizeof(int32_t) + // path meta - tlen
											4 * sizeof(int32_t) + // overlap meta
											(small ? 1 : 2) * libmaus2::util::loadValueLE4(&state.recordLengthArray[0]) // path
											;
										state.recordRead = 0;

										while ( Atmp.size() < state.recordLength+sizeof(int32_t) )
										{
											uint64_t const size = Atmp.size();
											uint64_t const one = 1;
											uint64_t const newsize = std::max(one,size<<1);
											Atmp.resize(newsize);
										}
										tmpptr = Atmp.begin();

										// std::cerr << "got record length " << recordLength << std::endl;

										std::copy(&state.recordLengthArray[0],&state.recordLengthArray[sizeof(state.recordLengthArray)],tmpptr);
										tmpptr += sizeof(int32_t);
									}
								}
								break;
							case overlap_parser_reading_record:
								while ( (data != data_e) && (state.recordRead < state.recordLength) )
								{
									*(tmpptr++) = *(data++);
									state.recordRead++;
								}
								if ( state.recordRead == state.recordLength )
								{
									while ( (odata.Adata.end() - dataptr) < static_cast<ptrdiff_t>(state.recordLength+sizeof(int32_t)) )
									{
										uint64_t const dataptroffset = dataptr - odata.Adata.begin();
										uint64_t const newsize = std::max(static_cast<uint64_t>(1),static_cast<uint64_t>(2*odata.Adata.size()));
										odata.Adata.resize(newsize);
										dataptr = odata.Adata.begin() + dataptroffset;
										// std::cerr << "resize data array to " << odata.Adata.size() << std::endl;
									}
									while ( (odata.Aoffsets.end()-offsetptr) < 1 )
									{
										uint64_t const offsetptroffset = offsetptr - odata.Aoffsets.begin();
										uint64_t const newsize = std::max(static_cast<uint64_t>(1),static_cast<uint64_t>(2*odata.Aoffsets.size()));
										odata.Aoffsets.resize(newsize);
										offsetptr = odata.Aoffsets.begin() + offsetptroffset;
										// std::cerr << "resize offsets array to " << odata.Aoffsets.size() << std::endl;
									}
									while ( (odata.Alength.end()-lengthptr) < 1 )
									{
										uint64_t const lengthptroffset = lengthptr - odata.Alength.begin();
										uint64_t const newsize = std::max(static_cast<uint64_t>(1),static_cast<uint64_t>(2*odata.Alength.size()));
										odata.Alength.resize(newsize);
										lengthptr = odata.Alength.begin() + lengthptroffset;
										// std::cerr << "resize length array to " << odata.Alength.size() << std::endl;
									}

									assert ( offsetptr < odata.Aoffsets.end() );
									assert ( lengthptr < odata.Alength.end() );
									assert ( dataptr+state.recordLength+sizeof(int32_t) <= odata.Adata.end() );

									*(offsetptr++) = dataptr - odata.Adata.begin();
									*(lengthptr++) = state.recordLength + sizeof(int32_t);

									std::copy(Atmp.begin(),Atmp.begin() + state.recordLength + sizeof(state.recordLengthArray), dataptr);
									dataptr += state.recordLength + sizeof(state.recordLengthArray);

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
									state.recordLengthRead = 0;
									state.state = overlap_parser_reading_record_length;
								}
								break;
						}
					}

					odata.overlapsInBuffer = offsetptr - odata.Aoffsets.begin();
				}
			};
		}
	}
}
#endif
