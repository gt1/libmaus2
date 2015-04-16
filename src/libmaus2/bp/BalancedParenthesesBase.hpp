/**
    libmaus2
    Copyright (C) 2007-2012 Simon Gog All Right Reserved.
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
#if ! defined(BALANCEDPARENTHESESBASE_HPP)
#define BALANCEDPARENTHESESBASE_HPP

#include <libmaus2/util/NearestNeighbourDictionary.hpp>
#include <libmaus2/util/IncreasingStack.hpp>

struct BalancedParenthesesBase
{
	typedef uint8_t lookup_type;
	static unsigned int const lookup_bits = 8*sizeof(lookup_type);

	::libmaus2::autoarray::AutoArray<uint8_t> openLookup;
	::libmaus2::autoarray::AutoArray<uint8_t> closeLookup;
	::libmaus2::autoarray::AutoArray<int8_t> leftMatchLookup;
	::libmaus2::autoarray::AutoArray<int8_t> rightMatchLookup;
	::libmaus2::autoarray::AutoArray<int8_t> excessLookup;
	
	BalancedParenthesesBase()
	: 
		openLookup( (1ull<<lookup_bits) * lookup_bits ), 
		closeLookup((1ull<<lookup_bits)*lookup_bits),
		leftMatchLookup( (1ull<<lookup_bits) * lookup_bits ), 
		rightMatchLookup((1ull<<lookup_bits)*lookup_bits),
		excessLookup((1ull<<lookup_bits))
	{
		for ( uint64_t i = 0; i < (1ull<<lookup_bits); ++i )
		{
			for ( uint64_t j = 0; j < lookup_bits; ++j )
			{
				openLookup [ i * lookup_bits + j ] = singleFindOpen(static_cast<lookup_type>(i),j);
				closeLookup [ i * lookup_bits + j ] = singleFindClose(static_cast<lookup_type>(i),j);
				leftMatchLookup [ i * lookup_bits + j ] = singleMatchExcessLeft(static_cast<lookup_type>(i),static_cast<int8_t>(j+1));
				rightMatchLookup [ i * lookup_bits + j ] = singleMatchExcessRight(static_cast<lookup_type>(i),-static_cast<int8_t>(j+1));
			}
			excessLookup[i] = 
				static_cast<int64_t>(::libmaus2::rank::PopCnt8<sizeof(long)>::popcnt8( (i ) & ::libmaus2::math::lowbits(lookup_bits) ))
				-
				static_cast<int64_t>(::libmaus2::rank::PopCnt8<sizeof(long)>::popcnt8( (~i) & ::libmaus2::math::lowbits(lookup_bits) ))
				;
		}
	}
	
	uint64_t wordOpen(lookup_type const v, uint64_t const i) const
	{
		return openLookup [ static_cast<uint64_t>(v) * lookup_bits + i ];
	}
	uint64_t wordClose(lookup_type const v, uint64_t const i) const
	{
		return closeLookup [ static_cast<uint64_t>(v) * lookup_bits + i ];
	}

	int8_t wordLeftMatch(lookup_type const v, uint64_t const i) const
	{
		return leftMatchLookup [ static_cast<uint64_t>(v) * lookup_bits + i ];
	}
	int8_t wordRightMatch(lookup_type const v, uint64_t const i) const
	{
		return rightMatchLookup [ static_cast<uint64_t>(v) * lookup_bits + i ];
	}
	
	template<typename iterator>
	uint64_t lookupFindClose(iterator I, uint64_t i) const
	{
		// closing?
		if ( ! ::libmaus2::bitio::getBit(I,i) )
			return i;

		// which lookup block is i in ?
		uint64_t const lookupblock = (i / lookup_bits);
		// bit position in lookup block
		uint64_t const lookupoffset = i - lookupblock*lookup_bits;
		// lookup word
		lookup_type const lookupword = ::libmaus2::bitio::getBits(I,lookupblock*lookup_bits,lookup_bits);
		// is closing parenthesis in this block?
		uint8_t const lookupres = wordClose( lookupword, lookupoffset  );
		
		if ( lookupres != lookupoffset )
			return lookupblock*lookup_bits + lookupres;

		// next scan position
		i = (lookupblock+1)*lookup_bits;

		// lookup parenthesis is not in this block, extract rest of lookup word
		unsigned int const lookuprestbits = lookup_bits-lookupoffset;
		lookup_type const lookuprest = lookupword & ::libmaus2::math::lowbits(lookuprestbits);
		// compute number of opening parentheses in rest of word
		unsigned int const opening = ::libmaus2::rank::PopCnt8<sizeof(unsigned long)>::popcnt8(lookuprest);
		// compute number of closing parentheses in rest of word
		unsigned int const closing = lookuprestbits-opening;		
		// current excess
		uint64_t excess = opening-closing;
		
		// get next word
		lookup_type nextlookupword = ::libmaus2::bitio::getBits(I,i,lookup_bits);
		
		// while excess is too large for lookup or closing parenthesis is not in lookup word
		while ( excess > lookup_bits || static_cast<uint64_t>(wordLeftMatch(nextlookupword,excess-1)) == lookup_bits )
		{
			excess += excessLookup[nextlookupword];
			i += lookup_bits;
			nextlookupword = ::libmaus2::bitio::getBits(I,i,lookup_bits);
		}

		return i + wordLeftMatch(nextlookupword,excess-1);
	}

	template<typename iterator>
	uint64_t lookupFindCloseOrFar(iterator I, uint64_t i, uint64_t const blocksize) const
	{
		// closing?
		if ( ! ::libmaus2::bitio::getBit(I,i) )
			return i;

		// which lookup block is i in ?
		uint64_t const lookupblock = (i / lookup_bits);
		// bit position in lookup block
		uint64_t const lookupoffset = i - lookupblock*lookup_bits;
		// lookup word
		lookup_type const lookupword = ::libmaus2::bitio::getBits(I,lookupblock*lookup_bits,lookup_bits);
		// is closing parenthesis in this block?
		uint8_t const lookupres = wordClose( lookupword, lookupoffset  );
		
		if ( lookupres != lookupoffset )
			return lookupblock*lookup_bits + lookupres;
		
		// next scan position
		i = (lookupblock+1)*lookup_bits;
		
		if ( i%blocksize == 0 )
			return ::std::numeric_limits<uint64_t>::max();
		
		// lookup parenthesis is not in this block, extract rest of lookup word
		unsigned int const lookuprestbits = lookup_bits-lookupoffset;
		lookup_type const lookuprest = lookupword & ::libmaus2::math::lowbits(lookuprestbits);
		// compute number of opening parentheses in rest of word
		unsigned int const opening = ::libmaus2::rank::PopCnt8<sizeof(unsigned long)>::popcnt8(lookuprest);
		// compute number of closing parentheses in rest of word
		unsigned int const closing = lookuprestbits-opening;
		// current excess
		uint64_t excess = opening-closing;
		
		// get next word
		lookup_type nextlookupword = ::libmaus2::bitio::getBits(I,i,lookup_bits);
		
		// while excess is too large for lookup or closing parenthesis is not in lookup word
		while ( excess > lookup_bits || static_cast<uint64_t>(wordLeftMatch(nextlookupword,excess-1)) == lookup_bits )
		{
			excess += excessLookup[nextlookupword];
			i += lookup_bits;
			
			if ( i%blocksize == 0 )
				return ::std::numeric_limits<uint64_t>::max();

			nextlookupword = ::libmaus2::bitio::getBits(I,i,lookup_bits);
		}

		return i + wordLeftMatch(nextlookupword,excess-1);
	}

	template<typename iterator>
	uint64_t lookupFindOpen(iterator I, uint64_t i) const
	{
		// is par at position i opening?
		if ( ::libmaus2::bitio::getBit(I,i) )
			return i;

		// check word
		uint64_t const lookupblock = (i / lookup_bits);
		uint64_t const lookupoffset = i - lookupblock*lookup_bits;
		lookup_type const lookupword = ::libmaus2::bitio::getBits(I,lookupblock*lookup_bits,lookup_bits);
		int8_t const lookupres = wordOpen( lookupword, lookupoffset  );

		if ( lookupres != static_cast<int64_t>(lookupoffset) )
			return lookupblock*lookup_bits + lookupres;
		
		// compute excess on part of word
		unsigned int const usebits = lookupoffset+1;
		unsigned int const restbits = lookup_bits-usebits;
		uint64_t const lookupmask = ::libmaus2::math::lowbits(usebits);
		lookup_type const rightalignedlookupword = (lookupword >> restbits) & lookupmask;
		int64_t const opening = static_cast<int64_t>(::libmaus2::rank::PopCnt8<sizeof(unsigned long)>::popcnt8(rightalignedlookupword));
		int64_t const closing = (usebits - opening);
		assert (closing > opening) ;
		
		// follow words until we find the correct one
		int64_t excess = opening - closing;
		i = lookupblock*lookup_bits;
		
		lookup_type nextlookupword = ::libmaus2::bitio::getBits(I,i-lookup_bits,lookup_bits);
		while ( (((-excess) > static_cast<int64_t>(lookup_bits))) || (wordRightMatch(nextlookupword,(-excess)-1) < 0) )
		{
			excess += excessLookup[nextlookupword];
			i -= lookup_bits;
			nextlookupword = ::libmaus2::bitio::getBits(I,i-lookup_bits,lookup_bits);
		}
		
		// reached word, return result
		return (i-lookup_bits) + wordRightMatch(nextlookupword,(-excess)-1);		
	}

	template<typename iterator>
	uint64_t lookupFindOpenOrFar(iterator I, uint64_t i, uint64_t const blocksize) const
	{
		// is par at position i opening?
		if ( ::libmaus2::bitio::getBit(I,i) )
			return i;

		// check word
		uint64_t const lookupblock = (i / lookup_bits);
		uint64_t const lookupoffset = i - lookupblock*lookup_bits;
		lookup_type const lookupword = ::libmaus2::bitio::getBits(I,lookupblock*lookup_bits,lookup_bits);
		int8_t const lookupres = wordOpen( lookupword, lookupoffset  );

		if ( lookupres != static_cast<int64_t>(lookupoffset) )
			return lookupblock*lookup_bits + lookupres;
		
		// compute excess on part of word
		unsigned int const usebits = lookupoffset+1;
		unsigned int const restbits = lookup_bits-usebits;
		uint64_t const lookupmask = ::libmaus2::math::lowbits(usebits);
		lookup_type const rightalignedlookupword = (lookupword >> restbits) & lookupmask;
		int64_t const opening = static_cast<int64_t>(::libmaus2::rank::PopCnt8<sizeof(unsigned long)>::popcnt8(rightalignedlookupword));
		int64_t const closing = (usebits - opening);
		assert (closing > opening) ;
		
		// follow words until we find the correct one
		int64_t excess = opening - closing;
		i = lookupblock*lookup_bits;
		
		if ( i%blocksize== 0)
			return std::numeric_limits<uint64_t>::max();
		
		lookup_type nextlookupword = ::libmaus2::bitio::getBits(I,i-lookup_bits,lookup_bits);
		while ( (((-excess) > static_cast<int64_t>(lookup_bits))) || (wordRightMatch(nextlookupword,(-excess)-1) < 0) )
		{
			excess += excessLookup[nextlookupword];
			i -= lookup_bits;

			if ( i%blocksize== 0)
				return std::numeric_limits<uint64_t>::max();

			nextlookupword = ::libmaus2::bitio::getBits(I,i-lookup_bits,lookup_bits);
		}
		
		// reached word, return result
		return (i-lookup_bits) + wordRightMatch(nextlookupword,(-excess)-1);		
	}

	template<typename iterator>
	static uint64_t naiveFindClose(iterator I, uint64_t i)
	{
		// closing?
		if ( ! ::libmaus2::bitio::getBit(I,i) )
			return i;

		uint64_t excess = 1;
		while ( excess )
		{
			bool const bit = ::libmaus2::bitio::getBit(I,++i);
			
			if ( bit )
				excess++;
			else
				excess--;
		}
		
		return i;
	}

	template<typename iterator>
	static uint64_t naiveFindOpen(iterator I, uint64_t i)
	{
		if ( ::libmaus2::bitio::getBit(I,i) )
			return i;
		
		int64_t excess = -1;
		while ( excess )
		{
			bool const bit = ::libmaus2::bitio::getBit(I,--i);
			
			if ( bit )
				excess++;
			else
				excess--;
		}
		
		return i;
	}

	template<typename value_type>
	static uint8_t singleFindClose(value_type const v, uint8_t const i)
	{
		uint8_t excess = 1;
		unsigned int offset = 1;
		while ( i+offset < 8*sizeof(v) )
		{
			value_type mask = 1ull << (8*sizeof(v) - i - offset - 1);
			// std::cerr << "i=" << static_cast<int>(i) << " offset=" << offset << " mask=" << std::hex << mask << std::dec << std::endl;
			
			if ( v & mask )
				excess++;
			else
				excess--;
				
			if ( ! excess )
				break;

			offset++;
		}
		
		if ( offset+i < 8*sizeof(v) )
			return offset + i;
		else
			return i;
	}

	template<typename value_type>
	static uint8_t singleFindOpen(value_type const v, uint8_t const i)
	{
		int8_t excess = -1;
		int offset = -1;
		while ( i+offset >= 0 )
		{
			value_type mask = 1ull << (8*sizeof(v) - i - offset - 1);
			// std::cerr << "i=" << static_cast<int>(i) << " offset=" << offset << " mask=" << std::hex << mask << std::dec << std::endl;

			if ( v & mask )
				excess++;
			else
				excess--;
				
			if ( ! excess )
				break;

			offset--;
		}
		
		if ( offset+i >= 0 )
			return offset+i;
		else
			return i;
	}

	template<typename value_type>
	static int8_t singleLeftExcess(value_type const v, uint8_t const i)
	{
		int8_t excess = 0;
		uint8_t offset = 0;
		while ( offset <= i )
		{
			value_type mask = 1ull << (8*sizeof(v) - offset - 1);
			// std::cerr << "i=" << static_cast<int>(i) << " offset=" << offset << " mask=" << std::hex << mask << std::dec << std::endl;
			
			if ( v & mask )
				excess++;
			else
				excess--;
				
			offset++;
		}
		
		return excess;
	}

	template<typename value_type>
	static int8_t singleRightExcess(value_type const v, uint8_t const i)
	{
		int8_t excess = 0;
		uint8_t offset = (8*sizeof(v)-1);
		while ( offset >= i )
		{
			value_type mask = 1ull << (8*sizeof(v) - offset - 1);
			// std::cerr << "i=" << static_cast<int>(i) << " offset=" << offset << " mask=" << std::hex << mask << std::dec << std::endl;
			
			if ( v & mask )
				excess++;
			else
				excess--;
				
			offset--;
		}
		
		return excess;
	}

	template<typename value_type>
	static uint8_t singleMatchExcessLeft(value_type const v, int8_t excess)
	{
		uint8_t offset = 0;
		while ( offset < 8*sizeof(v) )
		{
			value_type mask = 1ull << (8*sizeof(v) - offset - 1);
			// std::cerr << "i=" << static_cast<int>(i) << " offset=" << offset << " mask=" << std::hex << mask << std::dec << std::endl;
			
			if ( v & mask )
				excess++;
			else
				excess--;
				
			if ( ! excess )
				return offset;

			offset++;
		}
		
		return offset;
	}

	template<typename value_type>
	static int8_t singleMatchExcessRight(value_type const v, int8_t excess)
	{
		int8_t offset = 8*sizeof(v)-1;
		while ( offset >= 0 )
		{
			value_type mask = 1ull << (8*sizeof(v) - offset - 1);
			// std::cerr << "i=" << static_cast<int>(i) << " offset=" << offset << " mask=" << std::hex << mask << std::dec << std::endl;
			
			if ( v & mask )
				excess++;
			else
				excess--;
				
			if ( ! excess )
				return offset;

			offset--;
		}

		assert ( offset == -1 );
		return offset;
	}

	//! The code of the following function was imported from Simon Gogs SDSL Library
	//! Calculate pioneers as defined in the paper of Geary et al. (CPM 2004) with few extra space
	/*! \param bp The balanced parentheses sequence for that the pioneers should be calculated.
	 *  \param block_size Size of the blocks for which the pioneers should be calculated.
	 *  \param pioneer_bitmap Reference to the resulting bit_vector.
	 *  \par Time complexity
	 *       \f$ \Order{n} \f$, where \f$ n=\f$bp.size()  
	 *  \par Space complexity
	 *       \f$ \Order{2n + n} \f$ bits: \f$n\f$ bits for input, \f$n\f$ bits for output, and \f$n\f$ bits for a succinct stack.
	 *  \pre The parentheses sequence represented by bp has to be balanced.
	 */
	static ::libmaus2::bitio::IndexedBitVector::unique_ptr_type calculatePioneerBitVector(::libmaus2::bitio::BitVector const & bp, uint64_t const block_size)
	{
		::libmaus2::bitio::IndexedBitVector::unique_ptr_type Ppioneer_bitmap(new ::libmaus2::bitio::IndexedBitVector(bp.size()));
		::libmaus2::bitio::IndexedBitVector & pioneer_bitmap = *Ppioneer_bitmap;

		IncreasingStack opening_parenthesis(bp.size());

		uint64_t cur_pioneer_block = 0;
	 	uint64_t last_start = 0;
	 	uint64_t last_j = 0;
	 	uint64_t cur_block=0;
	 	uint64_t first_index_in_block=0;
	 	
	 	// calculate positions of findclose and findopen pioneers
		for(uint64_t j=0, new_block=block_size; j < bp.size(); ++j, --new_block)
		{
			if( !(new_block) )
			{
				cur_pioneer_block = j/block_size; 
				++cur_block;
				first_index_in_block = j;
				new_block = block_size;
			}

			// opening parenthesis
			if( bp[j] )
			{ 
				/*j < bp.size() is not neccecssary as the last parenthesis is always a closing one*/
				/* if closing par immediately follows opening, skip both and carry on */
				if( new_block>1 and !bp[j+1] )
				{
					++j;
					--new_block;
				}
				/* otherwise push opening par */
				else
				{
					opening_parenthesis.push(j);
				}
			}
			else
			{
				uint64_t const start = opening_parenthesis.top();
				opening_parenthesis.pop();
				// if start is not in this block (i.e. far parenthesis)
				if( start < first_index_in_block )
				{
					// same block as previous pioneer
					if( (start/block_size)==cur_pioneer_block  )
					{
						// erase previous pioneer
						pioneer_bitmap[last_start] = false;
						pioneer_bitmap[last_j] = false;
					}
					// set this pioneer
					pioneer_bitmap[start] = true;
					pioneer_bitmap[j] = true;
					cur_pioneer_block = start/block_size;
					last_start = start;
					last_j = j;
				}
			}
		}
		
		pioneer_bitmap.setupIndex();
		
		return UNIQUE_PTR_MOVE(Ppioneer_bitmap);
	}

	static libmaus2::util::NearestNeighbourDictionary::unique_ptr_type calculatePioneerBitVectorNND(::libmaus2::bitio::BitVector const & bp, uint64_t const block_size)
	{
		::libmaus2::bitio::IndexedBitVector::unique_ptr_type pion = calculatePioneerBitVector(bp,block_size);
		libmaus2::util::NearestNeighbourDictionary::unique_ptr_type ptr(new libmaus2::util::NearestNeighbourDictionary(*pion));	
		return UNIQUE_PTR_MOVE(ptr);
	}
	
	template<typename piovectype>
	static ::libmaus2::bitio::IndexedBitVector::unique_ptr_type extractPioneerFamily(
		::libmaus2::bitio::BitVector const & bp,
		piovectype const & pioneers
		)
	{
		uint64_t const numpioneers = pioneers.size() ? pioneers.rank1(pioneers.size()-1) : 0;
		::libmaus2::bitio::IndexedBitVector::unique_ptr_type Ppfamily ( new ::libmaus2::bitio::IndexedBitVector(numpioneers) );
		
		uint64_t prank = 0;
		for ( uint64_t i = 0; i < pioneers.size(); ++i )
			if ( pioneers[i] )
				(*Ppfamily)[prank++] = bp [ i ];
		assert ( prank == numpioneers );
		
		Ppfamily->setupIndex();
		
		int64_t excess = 0;
		for ( uint64_t i = 0; i < Ppfamily->size(); ++i )
		{
			if ( (*Ppfamily)[i] )
				excess++;
			else
				excess--;

			assert ( excess >= 0 );
		}
		assert ( excess == 0 );
		
		// std::cerr << "excess " << excess << std::endl;
		
		return UNIQUE_PTR_MOVE(Ppfamily);
	}
};
#endif

