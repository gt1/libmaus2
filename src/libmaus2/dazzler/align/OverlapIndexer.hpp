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

#include <libmaus2/util/CountPutObject.hpp>
#include <libmaus2/index/ExternalMemoryIndexGenerator.hpp>
#include <libmaus2/dazzler/align/OverlapMeta.hpp>
#include <libmaus2/dazzler/align/OverlapIndexerBase.hpp>		
#include <libmaus2/dazzler/align/AlignmentFile.hpp>
		
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
						return std::pair<bool,uint64_t>(false,0);	
					}
				}
				
				static std::string constructIndex(std::string const & aligns)
				{
					std::string const indexfn = std::string(".") + aligns + ".idx";
					libmaus2::index::ExternalMemoryIndexGenerator<OverlapMeta,base_level_log,inner_level_log> EMIG(indexfn);
					EMIG.setup();

					// open alignment file
					libmaus2::aio::InputStreamInstance algnfile(aligns);
					libmaus2::dazzler::align::AlignmentFile algn(algnfile);
					libmaus2::dazzler::align::Overlap OVL;

					// current a-read id
					libmaus2::dazzler::align::Overlap OVLprev;
					bool haveprev;
					int64_t lp = 0;
					std::pair<bool,uint64_t> P;

					// get next overlap                                                                                                                                                                                                                                
					while ( (P=OverlapIndexer::getOverlapAndOffset(algn,algnfile,OVL)).first )
					{
						if ( haveprev )
						{
							bool const ok = !(OVL < OVLprev);

							if ( !ok )
							{
								libmaus2::exception::LibMausException lme;
								lme.getStream() << "file " << aligns << " is not sorted" << std::endl;
								lme.finish();
								throw lme;
							}
						}
						
						if ( (lp++ & EMIG.base_index_mask) == 0 )
						{
							EMIG.put(
								OverlapMeta(OVL),
								std::pair<uint64_t,uint64_t>(P.second,0)
							);
						}
					
						haveprev = true;
						OVLprev = OVL;
					}
					
					EMIG.flush();
					
					return indexfn;
				}
			};
		}
	}
}
#endif
