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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_DECOMPRESSBLOCKWORKPACKAGE_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_DECOMPRESSBLOCKWORKPACKAGE_HPP

#include <libmaus/bambam/parallel/ControlInputInfo.hpp>
#include <libmaus/bambam/parallel/DecompressedBlock.hpp>
#include <libmaus/parallel/SimpleThreadWorkPackage.hpp>

namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{
			struct DecompressBlockWorkPackage : public libmaus::parallel::SimpleThreadWorkPackage
			{
				typedef DecompressBlockWorkPackage this_type;
				typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
				ControlInputInfo::input_block_type * inputblock;
				DecompressedBlock * outputblock;
				libmaus::lz::BgzfInflateZStreamBase * decoder;
	
				DecompressBlockWorkPackage()
				: 
					libmaus::parallel::SimpleThreadWorkPackage(), 
					inputblock(0),
					outputblock(0),
					decoder(0)
				{
				}
				
				DecompressBlockWorkPackage(
					uint64_t const rpriority, 
					ControlInputInfo::input_block_type * rinputblock,
					DecompressedBlock * routputblock,
					libmaus::lz::BgzfInflateZStreamBase * rdecoder,
					uint64_t const rdecompressDispatcherId
				)
				: libmaus::parallel::SimpleThreadWorkPackage(rpriority,rdecompressDispatcherId), inputblock(rinputblock), outputblock(routputblock), decoder(rdecoder)
				{
				}
			
				char const * getPackageName() const
				{
					return "DecompressBlockWorkPackage";
				}
			};
		}
	}
}
#endif
