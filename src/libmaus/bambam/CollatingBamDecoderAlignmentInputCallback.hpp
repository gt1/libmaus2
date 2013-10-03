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
#if ! defined(LIBMAUS_BAMBAM_COLLATINGBAMDECODERALIGNMENTINPUTCALLBACK_HPP)
#define LIBMAUS_BAMBAM_COLLATINGBAMDECODERALIGNMENTINPUTCALLBACK_HPP

#include <libmaus/bambam/BamAlignment.hpp>

namespace libmaus
{
	namespace bambam
	{
		/**
		 * input callback for collating bam decoders
		 **/
		struct CollatingBamDecoderAlignmentInputCallback
		{
			virtual ~CollatingBamDecoderAlignmentInputCallback() {}
			/**
			 * pure virtual input callback; this is to be called each time a collating
			 * decoder decodes an alignment from the original stream
			 *
			 * @param A alignment structure
			 **/
			virtual void operator()(::libmaus::bambam::BamAlignment const & A) = 0;
		};
	}
}
#endif
