/**
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
**/
#if ! defined(LIBMAUS_SUFFIXSORT_CIRCULARSUFFIXCOMPARATOR_HPP)
#define LIBMAUS_SUFFIXSORT_CIRCULARSUFFIXCOMPARATOR_HPP

#include <libmaus/aio/CircularWrapper.hpp>

namespace libmaus
{
	namespace suffixsort
	{
		struct CircularSuffixComparator
		{
			std::string const & filename;
			uint64_t const fs;
			
			CircularSuffixComparator(std::string const & rfilename)
			: filename(rfilename), fs(::libmaus::util::GetFileSize::getFileSize(filename))
			{
			}
			
			bool operator()(uint64_t pa, uint64_t pb) const
			{
				assert ( fs );
				
				pa %= fs;
				pb %= fs;
				
				if ( pa == pb )
					return false;
					
				::libmaus::aio::CircularWrapper cwa(filename,pa);
				::libmaus::aio::CircularWrapper cwb(filename,pb);
			
				for ( uint64_t i = 0; i < fs; ++i )
				{
					int const ca = cwa.get();
					int const cb = cwb.get();
					
					assert ( ca >= 0 );
					assert ( cb >= 0 );
					
					if ( ca != cb )
						return ca < cb;
				}
				
				return pa < pb;
			}

			// search for smallest suffix in SA that equals q or is larger than q
			template<typename saidx_t>
			static uint64_t suffixSearch(saidx_t const * SA, uint64_t const n, uint64_t const o, uint64_t const q, std::string const & filename)
			{
				uint64_t l = 0, r = n;
				::libmaus::suffixsort::CircularSuffixComparator CSC(filename);
				
				// binary search
				while ( r-l > 2 )
				{
					uint64_t const m = (l+r)>>1;		

					// is m too small? i.e. SA[m] < q
					if ( CSC(SA[m]+o,q) )
					{
						l = m+1;
					}
					// m is large enough
					else
					{
						r = m+1;
					}
				}
				
				// ! (SA[l] >= q) <=> q < SA[l]
				while ( l < r && CSC(SA[l]+o,q) )
					++l;
				
				if ( l < n )
				{
					// SA[l] >= q
					assert ( ! CSC(SA[l]+o,q) );
				}
				
				return l;
			}
		};
	}
}
#endif
