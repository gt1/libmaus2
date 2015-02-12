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
#if ! defined(LIBMAUS_BAMBAM_DUPSETCALLBACKSHAREDVECTOR_HPP)
#define LIBMAUS_BAMBAM_DUPSETCALLBACKSHAREDVECTOR_HPP

#include <libmaus/bambam/DupSetCallback.hpp>
#include <libmaus/bambam/DuplicationMetrics.hpp>
#include <libmaus/util/unique_ptr.hpp>

namespace libmaus
{
	namespace bambam
	{
		struct DupSetCallbackSharedVector : public ::libmaus::bambam::DupSetCallback
		{
			typedef DupSetCallbackSharedVector this_type;
		
			typedef std::map<uint64_t,::libmaus::bambam::DuplicationMetrics> map_type;
			typedef libmaus::util::unique_ptr<map_type>::type map_ptr_type;
			
			::libmaus::bitio::BitVector & B;
			map_type metrics;

			DupSetCallbackSharedVector(::libmaus::bitio::BitVector & rB) : B(rB), metrics() {}
			
			void operator()(::libmaus::bambam::ReadEnds const & A)
			{
				B.setSync(A.getRead1IndexInFile(),true);
				
				if ( A.isPaired() )
				{
					B.setSync(A.getRead2IndexInFile(),true);
					metrics[A.getLibraryId()].readpairduplicates++;
				}
				else
				{
					metrics[A.getLibraryId()].unpairedreadduplicates++;
				}
			}	

			uint64_t getNumDups() const
			{
				uint64_t dups = 0;
				for ( uint64_t i = 0; i < B.size(); ++i )
					if ( B.get(i) )
						dups++;
				
				return dups;
			}
			void addOpticalDuplicates(uint64_t const libid, uint64_t const count)
			{
				metrics[libid].opticalduplicates += count;
			}

			bool isMarked(uint64_t const i) const
			{
				return B[i];
			}
			
			void flush(uint64_t const)
			{
			
			}
		};
	}
}
#endif
