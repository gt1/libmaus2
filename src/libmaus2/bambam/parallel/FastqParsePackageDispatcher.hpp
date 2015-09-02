/*
    libmaus2
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
#if ! defined(LIBMAUS2_BAMBAM_PARALLEL_FASTQPARSEPACKAGEDISPATCHER_HPP)
#define LIBMAUS2_BAMBAM_PARALLEL_FASTQPARSEPACKAGEDISPATCHER_HPP

#include <libmaus2/bambam/parallel/FastqParsePackageFinishedInterface.hpp>
#include <libmaus2/bambam/parallel/FastqParsePackageReturnInterface.hpp>
#include <libmaus2/parallel/SimpleThreadWorkPackageDispatcher.hpp>
#include <libmaus2/bambam/BamSeqEncodeTable.hpp>
#include <libmaus2/fastx/SpaceTable.hpp>
#include <libmaus2/fastx/CharBuffer.hpp>
#include <libmaus2/bambam/BamAlignment.hpp>
#include <libmaus2/fastx/NameInfo.hpp>

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{

			struct FastqParsePackageDispatcher : public libmaus2::parallel::SimpleThreadWorkPackageDispatcher
			{
				typedef FastqParsePackageDispatcher this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
				
				FastqParsePackageReturnInterface & packageReturnInterface;
				FastqParsePackageFinishedInterface & addPendingInterface;
				libmaus2::bambam::BamSeqEncodeTable const seqenc;
				libmaus2::fastx::SpaceTable const ST;
				std::string const rgid;
				libmaus2::autoarray::AutoArray<uint8_t> rgA;
							
				FastqParsePackageDispatcher(
					FastqParsePackageReturnInterface & rpackageReturnInterface,
					FastqParsePackageFinishedInterface & raddPendingInterface,
					std::string const rrgid
				)
				: packageReturnInterface(rpackageReturnInterface), addPendingInterface(raddPendingInterface), seqenc(), ST(), rgid(rrgid),
				  rgA(rgid.size() ? (rgid.size() + 4) : 0, false)
				{
					if ( rgid.size() )
					{
						rgA[0] = 'R';
						rgA[1] = 'G';
						rgA[2] = 'Z';
						std::copy(rgid.begin(),rgid.end(),rgA.begin()+3);
						rgA[3 + rgid.size()] = 0;						
					}
				}
			
				void dispatch(
					libmaus2::parallel::SimpleThreadWorkPackage * P, 
					libmaus2::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & /* tpi */
				)
				{
					assert ( dynamic_cast<FastqParsePackage *>(P) != 0 );
					FastqParsePackage * BP = dynamic_cast<FastqParsePackage *>(P);
					
					FastqToBamControlSubReadPending & data = BP->data;
					FastQInputDescBase::input_block_type::shared_ptr_type & block = data.block;
					DecompressedBlock::shared_ptr_type & deblock = data.deblock;

					deblock->blockid = data.absid;
					deblock->final = block->meta.eof && ( data.subid+1 == block->meta.blocks.size() );
					deblock->P = deblock->D.begin();
					deblock->uncompdatasize = 0;

					libmaus2::fastx::EntityBuffer<uint8_t,libmaus2::bambam::BamAlignment::D_array_alloc_type> buffer;
					libmaus2::bambam::BamAlignment algn;
					
					std::pair<uint8_t *, uint8_t *> Q = block->meta.blocks[data.subid];
					while ( Q.first != Q.second )
					{
						std::pair<uint8_t *,uint8_t *> ls[4];
						
						for ( unsigned int i = 0; i < 4; ++i )
						{
							uint8_t * s = Q.first;
							while ( Q.first != Q.second && Q.first[0] != '\n' )
								++Q.first;
							if ( Q.first == Q.second )
							{
								libmaus2::exception::LibMausException lme;
								lme.getStream() << "Unexpected EOF." << std::endl;
								lme.finish();
								throw lme;
							}
							assert ( Q.first[0] == '\n' );
							ls[i] = std::pair<uint8_t *,uint8_t *>(s,Q.first);
							Q.first += 1;
						}
						
						uint8_t const * name = ls[0].first+1;
						uint32_t const namelen = ls[0].second-ls[0].first-1;
						int32_t const refid = -1;
						int32_t const pos = -1;
						uint32_t const mapq = 0;
						libmaus2::bambam::cigar_operation const * cigar = 0;
						uint32_t const cigarlen = 0;
						int32_t const nextrefid = -1;
						int32_t const nextpos = -1;
						uint32_t const tlen = 0;
						uint8_t const * seq = ls[1].first;
						uint32_t const seqlen = ls[1].second-ls[1].first;
						uint8_t const * qual = ls[3].first;
						int const qualshift = 33;

						libmaus2::fastx::NameInfo const NI(
							name,namelen,ST,libmaus2::fastx::NameInfoBase::fastq_name_scheme_generic
						);
						
						uint32_t flags = libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FUNMAP;
						if ( NI.ispair )
						{
							flags |= libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FPAIRED;
							flags |= libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FMUNMAP;
							if ( NI.isfirst )
								flags |= libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FREAD1;
							else
								flags |= libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FREAD2;
						}
						
						std::pair<uint8_t const *,uint8_t const *> NP = NI.getName();

						libmaus2::bambam::BamAlignmentEncoderBase::encodeAlignment
						(
							buffer,seqenc,NP.first,NP.second-NP.first,refid,pos,mapq,flags,cigar,cigarlen,
							nextrefid,nextpos,tlen,seq,seqlen,qual,qualshift
						);
						
						if ( rgA.size() )
							for ( uint64_t i = 0; i < rgA.size(); ++i )
								buffer.bufferPush(rgA[i]);
						
						algn.blocksize = buffer.swapBuffer(algn.D);
						
						deblock->pushData      (algn.D.begin(), algn.blocksize);
					}
					
					addPendingInterface.fastqParsePackageFinished(BP->data);
					packageReturnInterface.fastqParsePackageReturn(BP);
				}
			};

		}
	}
}
#endif
