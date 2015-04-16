/*
    libmaus
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2015 Genome Research Limited

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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_SAMPARSEWORKPACKAGEDISPATCHER_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_SAMPARSEWORKPACKAGEDISPATCHER_HPP

#include <libmaus/bambam/parallel/SamParsePutSamInfoInterface.hpp>
#include <libmaus/bambam/parallel/SamParseGetSamInfoInterface.hpp>
#include <libmaus/bambam/parallel/SamParseFragmentParsedInterface.hpp>
#include <libmaus/bambam/parallel/SamParseDecompressedBlockFinishedInterface.hpp>
#include <libmaus/bambam/parallel/SamParseWorkPackageReturnInterface.hpp>
#include <libmaus/bambam/parallel/SamParseWorkPackage.hpp>
#include <libmaus/parallel/SimpleThreadWorkPackageDispatcher.hpp>

namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{
			struct SamParseWorkPackageDispatcher : libmaus::parallel::SimpleThreadWorkPackageDispatcher
			{
				typedef SamParseWorkPackage this_type;
				typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
				
				SamParseWorkPackageReturnInterface & packageReturnInterface;
				SamParseDecompressedBlockFinishedInterface & blockFinishedInterface;
				SamParseFragmentParsedInterface & fragmentParsedInterface;
				SamParseGetSamInfoInterface & getSamInfoInterface;
				SamParsePutSamInfoInterface & putSamInfoInterface;
				
				SamParseWorkPackageDispatcher(
					SamParseWorkPackageReturnInterface & rpackageReturnInterface,
					SamParseDecompressedBlockFinishedInterface & rblockFinishedInterface,
					SamParseFragmentParsedInterface & rfragmentParsedInterface,
					SamParseGetSamInfoInterface & rgetSamInfoInterface,
					SamParsePutSamInfoInterface & rputSamInfoInterface
				)
				:
					packageReturnInterface(rpackageReturnInterface),
					blockFinishedInterface(rblockFinishedInterface),
					fragmentParsedInterface(rfragmentParsedInterface),
					getSamInfoInterface(rgetSamInfoInterface),
					putSamInfoInterface(rputSamInfoInterface)
				{
				
				}
						
				void dispatch(libmaus::parallel::SimpleThreadWorkPackage * P, libmaus::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & /* tpi */)
				{
					SamParseWorkPackage * BP = dynamic_cast<SamParseWorkPackage *>(P);
					assert ( BP );
					
					SamParsePending & SPP = BP->SPP;

					libmaus::bambam::parallel::GenericInputBase::generic_input_shared_block_ptr_type & block = SPP.block;
					uint64_t const subid = SPP.subid;
					uint64_t const absid = SPP.absid;
					uint64_t const streamid = BP->streamid;
					std::pair<uint8_t *,uint8_t *> Q = block->meta.blocks[subid];
					libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type db = BP->db;
					libmaus::bambam::SamInfo::shared_ptr_type saminfo = getSamInfoInterface.samParseGetSamInfo();
					libmaus::bambam::BamAlignment & algn = saminfo->algn;

					db->uncompdatasize = 0;
					db->P = db->D.begin();
					db->final = block->meta.eof && (subid+1 == block->meta.blocks.size());
					db->streamid = streamid;
					db->blockid = absid;
					
					if ( Q.first != Q.second )
					{
						assert ( Q.second[-1] == '\n' );
						while ( Q.first != Q.second )
						{
							uint8_t * p = Q.first;
							while ( *p != '\n' )
								++p;
							assert ( *p == '\n' );
						
							saminfo->parseSamLine(reinterpret_cast<char const *>(Q.first),reinterpret_cast<char const *>(p));
							db->pushData(algn.D.begin(),algn.blocksize);
						
							Q.first = ++p;
						}
					}
					
					putSamInfoInterface.samParsePutSamInfo(saminfo);
					blockFinishedInterface.samParseDecompressedBlockFinished(streamid,db);
					fragmentParsedInterface.samParseFragmentParsed(SPP);
					packageReturnInterface.samParseWorkPackageReturn(BP);
				}
			};
		}
	}
}
#endif
