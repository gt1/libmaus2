/*
    libmaus
    Copyright (C) 2009-2013 German Tischler
    Copyright (C) 2011-2013 Genome Research Limited

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
#if ! defined(LIBMAUS_BAMBAM_BAMALIGNMENTINPUTCALLBACKBAM_HPP)
#define LIBMAUS_BAMBAM_BAMALIGNMENTINPUTCALLBACKBAM_HPP

#include <libmaus/bambam/CircularHashCollatingBamDecoder.hpp>
#include <libmaus/bambam/BamWriter.hpp>
#include <libmaus/bambam/BamAlignmentInputPositionCallbackNull.hpp>

namespace libmaus
{
	namespace bambam
	{
		template<typename _update_base_type = libmaus::bambam::BamAlignmentInputPositionCallbackNull>
		struct BamAlignmentInputCallbackBam : 
			public ::libmaus::bambam::CollatingBamDecoderAlignmentInputCallback,
			public _update_base_type
		{
			typedef _update_base_type update_base_type;
			typedef BamAlignmentInputCallbackBam<update_base_type> this_type;
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

			uint64_t als;
			::libmaus::bambam::BamWriter::unique_ptr_type BWR;
			
			BamAlignmentInputCallbackBam(
				std::string const & filename,
				::libmaus::bambam::BamHeader const & bamheader,
				int const rewritebamlevel
			)
			: update_base_type(bamheader), als(0), BWR(new ::libmaus::bambam::BamWriter(filename,bamheader,rewritebamlevel))
			{
				
			}
			
			~BamAlignmentInputCallbackBam()
			{
			}

			void operator()(::libmaus::bambam::BamAlignment const & A)
			{
				als++;
				update_base_type::updatePosition(A);
				A.serialise(BWR->getStream());
			}	
		};
	}
}
#endif
