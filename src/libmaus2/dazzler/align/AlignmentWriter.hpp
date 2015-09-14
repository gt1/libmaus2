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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_ALIGNMENTWRITER_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_ALIGNMENTWRITER_HPP

#include <libmaus2/dazzler/align/OverlapIndexer.hpp>
#include <libmaus2/dazzler/align/AlignmentFile.hpp>
#include <libmaus2/aio/OutputStreamInstance.hpp>
#include <libmaus2/aio/InputOutputStreamFactoryContainer.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct AlignmentWriter : public libmaus2::dazzler::align::OverlapIndexerBase
			{
				typedef AlignmentWriter this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
			
				// output file name
				std::string const fn;
				// index file name
				std::string const ifn;
			
				// data output
				libmaus2::aio::OutputStreamInstance::unique_ptr_type PDOSI;
				std::ostream & DOSI;
				
				// index output
				libmaus2::aio::InputOutputStream::unique_ptr_type PIOSI;

				// index generator
				typedef libmaus2::index::ExternalMemoryIndexGenerator<OverlapMeta,base_level_log,inner_level_log> indexer_type;
				indexer_type::unique_ptr_type PEMIG;
				
				int64_t const tspace;
				bool const small;
				
				uint64_t const novlexptd;
				uint64_t novl;
				uint64_t dpos;
				
				static libmaus2::aio::InputOutputStream::unique_ptr_type openIndexStream(std::string const & ifn, bool const createindex)
				{
					libmaus2::aio::InputOutputStream::unique_ptr_type Pptr;
					
					if ( createindex )
					{
						libmaus2::aio::InputOutputStream::unique_ptr_type Tptr(libmaus2::aio::InputOutputStreamFactoryContainer::constructUnique(ifn,std::ios::in|std::ios::out|std::ios::trunc|std::ios::binary));
						Pptr = UNIQUE_PTR_MOVE(Tptr);
					}
					
					return UNIQUE_PTR_MOVE(Pptr);
				}
				
				static indexer_type::unique_ptr_type creatIndexer(std::iostream * IOSI)
				{
					indexer_type::unique_ptr_type Pptr;
					
					if ( IOSI )
					{
						indexer_type::unique_ptr_type Tptr(new indexer_type(*IOSI));
						Pptr = UNIQUE_PTR_MOVE(Tptr);
					}				
					
					return UNIQUE_PTR_MOVE(Pptr);
				}
				
				AlignmentWriter(std::string const & rfn, int64_t const rtspace, bool const createindex = true, uint64_t const rnovlexptd = 0)
				: fn(rfn), 
				  ifn(libmaus2::dazzler::align::OverlapIndexer::getIndexName(fn)), 
				  PDOSI(new libmaus2::aio::OutputStreamInstance(fn)),
				  DOSI(*PDOSI),
				  PIOSI(openIndexStream(ifn,createindex)),
				  PEMIG(creatIndexer(PIOSI.get())),
				  tspace(rtspace),
				  small(libmaus2::dazzler::align::AlignmentFile::tspaceToSmall(tspace)),
				  novlexptd(rnovlexptd),
				  novl(0),
				  dpos(0)
				{
					if ( PEMIG )
						PEMIG->setup();
					dpos += libmaus2::dazzler::align::AlignmentFile::serialiseHeader(DOSI,novlexptd,tspace);					
				}

				AlignmentWriter(std::ostream & rDOSI, int64_t const rtspace, uint64_t const rnovlexptd = 0)
				: fn(), 
				  ifn(),
				  PDOSI(),
				  DOSI(rDOSI),
				  PIOSI(),
				  PEMIG(creatIndexer(PIOSI.get())),
				  tspace(rtspace),
				  small(libmaus2::dazzler::align::AlignmentFile::tspaceToSmall(tspace)),
				  novlexptd(rnovlexptd),
				  novl(0),
				  dpos(0)
				{
					if ( PEMIG )
						PEMIG->setup();
					dpos += libmaus2::dazzler::align::AlignmentFile::serialiseHeader(DOSI,novlexptd,tspace);
				}

				AlignmentWriter(std::ostream & rDOSI, std::iostream & indexstream, int64_t const rtspace, uint64_t const rnovlexptd = 0)
				: fn(), 
				  ifn(),
				  PDOSI(),
				  DOSI(rDOSI),
				  PIOSI(),
				  PEMIG(creatIndexer(&indexstream)),
				  tspace(rtspace),
				  small(libmaus2::dazzler::align::AlignmentFile::tspaceToSmall(tspace)),
				  novlexptd(rnovlexptd),
				  novl(0),
				  dpos(0)
				{
					if ( PEMIG )
						PEMIG->setup();
					dpos += libmaus2::dazzler::align::AlignmentFile::serialiseHeader(DOSI,novlexptd,tspace);
				}
				
				~AlignmentWriter()
				{
					if ( novl != novlexptd )
					{
						DOSI.seekp(0,std::ios::beg);
						libmaus2::dazzler::align::AlignmentFile::serialiseHeader(DOSI,novl,tspace);
						DOSI.seekp(dpos,std::ios::beg);
					}
					// flush alignment stream
					DOSI.flush();
					// reset alignment file handle
					PDOSI.reset();
				
					// finalise index if we are producing any
					if ( PEMIG )
					{
						PEMIG->flush();
						PEMIG.reset();
					}
					
					// flush and close index stream
					if ( PIOSI )
					{
						PIOSI->flush();
						PIOSI.reset();
					}
				}
				
				void put(libmaus2::dazzler::align::Overlap const & OVL)
				{
					if ( (((novl++) & indexer_type::base_index_mask) == 0) && PEMIG )
						PEMIG->put(libmaus2::dazzler::align::OverlapMeta(OVL), std::pair<uint64_t,uint64_t>(dpos,0));

					dpos += OVL.serialiseWithPath(DOSI,small);
				}
			};
		}
	}
}
#endif
