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


#if ! defined(DYNAMICCUMULATIVEFREQUENCIES_HPP)
#define DYNAMICCUMULATIVEFREQUENCIES_HPP

/**
 * cumulative frequencies with updates
 * as presented in
 * Moffat, Neal, Witten: Arithmetic coding revisited
 * data structure according to Fenwick
 **/

#include <libmaus/types/types.hpp>
#include <libmaus/autoarray/AutoArray.hpp>

namespace libmaus
{
	namespace cumfreq
	{
		struct DynamicCumulativeFrequencies
		{
			private:
			uint64_t const sigma;
			::libmaus::autoarray::AutoArray<uint64_t> table;

			public:
			DynamicCumulativeFrequencies(uint64_t rsigma)
			: sigma(rsigma),  table(sigma) {}

			uint64_t operator[](uint64_t const key) const
			{
				uint64_t cumfreq = 0;
				uint64_t left = 0;
				uint64_t width = sigma;

				while ( width )
				{
					uint64_t width2 = width>>1;
					uint64_t const mid = left+width2;
					
					// key is larger than middle,
					// add this power of two
					if ( key > mid )
					{
						cumfreq += table[mid];
						// set new interval
						left = mid+1;
						// if interval width is even, decrease by one
						if ( !(width&1) )
							width2 -= 1;
					}
					
					width = width2;
				}
				
				return cumfreq;
			}

			uint64_t get(uint64_t const key) const
			{
				return this->operator[](key);
			}

			uint64_t add(uint64_t const key, uint64_t const v)
			{
				uint64_t cumfreq = 0;
				uint64_t left = 0;
				uint64_t width = sigma;

				while ( width )
				{
					uint64_t width2 = width>>1;
					uint64_t const m = left+width2;
					
					// key is larger than middle,
					// add this power of two
					if ( key > m )
					{
						cumfreq += table[m];
						// set new interval
						left = m+1;
						// if interval width is even, decrease by one
						if ( !(width&1) )
							width2 -= 1;
					}
					else
					{
						table[m] += v;
					}
					
					width = width2;
				}
				
				return cumfreq;
			}
			
			uint64_t inc(uint64_t const key)
			{
				return add(key,1);
			}

			void half()
			{
				for ( uint64_t i = 0; i < table.size(); ++i )
					table[i] /= 2;
			}
		};
	}
}
#endif
