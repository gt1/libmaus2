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

#include <libmaus2/bitbtree/bitbtree.hpp>
#include <libmaus2/util/BitList.hpp>
#include <libmaus2/util/VarBitList.hpp>

template<unsigned int k, unsigned int w>
int testBitBTreeShort()
{
	::libmaus2::bitbtree::BitBTree<k,w> B;
	::libmaus2::util::VarBitList L;

	for ( uint64_t i = 0; i < B.getN(); ++i )
		std::cerr << B[i] << std::endl;

	B.insertBit(/*pos*/0,1);
	L.insertBit(/*pos*/0,1);

	for ( unsigned int i = 0; i < 62; ++i )
	{
		B.insertBit(/*pos*/0,0);
		L.insertBit(/*pos*/0,0);
	}
	B.insertBit(63,1);
	L.insertBit(63,1);

	B.insertBit(/*pos*/0,0);
	L.insertBit(/*pos*/0,0);
	B.insertBit(/*pos*/15,1);
	L.insertBit(/*pos*/15,1);
	B.insertBit(/*pos*/40,1);
	L.insertBit(/*pos*/40,1);

	for ( unsigned int i = 0; i < 300; ++i )
	{
		B.insertBit(15,1);
		L.insertBit(15,1);
	}

	for ( uint64_t i = 0; i < 5000; ++i )
	{
		typename ::libmaus2::bitbtree::BitBTree<k,w>::unique_ptr_type cloned(B.clone());

		uint64_t const pos = rand() % (B.getN()+1);
		// uint64_t const pos = B.getN();
		bool const b = rand() % 2;

		std::cerr << "n=" << B.getN() << " bitsize=" << B.bitSize() << " pos=" << pos << std::endl;

		#if 0
		for ( unsigned int j = 0; j < B.getN(); ++j )
		{
			// std::cerr << B.rank1(j) << " " << L.rank1(j) << std::endl;
			assert ( B.rank1(j) == L.rank1(j) );
		}
		#endif

		try
		{
			B.insertBit(pos,b);
			L.insertBit(pos,b);

			std::ostringstream Bostr; Bostr << B;
			std::ostringstream Lostr; Lostr << L;
			assert ( Bostr.str() == Lostr.str() );
		}
		catch(...)
		{
			cloned->toString(std::cerr);
			std::cerr << std::endl;

			std::cerr << "----" << std::endl;

			B.toString(std::cerr);
			std::cerr << std::endl;

			return EXIT_FAILURE;
		}
	}

	std::cerr << "B " << B << std::endl;
	std::cerr << "L " << L << std::endl;

	std::ostringstream Bostr; Bostr << B;
	std::ostringstream Lostr; Lostr << L;
	assert ( Bostr.str() == Lostr.str() );

	return EXIT_SUCCESS;
}

#define BITTREE_DEBUG

