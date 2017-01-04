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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_OVERLAPINDEXER_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_OVERLAPINDEXER_HPP

#include <libmaus2/dazzler/align/AlignmentFileDecoder.hpp>
#include <libmaus2/dazzler/align/AlignmentFileRegion.hpp>
#include <libmaus2/util/CountPutObject.hpp>
#include <libmaus2/index/ExternalMemoryIndexGenerator.hpp>
#include <libmaus2/dazzler/align/OverlapMeta.hpp>
#include <libmaus2/dazzler/align/OverlapIndexerBase.hpp>
#include <libmaus2/dazzler/align/AlignmentFile.hpp>
#include <libmaus2/aio/InputStreamFactoryContainer.hpp>
#include <libmaus2/index/ExternalMemoryIndexDecoder.hpp>
#include <libmaus2/aio/InputStreamInstance.hpp>
#include <libmaus2/aio/OutputStreamFactoryContainer.hpp>
#include <libmaus2/util/TempFileRemovalContainer.hpp>
#include <libmaus2/util/ArgInfo.hpp>
#include <libmaus2/util/GetFileSize.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct OverlapIndexer : public OverlapIndexerBase
			{
				static std::pair<bool,uint64_t> getOverlapAndOffset(
					libmaus2::dazzler::align::AlignmentFile & algn,
					std::istream & algnfile,
					libmaus2::dazzler::align::Overlap & OVL
				)
				{
					std::streampos pos = algnfile.tellg();

					if ( algn.getNextOverlap(algnfile,OVL) )
					{
						if ( pos < 0 )
						{
							libmaus2::exception::LibMausException lme;
							lme.getStream() << "getOverlapAndOffset: tellg call failed" << std::endl;
							lme.finish();
							throw lme;
						}
						return std::pair<bool,uint64_t>(true,pos);
					}
					else
					{
						return std::pair<bool,uint64_t>(false,pos);
					}
				}

				static std::string getIndexName(std::string const & aligns)
				{
					std::string::size_type const p = aligns.find_last_of('/');

					if ( p == std::string::npos )
						return std::string(".") + aligns + std::string(".bidx");
					else
					{
						std::string const prefix = aligns.substr(0,p);
						std::string const suffix = aligns.substr(p+1);
						return prefix + "/." + suffix + ".bidx";
					}
				}



				static bool haveIndex(std::string const & aligns)
				{
					return libmaus2::aio::InputStreamFactoryContainer::tryOpen(getIndexName(aligns));
				}

				static uint64_t getReadStartPosition(std::string const & aligns, int64_t const aread, std::string const & indexname)
				{
					libmaus2::aio::InputStreamInstance::unique_ptr_type Pfile(new libmaus2::aio::InputStreamInstance(aligns));

					libmaus2::index::ExternalMemoryIndexDecoder<OverlapMeta,base_level_log,inner_level_log> EMID(indexname);
					libmaus2::index::ExternalMemoryIndexDecoderFindLargestSmallerResult<OverlapMeta> ER = EMID.findLargestSmaller(OverlapMeta(aread,0,0,0,0,0,0));

					libmaus2::dazzler::align::AlignmentFile::unique_ptr_type Palgn(new libmaus2::dazzler::align::AlignmentFile(*Pfile));

					Palgn->alre += (ER.blockid << base_level_log);
					Pfile->clear();
					Pfile->seekg(ER.P.first);

					libmaus2::dazzler::align::Overlap OVL;

					uint64_t ppos = Pfile->tellg();

					while ( Palgn->peekNextOverlap(*Pfile,OVL) && OVL.aread < aread )
					{
						Palgn->getNextOverlap(*Pfile,OVL);
						ppos = Pfile->tellg();
					}

					if ( Palgn->putbackslotactive )
					{
						Palgn->putbackslotactive = false;
						assert ( Palgn->alre > 0 );
						Palgn->alre -= 1;
					}

					Pfile->clear();
					Pfile->seekg(ppos);

					return ppos;
				}

				static uint64_t getReadStartPosition(std::string const & aligns, int64_t const aread)
				{
					if ( ! haveIndex(aligns) )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "OverlapIndexer::getReadStartPosition: index file " << getIndexName(aligns) << " not available" << std::endl;
						lme.finish();
						throw lme;
					}

					return getReadStartPosition(aligns,aread,getIndexName(aligns));
				}

				static void openAlignmentFileAtRead(
					std::string const & aligns,
					int64_t const aread,
					libmaus2::aio::InputStreamInstance::unique_ptr_type & Pfile,
					libmaus2::dazzler::align::AlignmentFile::unique_ptr_type & Palgn,
					std::string const & indexname
				)
				{
					if ( aread == 0 )
					{
						libmaus2::aio::InputStreamInstance::unique_ptr_type Tfile(new libmaus2::aio::InputStreamInstance(aligns));
						Pfile = UNIQUE_PTR_MOVE(Tfile);
						libmaus2::dazzler::align::AlignmentFile::unique_ptr_type Talgn(new libmaus2::dazzler::align::AlignmentFile(*Pfile));
						Palgn = UNIQUE_PTR_MOVE(Talgn);
					}
					else
					{

						libmaus2::index::ExternalMemoryIndexDecoder<OverlapMeta,base_level_log,inner_level_log> EMID(indexname);

						if ( ! EMID.size() )
						{
							libmaus2::aio::InputStreamInstance::unique_ptr_type Tfile(new libmaus2::aio::InputStreamInstance(aligns));
							Pfile = UNIQUE_PTR_MOVE(Tfile);
							libmaus2::dazzler::align::AlignmentFile::unique_ptr_type Talgn(new libmaus2::dazzler::align::AlignmentFile(*Pfile));
							Palgn = UNIQUE_PTR_MOVE(Talgn);
						}
						else
						{
							libmaus2::aio::InputStreamInstance::unique_ptr_type Tfile(new libmaus2::aio::InputStreamInstance(aligns));
							Pfile = UNIQUE_PTR_MOVE(Tfile);

							libmaus2::index::ExternalMemoryIndexDecoderFindLargestSmallerResult<OverlapMeta> ER = EMID.findLargestSmaller(OverlapMeta(aread,0,0,0,0,0,0));

							libmaus2::dazzler::align::AlignmentFile::unique_ptr_type Talgn(new libmaus2::dazzler::align::AlignmentFile(*Pfile));
							Palgn = UNIQUE_PTR_MOVE(Talgn);

							Palgn->alre += (ER.blockid << base_level_log);
							Pfile->clear();
							Pfile->seekg(ER.P.first);

							libmaus2::dazzler::align::Overlap OVL;

							uint64_t ppos = Pfile->tellg();

							while ( Palgn->peekNextOverlap(*Pfile,OVL) && OVL.aread < aread )
							{
								Palgn->getNextOverlap(*Pfile,OVL);
								ppos = Pfile->tellg();
							}

							if ( Palgn->putbackslotactive )
							{
								Palgn->putbackslotactive = false;
								assert ( Palgn->alre > 0 );
								Palgn->alre -= 1;
							}

							Pfile->clear();
							Pfile->seekg(ppos);
						}
					}
				}

				static void openAlignmentFileAtRead(
					std::string const & aligns,
					int64_t const aread,
					libmaus2::aio::InputStreamInstance::unique_ptr_type & Pfile,
					libmaus2::dazzler::align::AlignmentFile::unique_ptr_type & Palgn
				)
				{
					if ( ! haveIndex(aligns) )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "OverlapIndexer::openAlignmentFileAtRead: index file " << getIndexName(aligns) << " not available" << std::endl;
						lme.finish();
						throw lme;
					}

					openAlignmentFileAtRead(aligns,aread,Pfile,Palgn,getIndexName(aligns));
				}

				static int64_t getMaximumARead(std::string const & aligns, std::string const & indexname)
				{
					libmaus2::aio::InputStreamInstance::unique_ptr_type Pfile(new libmaus2::aio::InputStreamInstance(aligns));

					libmaus2::index::ExternalMemoryIndexDecoder<OverlapMeta,base_level_log,inner_level_log> EMID(indexname);

					// empty file?
					if ( !EMID.size() )
						return -1;

					std::pair<uint64_t,uint64_t> const P = EMID[EMID.size()-1];

					libmaus2::dazzler::align::AlignmentFile::unique_ptr_type Palgn(new libmaus2::dazzler::align::AlignmentFile(*Pfile));
					Palgn->novl -= ((EMID.size()-1) << base_level_log);
					Pfile->clear();
					Pfile->seekg(P.first);

					libmaus2::dazzler::align::Overlap OVL;
					int64_t aread = -1;
					while ( Palgn->getNextOverlap(*Pfile,OVL) )
						aread = OVL.aread;

					return aread;
				}

				static int64_t getMaximumARead(std::string const & aligns)
				{
					if ( ! haveIndex(aligns) )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "OverlapIndexer::getMaximumARead: index file " << getIndexName(aligns) << " not available" << std::endl;
						lme.finish();
						throw lme;
					}

					return getMaximumARead(aligns,getIndexName(aligns));
				}

				static int64_t getMinimumARead(std::string const & aligns)
				{
					libmaus2::aio::InputStreamInstance::unique_ptr_type Pfile(new libmaus2::aio::InputStreamInstance(aligns));
					libmaus2::dazzler::align::AlignmentFile::unique_ptr_type Palgn(new libmaus2::dazzler::align::AlignmentFile(*Pfile));
					libmaus2::dazzler::align::Overlap OVL;
					int64_t aread = -1;
					if ( Palgn->getNextOverlap(*Pfile,OVL) )
						aread = OVL.aread;

					return aread;
				}

				struct OpenAlignmentFileRegionInfo
				{
					std::streampos gposbelow;
					std::streampos gposabove;
					uint64_t algnlow;
					uint64_t algnhigh;

					OpenAlignmentFileRegionInfo() {}
					OpenAlignmentFileRegionInfo(std::streampos const rgposbelow, std::streampos const rgposabove, uint64_t const ralgnlow, uint64_t const ralgnhigh)
					: gposbelow(rgposbelow), gposabove(rgposabove), algnlow(ralgnlow), algnhigh(ralgnhigh)
					{
					}
				};

				static OpenAlignmentFileRegionInfo openAlignmentFileRegion(
					std::string const & aligns,
					int64_t afrom, // lower bound, included
					int64_t ato, // upper bound, not included
					libmaus2::aio::InputStreamInstance::unique_ptr_type & Pfile,
					libmaus2::dazzler::align::AlignmentFile::unique_ptr_type & Palgn,
					std::string const & indexname
				)
				{
					if ( ato < afrom )
						ato = afrom;

					openAlignmentFileAtRead(aligns,ato,Pfile,Palgn,indexname);

					std::streampos const gposabove = Pfile->tellg();

					if ( gposabove < 0 )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "OverlapIndexer::openAlignmentFileRegion: tellg() failed" << std::endl;
						lme.finish();
						throw lme;
					}

					int64_t const numabove = Palgn->novl-Palgn->alre;
					int64_t const abovestart = Palgn->alre;

					openAlignmentFileAtRead(aligns,afrom,Pfile,Palgn,indexname);

					int64_t const regionstart = Palgn->alre;

					std::streampos const gposbelow = Pfile->tellg();

					if ( gposbelow < 0 )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "OverlapIndexer::openAlignmentFileRegion: tellg() failed" << std::endl;
						lme.finish();
						throw lme;
					}

					if ( Palgn->novl >= numabove )
						Palgn->novl -= numabove;
					else
						Palgn->novl = 0;

					return OpenAlignmentFileRegionInfo(gposbelow,gposabove,regionstart,abovestart);
				}

				static OpenAlignmentFileRegionInfo openAlignmentFileRegion(
					std::string const & aligns,
					int64_t afrom, // lower bound, included
					int64_t ato, // upper bound, not included
					libmaus2::aio::InputStreamInstance::unique_ptr_type & Pfile,
					libmaus2::dazzler::align::AlignmentFile::unique_ptr_type & Palgn
				)
				{
					if ( ! haveIndex(aligns) )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "OverlapIndexer::openAlignmentFileRegion: index file " << getIndexName(aligns) << " not available" << std::endl;
						lme.finish();
						throw lme;
					}

					return openAlignmentFileRegion(aligns,afrom,ato,Pfile,Palgn,getIndexName(aligns));
				}

				static AlignmentFileRegion::unique_ptr_type openAlignmentFileRegion(
					std::string const & aligns,
					int64_t afrom, // lower bound, included
					int64_t ato, // upper bound, not included
					OpenAlignmentFileRegionInfo & info,
					std::string const & indexname
				)
				{
					libmaus2::aio::InputStreamInstance::unique_ptr_type Pfile;
					libmaus2::dazzler::align::AlignmentFile::unique_ptr_type Palgn;
					info = openAlignmentFileRegion(aligns,afrom,ato,Pfile,Palgn,indexname);
					AlignmentFileRegion::unique_ptr_type Tptr(new AlignmentFileRegion(Pfile,Palgn));
					return UNIQUE_PTR_MOVE(Tptr);
				}

				static AlignmentFileRegion::unique_ptr_type openAlignmentFileRegion(
					std::string const & aligns,
					int64_t afrom, // lower bound, included
					int64_t ato, // upper bound, not included
					OpenAlignmentFileRegionInfo & info
				)
				{
					if ( ! haveIndex(aligns) )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "OverlapIndexer::openAlignmentFileRegion: index file " << getIndexName(aligns) << " not available" << std::endl;
						lme.finish();
						throw lme;
					}

					AlignmentFileRegion::unique_ptr_type tptr(openAlignmentFileRegion(aligns,afrom,ato,info,getIndexName(aligns)));

					return UNIQUE_PTR_MOVE(tptr);
				}


				static AlignmentFileDecoder::unique_ptr_type openAlignmentFileAt(
					std::string const & aligns,
					int64_t afrom, // lower bound, included
					int64_t ato, // upper bound, not included
					DalignerIndexDecoder & index
				)
				{
					uint64_t const pb = index[afrom];
					uint64_t const pe = index[ato];

					AlignmentFileDecoder::unique_ptr_type tptr(new AlignmentFileDecoder(aligns,index.tspace,pb,pe));
					return UNIQUE_PTR_MOVE(tptr);
				}

				static AlignmentFileRegion::unique_ptr_type openAlignmentFileRegion(
					std::string const & aligns,
					int64_t afrom, // lower bound, included
					int64_t ato, // upper bound, not included
					std::string const & indexname
				)
				{
					OpenAlignmentFileRegionInfo info;
					AlignmentFileRegion::unique_ptr_type Tptr(openAlignmentFileRegion(aligns,afrom,ato,info,indexname));
					return UNIQUE_PTR_MOVE(Tptr);
				}

				static AlignmentFileRegion::unique_ptr_type openAlignmentFileRegion(
					std::string const & aligns,
					int64_t afrom, // lower bound, included
					int64_t ato // upper bound, not included
				)
				{
					if ( ! haveIndex(aligns) )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "OverlapIndexer::openAlignmentFileRegion: index file " << getIndexName(aligns) << " not available" << std::endl;
						lme.finish();
						throw lme;
					}

					AlignmentFileRegion::unique_ptr_type tptr(openAlignmentFileRegion(aligns,afrom,ato,getIndexName(aligns)));
					return UNIQUE_PTR_MOVE(tptr);
				}

				static AlignmentFileRegion::unique_ptr_type openAlignmentFile(std::string const & aligns, std::string const & indexname)
				{
					int64_t const max = getMaximumARead(aligns);
					libmaus2::aio::InputStreamInstance::unique_ptr_type Pfile;
					libmaus2::dazzler::align::AlignmentFile::unique_ptr_type Palgn;
					openAlignmentFileRegion(aligns,0,max+1,Pfile,Palgn,indexname);
					AlignmentFileRegion::unique_ptr_type Tptr(new AlignmentFileRegion(Pfile,Palgn));
					return UNIQUE_PTR_MOVE(Tptr);
				}

				static AlignmentFileRegion::unique_ptr_type openAlignmentFile(std::string const & aligns)
				{
					int64_t const max = getMaximumARead(aligns);
					libmaus2::aio::InputStreamInstance::unique_ptr_type Pfile;
					libmaus2::dazzler::align::AlignmentFile::unique_ptr_type Palgn;
					openAlignmentFileRegion(aligns,0,max+1,Pfile,Palgn);
					AlignmentFileRegion::unique_ptr_type Tptr(new AlignmentFileRegion(Pfile,Palgn));
					return UNIQUE_PTR_MOVE(Tptr);
				}

				static AlignmentFileRegion::unique_ptr_type openAlignmentFileWithoutIndex(std::string const & aligns)
				{
					libmaus2::aio::InputStreamInstance::unique_ptr_type Pfile(new libmaus2::aio::InputStreamInstance(aligns));
					libmaus2::dazzler::align::AlignmentFile::unique_ptr_type Palgn(new libmaus2::dazzler::align::AlignmentFile(*Pfile));
					AlignmentFileRegion::unique_ptr_type Tptr(new AlignmentFileRegion(Pfile,Palgn));
					return UNIQUE_PTR_MOVE(Tptr);
				}

				static std::string constructIndex(std::string const & aligns, std::ostream * verbstr = 0)
				{
					std::string const indexfn = getIndexName(aligns);
					std::string const indexfntmp = indexfn + libmaus2::util::ArgInfo::getDefaultTmpFileName(std::string()) + ".tmp";
					libmaus2::util::TempFileRemovalContainer::addTempFile(indexfntmp);

					std::string const dalignerindexfn = DalignerIndexDecoder::getDalignerIndexName(aligns);
					std::string const dalignerindexfntmp = dalignerindexfn + libmaus2::util::ArgInfo::getDefaultTmpFileName(std::string()) + ".tmp";
					libmaus2::util::TempFileRemovalContainer::addTempFile(dalignerindexfntmp);

					typedef libmaus2::index::ExternalMemoryIndexGenerator<OverlapMeta,base_level_log,inner_level_log> indexer_type;
					indexer_type::unique_ptr_type EMIG(new indexer_type(indexfntmp));
					EMIG->setup();
					libmaus2::aio::OutputStreamInstance::unique_ptr_type DOFS(new libmaus2::aio::OutputStreamInstance(dalignerindexfntmp));

					uint64_t doffset = 0;
					libmaus2::dazzler::db::OutputBase::putLittleEndianInteger8(*DOFS,0,doffset);
					libmaus2::dazzler::db::OutputBase::putLittleEndianInteger8(*DOFS,0,doffset);
					libmaus2::dazzler::db::OutputBase::putLittleEndianInteger8(*DOFS,0,doffset);
					libmaus2::dazzler::db::OutputBase::putLittleEndianInteger8(*DOFS,0,doffset);

					// open alignment file
					libmaus2::aio::InputStreamInstance algnfile(aligns);
					libmaus2::dazzler::align::AlignmentFile algn(algnfile);
					libmaus2::dazzler::align::Overlap OVL;

					// current a-read id
					libmaus2::dazzler::align::Overlap OVLprev;
					bool haveprev = false;
					uint64_t lp = 0;
					int64_t nextid = 0;
					// initialise to point to first record of alignment file
					// (which is the end of the file if file is empty)
					std::pair<bool,uint64_t> P(false,libmaus2::dazzler::align::AlignmentFile::getSerialisedHeaderSize());

					// maximum trace length
					int64_t tmax = 0;
					// maximum out degree (pile depth)
					int64_t omax = 0;
					// total number of trace points
					int64_t ttot = 0;
					// maximum number of trace points for an overlap
					int64_t smax = 0;

					int64_t odeg = 0;
					int64_t sdeg = 0;
					// get next overlap
					while ( (P=OverlapIndexer::getOverlapAndOffset(algn,algnfile,OVL)).first )
					{
						if ( haveprev )
						{
							bool const ok = !(OVL < OVLprev);

							if ( !ok )
							{
								libmaus2::exception::LibMausException lme;
								lme.getStream() << "file " << aligns << " is not sorted" << std::endl
									<< OVLprev << std::endl
									<< OVL << std::endl
									;
								lme.finish();
								throw lme;
							}
						}

						if ( (! haveprev) || (haveprev && (OVLprev.aread != OVL.aread)) )
						{
							if ( ! haveprev )
								nextid = OVL.aread;

							while ( nextid < OVL.aread )
							{
								libmaus2::dazzler::db::OutputBase::putLittleEndianInteger8(*DOFS,P.second,doffset);
								nextid += 1;
							}

							omax = std::max(omax,odeg);
							smax = std::max(smax,sdeg);

							libmaus2::dazzler::db::OutputBase::putLittleEndianInteger8(*DOFS,P.second,doffset);
							nextid += 1;
							odeg = 1; // start new pile count
							sdeg = OVL.path.tlen; // start new trace points per pile
						}
						else
						{
							odeg += 1; // increment overlaps in pile
							sdeg += OVL.path.tlen; // add to trace points per pile
						}

						if ( ((lp++) & EMIG->base_index_mask) == 0 )
						{
							EMIG->put(
								OverlapMeta(OVL),
								std::pair<uint64_t,uint64_t>(P.second,0)
							);
						}

						// update maximum number of trace points in any overlap
						tmax = std::max(static_cast<int64_t>(OVL.path.tlen),tmax);
						// update total number of trace points in LAS file
						ttot += OVL.path.tlen;

						haveprev = true;
						OVLprev = OVL;

						if ( verbstr && ((lp & (1024*1024-1)) == 0) )
						{
							(*verbstr) << "[V] " << static_cast<double>(lp)/algn.novl << std::endl;
						}
					}

					libmaus2::dazzler::db::OutputBase::putLittleEndianInteger8(*DOFS,P.second,doffset);

					omax = std::max(omax,odeg);
					smax = std::max(smax,sdeg);

					DOFS->seekp(0,std::ios::beg);
					libmaus2::dazzler::db::OutputBase::putLittleEndianInteger8(*DOFS,omax,doffset);
					libmaus2::dazzler::db::OutputBase::putLittleEndianInteger8(*DOFS,ttot,doffset);
					libmaus2::dazzler::db::OutputBase::putLittleEndianInteger8(*DOFS,smax,doffset);
					libmaus2::dazzler::db::OutputBase::putLittleEndianInteger8(*DOFS,tmax,doffset);
					DOFS->flush();
					DOFS.reset();

					EMIG->flush();
					EMIG.reset();

					libmaus2::aio::OutputStreamFactoryContainer::rename(indexfntmp,indexfn);
					libmaus2::aio::OutputStreamFactoryContainer::rename(dalignerindexfntmp,dalignerindexfn);

					return indexfn;
				}
			};
		}
	}
}
#endif
