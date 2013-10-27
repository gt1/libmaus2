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
#if ! defined(LIBMAUS_BAMBAM_DUPSETCALLBACKVECTOR_HPP)
#define LIBMAUS_BAMBAM_DUPSETCALLBACKVECTOR_HPP

#include <libmaus/bambam/DupSetCallback.hpp>
#include <libmaus/bambam/DuplicationMetrics.hpp>

namespace libmaus
{
	namespace bambam
	{
		struct DupSetCallbackVector : public ::libmaus::bambam::DupSetCallback
		{
			::libmaus::bitio::BitVector B;

			std::map<uint64_t,::libmaus::bambam::DuplicationMetrics> & metrics;

			DupSetCallbackVector(
				uint64_t const n,
				std::map<uint64_t,::libmaus::bambam::DuplicationMetrics> & rmetrics
			) : B(n), metrics(rmetrics) /* unpairedreadduplicates(), readpairduplicates(), metrics(rmetrics) */ {}
			
			void operator()(::libmaus::bambam::ReadEnds const & A)
			{
				B.set(A.getRead1IndexInFile(),true);
				
				if ( A.isPaired() )
				{
					B.set(A.getRead2IndexInFile(),true);
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
