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
#if ! defined(LIBMAUS_BAMBAM_DUPSETCALLBACKSET_HPP)
#define LIBMAUS_BAMBAM_DUPSETCALLBACKSET_HPP

#include <libmaus/bambam/DupSetCallback.hpp>

namespace libmaus
{
	namespace bambam
	{
		struct DupSetCallbackSet : public ::libmaus::bambam::DupSetCallback
		{
			std::set<uint64_t> S;

			virtual void operator()(::libmaus::bambam::ReadEnds const & A)
			{
				S.insert(A.getRead1IndexInFile());
				
				if ( A.isPaired() )
					S.insert(A.getRead2IndexInFile());
			}
			virtual uint64_t getNumDups() const
			{
				return S.size();
			}
			virtual void addOpticalDuplicates(uint64_t const, uint64_t const)
			{
			
			}
			virtual bool isMarked(uint64_t const i) const
			{
				return S.find(i) != S.end();
			}
			virtual void flush(uint64_t const)
			{
			
			}
		};
	}
}
#endif
