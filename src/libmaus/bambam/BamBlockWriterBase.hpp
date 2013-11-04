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
#if ! defined(LIBMAUS_BAMBAM_BAMBLOCKWRITERBASE_HPP)
#define LIBMAUS_BAMBAM_BAMBLOCKWRITERBASE_HPP

#include <libmaus/types/types.hpp>
#include <libmaus/util/unique_ptr.hpp>
#include <libmaus/util/shared_ptr.hpp>
#include <libmaus/bambam/BamAlignment.hpp>

namespace libmaus
{
	namespace bambam
	{
		struct BamBlockWriterBase
		{
			typedef BamBlockWriterBase this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
		
			virtual ~BamBlockWriterBase() {}

			/**
			 * write a BAM data block
			 **/
			virtual void writeBamBlock(uint8_t const *, uint64_t const) = 0;
			
			/**
			 * write alignment
			 **/
			virtual void writeAlignment(libmaus::bambam::BamAlignment const & A)
			{
				writeBamBlock(A.D.begin(),A.blocksize);
			}
		};
	}
}
#endif
