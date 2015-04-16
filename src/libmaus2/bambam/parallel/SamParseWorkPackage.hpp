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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_SAMPARSEWORKPACKAGE_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_SAMPARSEWORKPACKAGE_HPP

#include <libmaus/bambam/parallel/SamParsePending.hpp>
#include <libmaus/bambam/parallel/DecompressedBlock.hpp>
#include <libmaus/parallel/SimpleThreadWorkPackage.hpp>

namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{
			struct SamParseWorkPackage : public libmaus::parallel::SimpleThreadWorkPackage
			{
				typedef SamParseWorkPackage this_type;
				typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
				
				uint64_t streamid;
				SamParsePending SPP;
				libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type db;
				
				SamParseWorkPackage()
				: libmaus::parallel::SimpleThreadWorkPackage(), streamid(0), SPP(), db()
				{	
				}		
				SamParseWorkPackage(
					uint64_t const rpriority, 
					uint64_t const rdispatcherid, 
					uint64_t const rstreamid,
					SamParsePending const & rSPP,
					libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type rdb
				)
				: libmaus::parallel::SimpleThreadWorkPackage(rpriority,rdispatcherid), streamid(rstreamid), SPP(rSPP), db(rdb)
				{
				
				}
				virtual ~SamParseWorkPackage() {}
			
				char const * getPackageName() const
				{
					return "SamParseWorkPackage";
				}
			};
		}
	}
}
#endif
