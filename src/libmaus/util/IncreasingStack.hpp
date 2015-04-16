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

#if ! defined(INCREASINGSTACK_HPP)
#define INCREASINGSTACK_HPP

#include <libmaus/types/types.hpp>
#include <libmaus/bitio/BitVector.hpp>

struct IncreasingStack
{
	uint64_t n;
	uint64_t b;
	::libmaus::bitio::BitVector data;
	::libmaus::bitio::BitVector blockused;
	uint64_t topv;
	uint64_t fill;
	
	IncreasingStack(uint64_t const rn)
	: n(rn), b( (n+63)/64 ), data(n), blockused(b), topv(0), fill(0)
	{
	}
	
	void push(uint64_t const i)
	{
		uint64_t const block = i>>6;
		
		bool const blockpreempty = !(data.A[block]);
		data[i] = true;
		blockused[block] = true;
		
		if ( blockpreempty && block && ! blockused[block-1] )
			data.A[block-1] = topv;

		topv = i;
		fill++;
	}
	
	uint64_t top() const
	{
		if ( ! fill )
		{
			::libmaus::exception::LibMausException se;
			se.getStream() << "IncreasingStack::top() called on empty stack." << std::endl;
			se.finish();
			throw se;
		}
		return topv;
	}
	
	void pop()
	{
		uint64_t const r = topv;
		uint64_t const block = (r>>6);
		
		// erase bit
		data[r] = false;
		// more bits in block?
		if ( ! data.A[block] )
			blockused[block] = false;
		
		if ( --fill )
		{
			// another bit in same block or block below?
			if ( blockused[block] || (block&&blockused[block-1]) )
			{
				// go back until we find the bit
				uint64_t i = r;
				while ( ! data[i] )
					--i;
				topv = i;
			}
			// no more bits in same block
			else
			{
				assert ( block && !blockused[block-1] );

				// get pointer
				topv = data.A[block-1];
				// erase pointer
				data.A[block-1] = 0;					
			}		
		}
	}
};
#endif