template<unsigned int k, unsigned int w>
int testBitBTreeFunctionalityInsertDelete()
{
	::libmaus2::bitbtree::BitBTree<k,w> B;
	#if defined(BITTREE_DEBUG)
	::libmaus2::util::VarBitList L;
	#endif

	// srand(time(0));
	srand(5);

	uint64_t c = 0;
	uint64_t const runs = 512;

	for ( unsigned int z = 0; z < runs; ++z )
	{
		// B.toString(std::cerr);

#if defined(BITTREE_DEBUG)
#if defined(_OPENMP)
#pragma omp parallel for
#endif
		for ( int j = 0; j < static_cast<int>(B.getN()); ++j )
		{
			// std::cerr << B.rank1(j) << " " << L.rank1(j) << std::endl;
			assert ( B.rank1(j) == L.rank1(j) );
		}
		// std::cerr << ".";
#endif

#if defined(BITTREE_DEBUG)
#if defined(_OPENMP)
#pragma omp parallel for
#endif
		for ( int j = 0; j < static_cast<int>(B.root_bcnt); ++j )
		{
			bool const ok = B.select1(j) == L.select1(j);

			if ( ! ok )
			{
				std::cerr << "j=" << j
					<< " B.select="
					<< B.select1(j)
					<< " L.select="
					<< L.select1(j)
					<< std::endl;

				#if 0
				for ( unsigned int z = 0; z <= L.select1(j); ++z )
					std::cerr << B[z];
				std::cerr << std::endl;
				#endif

				// std::cerr << B.select1(j) << " " << L.select1(j) << std::endl;
				assert ( ok );
			}
		}
		// std::cerr << ".";
#endif

#if defined(BITTREE_DEBUG)
#if defined(_OPENMP)
#pragma omp parallel for
#endif
		for ( int j = 0; j < static_cast<int>(B.root_cnt - B.root_bcnt); ++j )
		{
			// std::cerr << B.select0(j) << " " << L.select0(j) << std::endl;
			assert ( B.select0(j) == L.select0(j) );
		}
		// std::cerr << ".";
#endif

#if defined(BITTREE_DEBUG)
		//if ( (++c & (1024-1)) == 0 )
		if ( (++c & (128-1)) == 0 )
#else
		if ( (++c & (16*1024-1)) == 0 )
#endif
			std::cerr << "\r                               \r" << c << "/" << runs << "\t" << B.getN() << "\t" << B.bitSize();

		typename ::libmaus2::bitbtree::BitBTree<k,w>::unique_ptr_type clone(B.clone());

		bool b = rand() % 2;
		uint64_t p = rand() % (B.getN()+1);

		try
		{
			// if ( B.innernodes->allocatedNodes-B.innernodes->nodeFreeList.size() < 30 )
			if ( B.innernodes->blocks.size()*B.innernodes->blocksize-B.innernodes->nodeFreeList.size() < 30 )
			{
				switch ( rand() % 4 )
				{

					case 0:
					case 1:
						{
						B.insertBit(p,b);
#if defined(BITTREE_DEBUG)
						L.insertBit(p,b);
#endif
						}
						break;
					case 2:
						if ( B.getN() )
						{
							p = rand() % (B.getN());
							B.deleteBit(p);
#if defined(BITTREE_DEBUG)
							L.deleteBit(p);
#endif
						}
						break;
				}
			}
			else
			{
				if ( B.getN() )
				{
					uint64_t p = rand() % (B.getN());
					B.deleteBit(p);
#if defined(BITTREE_DEBUG)
					L.deleteBit(p);
#endif
				}
			}

#if defined(BITTREE_DEBUG)
			std::ostringstream Bostr; Bostr << B;
			std::ostringstream Lostr; Lostr << L;
			assert ( Bostr.str() == Lostr.str() );
#endif

			// std::cerr << "---" << std::endl;
		}
		catch(std::exception const & ex)
		{
			std::cerr << ex.what() << std::endl;
			std::cerr << "Failed to erase bit " << p << std::endl;
			clone->toString(std::cerr);
			std::cerr << "\n\n****************************\n\n" << std::endl;
			B.toString(std::cerr);
			return EXIT_FAILURE;
		}
	}

	std::cerr << std::endl;

	return EXIT_SUCCESS;
}

int main()
{
	unsigned int const k = 1;
	unsigned int const w = 1;

	for ( uint64_t i = 0; i < 128; ++i )
	{
		::libmaus2::uint::UInt<2> U;
		U.setBit(i,true);
		assert ( U.getBit(i) );
		assert ( U.rank1(i) == 1 );
		assert ( U.select1(0) == i );
		std::cerr << U << std::endl;
	}

	std::cerr << "---" << std::endl;

	uint64_t n = 128;
	::libmaus2::bitbtree::BitBTree<k,w> B(n,false);

	std::cerr << B << std::endl;

	B.setBitQuick(5,true);
	B.setBitQuick(8,true);
	B.setBitQuick(9,true);
	B.setBitQuick(5,false);
	B.setBitQuick(127,true);
	B.setBitQuick(126,false);

	std::cerr << B << std::endl;
	for ( uint64_t i = 0; i < B.size(); ++i )
		std::cerr << B.rank1(i);
	std::cerr << std::endl;
	std::cerr << B.count1() << std::endl;
	B.toString(std::cerr);

	int r = testBitBTreeShort<k,w>();

	if ( r != EXIT_SUCCESS )
		return EXIT_FAILURE;

	r = testBitBTreeFunctionalityInsertDelete<k,w>();

	if ( r != EXIT_SUCCESS )
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}
