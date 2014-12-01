/*
    libmaus
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#if ! defined(LIBMAUS_BAMBAM_BAMPARALLELDECODINGALIGNMENTREWRITEBUFFERPOSCOMPARATOR_HPP)
#define LIBMAUS_BAMBAM_BAMPARALLELDECODINGALIGNMENTREWRITEBUFFERPOSCOMPARATOR_HPP

#include <libmaus/bambam/BamAlignmentDecoderBase.hpp>
#include <libmaus/bambam/BamParallelDecodingAlignmentRewriteBuffer.hpp>

namespace libmaus
{
	namespace bambam
	{		
		struct BamParallelDecodingAlignmentRewriteBufferPosComparator
		{
			BamParallelDecodingAlignmentRewriteBuffer * buffer;
			uint8_t const * text;
			
			BamParallelDecodingAlignmentRewriteBufferPosComparator(BamParallelDecodingAlignmentRewriteBuffer * rbuffer)
			: buffer(rbuffer), text(buffer->A.begin())
			{
			
			}
			
			bool operator()(BamParallelDecodingAlignmentRewriteBuffer::pointer_type A, BamParallelDecodingAlignmentRewriteBuffer::pointer_type B) const
			{
				uint8_t const * pa = text + A + (sizeof(uint32_t) + sizeof(uint64_t));
				uint8_t const * pb = text + B + (sizeof(uint32_t) + sizeof(uint64_t));

				int32_t const refa = ::libmaus::bambam::BamAlignmentDecoderBase::getRefID(pa);
				int32_t const refb = ::libmaus::bambam::BamAlignmentDecoderBase::getRefID(pb);
			
				if ( refa != refb )
					return  static_cast<uint32_t>(refa) < static_cast<uint32_t>(refb);

				int32_t const posa = ::libmaus::bambam::BamAlignmentDecoderBase::getPos(pa);
				int32_t const posb = ::libmaus::bambam::BamAlignmentDecoderBase::getPos(pb);
				
				return posa < posb;
			}
		};
	}
}
#endif
