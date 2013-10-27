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

namespace libmaus
{
	namespace bambam
	{
		struct BamAlignmentInputCallbackBam : public ::libmaus::bambam::CollatingBamDecoderAlignmentInputCallback
		{
			typedef BamAlignmentInputCallbackBam this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

			uint64_t als;
			::libmaus::bambam::BamWriter::unique_ptr_type BWR;
			
			BamAlignmentInputCallbackBam(
				std::string const & filename,
				::libmaus::bambam::BamHeader const & bamheader,
				int const rewritebamlevel
			)
			: als(0), BWR(new ::libmaus::bambam::BamWriter(filename,bamheader,rewritebamlevel))
			{
				
			}
			
			~BamAlignmentInputCallbackBam()
			{
			}

			void operator()(::libmaus::bambam::BamAlignment const & A)
			{
				// std::cerr << "Callback." << std::endl;
				als++;
				A.serialise(BWR->getStream());
			}	
		};
	}
}
#endif
