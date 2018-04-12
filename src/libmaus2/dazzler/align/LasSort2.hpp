/*
    libmaus2
    Copyright (C) 2018 German Tischler-Höhle

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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_LASSORT2_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_LASSORT2_HPP

#include <libmaus2/dazzler/align/SimpleOverlapParser.hpp>
#include <libmaus2/dazzler/align/OverlapDataInterface.hpp>
#include <libmaus2/aio/InputOutputStreamFactoryContainer.hpp>
#include <libmaus2/util/FiniteSizeHeap.hpp>
#include <libmaus2/aio/FileRemoval.hpp>
#include <libmaus2/aio/OutputStreamFactoryContainer.hpp>
#include <libmaus2/dazzler/align/OverlapIndexer.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			template<typename comparator_type = libmaus2::dazzler::align::OverlapDataInterfaceFullComparator>
			struct LasSort2
			{
				static std::string getNextTmpFile(std::string const & tmpbase, uint64_t const i)
				{
					std::ostringstream ostr;
					ostr << tmpbase << "_" << i;
					return ostr.str();
				}

				struct BlockInfo
				{
					uint64_t ostart;
					uint64_t oend;
					uint64_t n;

					BlockInfo() {}
					BlockInfo(
						uint64_t const rostart,
						uint64_t const roend,
						uint64_t const rn
					) : ostart(rostart), oend(roend), n(rn) {}
				};


				static int lassort2(
					std::string const & outlas,
					std::vector<std::string> const & Vin,
					uint64_t const blocksize,
					uint64_t const fanin,
					std::string const & tmpbase,
					bool const index,
					std::ostream * err
				)
				{
					int64_t const tspace = Vin.size() ? libmaus2::dazzler::align::AlignmentFile::getTSpace(Vin) : libmaus2::dazzler::align::AlignmentFile::getMinimumNonSmallTspace();

					uint64_t tmpid = 0;
					std::string tmpfn;
					uint64_t numaln = 0;

					std::vector < BlockInfo > V;

					{
						tmpfn = getNextTmpFile(tmpbase,tmpid++);
						libmaus2::aio::OutputStreamInstance::unique_ptr_type OSI(
							new libmaus2::aio::OutputStreamInstance(tmpfn)
						);

						uint64_t offset = libmaus2::dazzler::align::AlignmentFile::serialiseHeader(*OSI,0,tspace);

						for ( uint64_t z = 0; z < Vin.size(); ++z )
						{
							if ( err )
								(*err) << "[V] processing " << Vin[z] << std::endl;

							libmaus2::dazzler::align::SimpleOverlapParser SOP(Vin[z],blocksize);

							while ( SOP.parseNextBlock() )
							{
								libmaus2::dazzler::align::OverlapData & data = SOP.getData();
								uint64_t const lnumaln = data.size();

								if ( err )
									(*err) << "[V]\tblock of size " << lnumaln << std::endl;

								libmaus2::autoarray::AutoArray<libmaus2::dazzler::align::OverlapData::OverlapOffset> & Aoffsets = data.Aoffsets;
								comparator_type comp(data.Adata.begin());

								if ( err )
									(*err) << "[V]\tsorting block...";
								std::sort(Aoffsets.begin(),Aoffsets.begin() + lnumaln,comp);
								if ( err )
									(*err) << "done." << std::endl;

								uint64_t const blockstart = offset;

								for ( uint64_t i = 0; i < lnumaln; ++i )
								{
									uint8_t const * p = data.Adata.begin() + Aoffsets[i].offset;
									char const * c = reinterpret_cast<char const *>(p);
									OSI->write(c,Aoffsets[i].length);
									offset += Aoffsets[i].length;
								}

								uint64_t const blockend = offset;

								numaln += lnumaln;

								V.push_back(BlockInfo(blockstart,blockend,lnumaln));
							}
						}

						OSI->flush();
						OSI.reset();

						{
							libmaus2::aio::InputOutputStream::unique_ptr_type Optr(
								libmaus2::aio::InputOutputStreamFactoryContainer::constructUnique(tmpfn,std::ios::in|std::ios::out|std::ios::binary)
							);
							libmaus2::dazzler::align::AlignmentFile::serialiseHeader(*Optr,numaln,tspace);
						}
					}

					while ( V.size() > 1 )
					{
						if ( err )
							(*err) << "[V] number of blocks is " << V.size() << std::endl;

						std::string const nexttmpfn = getNextTmpFile(tmpbase,tmpid++);

						libmaus2::aio::OutputStreamInstance::unique_ptr_type OSI(
							new libmaus2::aio::OutputStreamInstance(nexttmpfn)
						);

						uint64_t offset = libmaus2::dazzler::align::AlignmentFile::serialiseHeader(*OSI,numaln,tspace);
						std::vector < BlockInfo > VN;

						uint64_t const numout = (V.size() + fanin - 1)/fanin;

						for ( uint64_t y = 0; y < numout; ++y )
						{
							uint64_t const ilow = y * fanin;
							uint64_t const ihigh = std::min(ilow+fanin,V.size());
							uint64_t const isize = ihigh-ilow;

							if ( err )
								(*err) << "[V] merging [" << ilow << "," << ihigh << ")" << std::endl;

							struct HeapNode
							{
								std::pair<uint8_t const *, uint8_t const *> P;
								uint64_t id;

								HeapNode()
								{}

								HeapNode(std::pair<uint8_t const *, uint8_t const *> const rP, uint64_t const rid)
								: P(rP), id(rid) {}

								bool operator<(HeapNode const & H) const
								{
									if (
										comparator_type::compare(P.first,H.P.first)
									)
									{
										return true;
									}
									else if (
										comparator_type::compare(H.P.first,P.first)
									)
									{
										return false;
									}
									else
									{
										return id < H.id;
									}
								}
							};

							libmaus2::autoarray::AutoArray< libmaus2::aio::InputStreamInstance::unique_ptr_type > Ain(isize);
							libmaus2::autoarray::AutoArray< libmaus2::dazzler::align::SimpleOverlapParser::unique_ptr_type > Apar(isize);
							libmaus2::autoarray::AutoArray< libmaus2::dazzler::align::SimpleOverlapParserGet::unique_ptr_type > AG(isize);
							libmaus2::util::FiniteSizeHeap<HeapNode> FSH(isize);
							uint64_t cnumaln = 0;
							for ( uint64_t i = ilow; i < ihigh; ++i )
							{
								libmaus2::aio::InputStreamInstance::unique_ptr_type tptr(new libmaus2::aio::InputStreamInstance(tmpfn));
								tptr->clear();
								tptr->seekg(V[i].ostart);
								Ain[i-ilow] = UNIQUE_PTR_MOVE(tptr);

								libmaus2::dazzler::align::SimpleOverlapParser::unique_ptr_type sptr(
									new libmaus2::dazzler::align::SimpleOverlapParser(*(Ain[i-ilow]),tspace,32*1024 /* buf size */,
										libmaus2::dazzler::align::OverlapParser::overlapparser_do_split,
										V[i].oend-V[i].ostart
									)
								);
								Apar[i-ilow] = UNIQUE_PTR_MOVE(sptr);

								libmaus2::dazzler::align::SimpleOverlapParserGet::unique_ptr_type G(
									new libmaus2::dazzler::align::SimpleOverlapParserGet(*(Apar[i-ilow]))
								);
								AG[i-ilow] = UNIQUE_PTR_MOVE(G);

								std::pair<uint8_t const *, uint8_t const *> P;
								bool const ok = AG[i-ilow]->getNext(P);
								if ( ok )
								{
									FSH.push(HeapNode(P,i-ilow));
								}

								cnumaln += V[i].n;
							}

							uint64_t const blockstart = offset;

							uint64_t lnumaln = 0;
							while ( !FSH.empty() )
							{
								HeapNode H = FSH.pop();

								lnumaln++;
								OSI->write(reinterpret_cast<char const *>(H.P.first),H.P.second-H.P.first);
								offset += H.P.second-H.P.first;

								std::pair<uint8_t const *, uint8_t const *> P;
								bool const ok = AG[H.id]->getNext(P);
								if ( ok )
									FSH.push(HeapNode(P,H.id));
							}

							uint64_t const blockend = offset;

							if ( lnumaln != cnumaln )
								if ( err )
									(*err) << "[E] copied " << lnumaln << " expected " << cnumaln << std::endl;

							VN.push_back(BlockInfo(blockstart,blockend,lnumaln));
						}

						OSI->flush();
						OSI.reset();

						libmaus2::aio::FileRemoval::removeFile(tmpfn);
						tmpfn = nexttmpfn;

						V = VN;
					}

					libmaus2::aio::OutputStreamFactoryContainer::rename(tmpfn,outlas);

					if ( index )
						libmaus2::dazzler::align::OverlapIndexer::constructIndex(outlas);

					return EXIT_SUCCESS;
				}
			};
		}
	}
}
#endif
