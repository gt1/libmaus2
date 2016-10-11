/*
    libmaus2
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
#if !defined(BALANCEDPARENTHESES_HPP)
#define BALANCEDPARENTHESES_HPP

#include <libmaus2/bp/BalancedParenthesesBase.hpp>
#include <stack>

struct BalancedParentheses : public BalancedParenthesesBase
{
	static unsigned int const blocksize = 64;

	private:
	// balanced parentheses vector
	::libmaus2::bitio::IndexedBitVector::unique_ptr_type PUUB;
	::libmaus2::bitio::IndexedBitVector const & UUB;
	uint64_t const * UB;

	// first level pioneer vector
	// ::libmaus2::bitio::IndexedBitVector::unique_ptr_type const pion0;
	libmaus2::util::NearestNeighbourDictionary::unique_ptr_type pion0;
	// first level pioneer family bit vector
	::libmaus2::bitio::IndexedBitVector::unique_ptr_type const pio0;

	// second level pioneer vector
	// ::libmaus2::bitio::IndexedBitVector::unique_ptr_type const pion1;
	libmaus2::util::NearestNeighbourDictionary::unique_ptr_type pion1;
	// second level pioneer family bit vector
	::libmaus2::bitio::IndexedBitVector::unique_ptr_type const pio1;
	// lookup table for second level pioneer family
	::libmaus2::autoarray::AutoArray<uint64_t> const pio1lookup;

	uint64_t byteSize() const
	{
		return
			UUB.byteSize() +
			pion0->byteSize() +
			pio0->byteSize() +
			pion1->byteSize() +
			pio1->byteSize() +
			pio1lookup.byteSize();
	}

	::libmaus2::autoarray::AutoArray<uint64_t> computePio1Lookup() const
	{
		IncreasingStack IS(pio1->size());
		::libmaus2::autoarray::AutoArray<uint64_t> pio1lookup(pio1->size());
		for ( uint64_t i = 0; i < pio1->size(); ++i )
			if ( (*pio1)[i] )
			{
				IS.push(i);
			}
			else
			{
				uint64_t const j = IS.top();
				IS.pop();

				pio1lookup[j] = i;
				pio1lookup[i] = j;
			}

		return pio1lookup;
	}

	uint64_t findClosePio0(uint64_t const i) const
	{
		uint64_t const pio0rank = pion0->rank1(i)-1;

		// same block?
		uint64_t const lfo0 = lookupFindCloseOrFar(pio0->get(),pio0rank,blocksize);
		if ( lfo0 != std::numeric_limits<uint64_t>::max() )
			return pion0->select1(lfo0);

		// level 2 pioneer?
		if ( (*pion1)[pio0rank] )
		{
			uint64_t const pio1rank = pion1->rank1(pio0rank)-1;
			uint64_t const pio1close = pio1lookup[pio1rank];
			uint64_t const pio0close = pion1->select1(pio1close);
			uint64_t const rclose = pion0->select1(pio0close);
			return rclose;
		}

		// previous pioneer in level 2 index at level 1
		uint64_t const prev2 = pion1->prev1(pio0rank);
		// should be a pioneer
		// assert ( (*pion1)[prev2] );
		// should be opening par
		// assert ( (*pio0)[prev2] );
		// rank on level 2
		uint64_t const prev2index2 = pion1->rank1(prev2)-1;
		// should be opening
		// assert ( (*pio1)[prev2index2] );
		// difference in excess
		int64_t const prev2excessdiff = pio0->excess(pio0rank,prev2);
		// assert ( prev2excessdiff > 0 );

		// closing par on level 2
		uint64_t const close2 = pio1lookup[prev2index2];
		// closing pioneer on level 1
		uint64_t const close1 = pion1->select1(close2);

		// obtained pioneers should be closing
		// assert ( ! ((*pio1)[close2]) );
		// assert ( ! ((*pio0)[close1]) );

		// par is supposed to be behind actual one
		// assert ( close1 > lookupFindClose(pio0->get(),pio0rank) );

		uint64_t ci = close1;
		uint64_t minmatex = 0;
		while ( ci / blocksize == close1 / blocksize )
		{
			if ( - pio0->excess ( close1, ci ) == prev2excessdiff )
				minmatex = ci;
			ci--;
		}

		return pion0->select1(minmatex);
	}

	uint64_t findOpenPio0(uint64_t const i) const
	{
		uint64_t const pio0rank = pion0->rank1(i)-1;

		// same block?
		uint64_t const lfo0 = lookupFindOpenOrFar(pio0->get(),pio0rank,blocksize);
		if ( lfo0 != std::numeric_limits<uint64_t>::max() )
			return pion0->select1(lfo0);

		// level 2 pioneer?
		if ( (*pion1)[pio0rank] )
		{
			uint64_t const pio1rank = pion1->rank1(pio0rank)-1;
			uint64_t const pio1open = pio1lookup[pio1rank];
			uint64_t const pio0open = pion1->select1(pio1open);
			uint64_t const ropen = pion0->select1(pio0open);
			return ropen;
		}

		// next pioneer in level 2 index at level 1
		uint64_t const next2 = pion1->next1(pio0rank);
		// should be a pioneer
		// assert ( (*pion1)[next2] );
		// should be closing par
		// assert ( ! (*pio0)[next2] );

		// rank on level 2
		uint64_t const next2index2 = pion1->rank1(next2)-1;
		// should be closing
		// assert ( ! (*pio1)[next2index2] );
		// difference in excess
		int64_t const next2excessdiff = pio0->excess(next2,pio0rank)+1;
		// assert ( next2excessdiff-1 < 0 );

		// opening par on level 2
		uint64_t const open2 = pio1lookup[next2index2];
		// opening pioneer on level 1
		uint64_t const open1 = pion1->select1(open2);

		// obtained pioneers should be closing
		// assert ( ((*pio1)[open2]) );
		// assert ( ((*pio0)[open1]) );

		// par is supposed to be before actual one
		// assert ( open1 < lookupFindOpen(pio0->get(),pio0rank) );

		uint64_t ci = open1;
		uint64_t maxmatex = 0;
		while ( ci / blocksize == open1 / blocksize )
		{
			if ( pio0->excess ( open1, ci ) == next2excessdiff )
				maxmatex = ci;
			ci++;
		}

		return pion0->select1(maxmatex+1);
	}

	public:
	uint64_t findClose(uint64_t const i) const
	{
		// check whether result is in same block
		uint64_t const lfo = lookupFindCloseOrFar(UB,i,blocksize);
		if ( lfo != std::numeric_limits<uint64_t>::max() )
			return lfo;

		// level 1 pioneer par?
		if ( (*pion0)[i] )
			return findClosePio0(i);

		// previous pioneer
		uint64_t const prevpio = pion0->prev1(i);
		int64_t const prev1excessdiff = UUB.excess(i,prevpio);
		uint64_t const pioclosing = findClosePio0(prevpio);

		uint64_t ci = pioclosing;
		uint64_t minmatex = 0;
		while ( ci / blocksize == pioclosing / blocksize )
		{
			if ( - UUB.excess ( pioclosing, ci ) == prev1excessdiff )
				minmatex = ci;
			ci--;
		}

		return minmatex;
	}

	uint64_t findOpen(uint64_t const i) const
	{
		// check whether result is in same block
		uint64_t const lfo = lookupFindOpenOrFar(UB,i,blocksize);
		if ( lfo != std::numeric_limits<uint64_t>::max() )
			return lfo;

		// level 1 pioneer par?
		if ( (*pion0)[i] )
			return findOpenPio0(i);

		// uint64_t const expected = lookupFindOpen(UB,i);

		// next pioneer
		uint64_t const nextpio = pion0->next1(i);
		int64_t const next1excessdiff = UUB.excess(nextpio,i)+1;
		uint64_t const pioopening = findOpenPio0(nextpio);

		uint64_t ci = pioopening;
		uint64_t maxmatex = 0;
		while ( ci / blocksize == pioopening / blocksize )
		{
			if ( UUB.excess ( pioopening, ci ) == next1excessdiff )
				maxmatex = ci;
			ci++;
		}

		return maxmatex+1;
	}

	/*
	 * trivial (slow) implementation of enclose, should be reasonably quick for small number of children per node
	 * (i.e. small alphabet in suffix tree)
	 */
	uint64_t enclose(uint64_t i) const
	{
		// closing?
		if ( !UUB[i] )
			i = findOpen(i);

		// root of tree?
		if ( ! i )
			return 0;

		// i should be an opening par now
		assert ( UUB[i] );

		// move left among siblings until we reached a leftmost child node
		while ( !UUB[i-1] )
			i = findOpen(i-1);

		// now both i and i-1 should be opening pars and i-1 is the parent of i
		assert ( i );
		assert ( UUB[i] );
		assert ( UUB[i-1] );

		// return parent
		return i-1;
	}

	void checkScanFind() const
	{
		std::stack < uint64_t > S;
		IncreasingStack IS(UUB.size());
		for ( uint64_t i = 0; i < UUB.size(); ++i )
			if ( UUB[i] )
			{
				S.push(i);
				IS.push(i);
			}
			else
			{
				uint64_t const v0 =
					S.top();
				S.pop();
				#if ! defined(NDEBUG)
				uint64_t const v1 =
				#endif
					IS.top();
				IS.pop();

				#if ! defined(NDEBUG)
				assert ( v0 == v1 );
				#endif

				uint64_t const cl = lookupFindClose(UB,v0);

				if ( cl != i )
					std::cerr << "v0=" << v0 << " i=" << i << " cl=" << cl << std::endl;
				assert ( cl == i );

				uint64_t const ol = lookupFindOpen(UB, i);

				if ( ol != v0 )
				{
					std::cerr << "Fail." << std::endl;
				}

				assert ( ol == v0 );

				#if ! defined(NDEBUG)
				unsigned int const blocksize = 64;
				assert (
					lookupFindCloseOrFar(UB,v0,blocksize) == lookupFindClose(UB,v0)
					||
					(
						lookupFindClose(UB,v0)/blocksize != v0/blocksize
						&&
						lookupFindCloseOrFar(UB,v0,blocksize) == std::numeric_limits<uint64_t>::max()
					)
				);
				assert (
					lookupFindOpenOrFar(UB,i,blocksize) == lookupFindOpen(UB,i)
					||
					(
						lookupFindOpen(UB,i)/blocksize != i/blocksize
						&&
						lookupFindOpenOrFar(UB,i,blocksize) == std::numeric_limits<uint64_t>::max()
					)
				);
				#endif

				// std::cerr << "v0=" << v0 << " v1=" << v1 << std::endl;
			}
	}

	void printBPSizeInfo() const
	{
		std::cerr << "size " << (byteSize()*8.0)/UUB.size() << " bits per par" << std::endl;
		uint64_t const numpion = pion0->rank1(pion0->size()-1);
		std::cerr << "Number of pioneer parentheses: " << numpion << " number of parentheses: " << pion0->size() << std::endl;
		std::cerr << "Lenght of BP:" << UUB.size() << std::endl;
		std::cerr << "Length of pio0: " << pio0->size() << std::endl;
		std::cerr << "Length of pio1: " << pio1->size() << std::endl;
	}

	void checkOperations() const
	{
		//std::cerr << "Checking...";
		std::vector <uint64_t> parstack;
		for ( uint64_t i = 0; i < UUB.size(); ++i )
		{
			#if 1
			assert ( findClose(i) ==  lookupFindClose(UB,i) );
			assert ( findOpen(i) == lookupFindOpen(UB,i) );
			#endif

			if ( UUB[i] )
			{
				if ( parstack.size() )
					assert ( parstack.back() == enclose(i) );
				parstack.push_back(i);
			}
			else
			{
				parstack.pop_back();
				if ( parstack.size() )
					assert ( parstack.back() == enclose(i) );
			}

			#if 0
			par.push_back(findClose(i) );
			par.push_back(findOpen(i) );
			par.push_back( enclose(i) );
			#endif
			// std::cerr << "i=" << i << " enclose()="<< enclose(i) << std::endl;
		}
		//std::cerr << "done." << std::endl;
	}

	BalancedParentheses(::libmaus2::bitio::IndexedBitVector::unique_ptr_type & rPUUB)
	: PUUB(UNIQUE_PTR_MOVE(rPUUB)), UUB(*PUUB), UB(UUB.get()),
	  // first level pioneer data
	  pion0(BalancedParenthesesBase::calculatePioneerBitVectorNND(UUB,blocksize)),
	  // pion0(BalancedParenthesesBase::calculatePioneerBitVector(UUB, blocksize)),
	  pio0(BalancedParenthesesBase::extractPioneerFamily(UUB,*pion0)),
	  // second level pioneer data
	  pion1(BalancedParenthesesBase::calculatePioneerBitVectorNND(*pio0,blocksize)),
	  // pion1(BalancedParenthesesBase::calculatePioneerBitVector(*pio0, blocksize)),
	  pio1(BalancedParenthesesBase::extractPioneerFamily(*pio0,*pion1)),
	  pio1lookup(computePio1Lookup())
	{
	}
};
#endif
