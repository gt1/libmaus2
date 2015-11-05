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

#if ! defined(DYNAMICWAVELETTREE_HPP)
#define DYNAMICWAVELETTREE_HPP

#include <sstream>
#include <limits>
#include <libmaus2/bitbtree/bitbtree.hpp>

#if defined(_MSC_VER) || defined(__MINGW32__)
#include <windows.h>
#include <malloc.h>
#endif

namespace libmaus2
{
	namespace wavelet
	{
		template<unsigned int _k, unsigned int _w>
		struct DynamicWaveletTree
		{
			static unsigned int const k = _k;
			static unsigned int const w = _w;
			typedef DynamicWaveletTree<k,w> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			typename ::libmaus2::bitbtree::BitBTree<k,w>::unique_ptr_type R;
			uint64_t const b;
			uint64_t n;

			bool identical(this_type const & O) const
			{
				return
					R->identical(*(O.R)) && b == O.b && n == O.n;
			}

			unique_ptr_type clone() const
			{
				unique_ptr_type C(new DynamicWaveletTree<k,w>(b));
				C->R = R->clone();
				C->n = n;
				return UNIQUE_PTR_MOVE(C);
			}

			DynamicWaveletTree(uint64_t const rb)
			: R(new ::libmaus2::bitbtree::BitBTree<k,w>() ), b(rb), n(0)
			{

			}

			void serialise(std::ostream & out) const
			{
				R->serialise(out);
				libmaus2::util::NumberSerialisation::serialiseNumber(out,b);
				libmaus2::util::NumberSerialisation::serialiseNumber(out,n);
			}

			static typename ::libmaus2::bitbtree::BitBTree<k,w>::unique_ptr_type loadBitBTree(std::istream & in)
			{
				typename ::libmaus2::bitbtree::BitBTree<k,w>::unique_ptr_type Ptree(new ::libmaus2::bitbtree::BitBTree<k,w>);
				Ptree->deserialise(in);
				return UNIQUE_PTR_MOVE(Ptree);
			}

			DynamicWaveletTree(std::istream & in)
			: R(loadBitBTree(in)), b(libmaus2::util::NumberSerialisation::deserialiseNumber(in)), n(libmaus2::util::NumberSerialisation::deserialiseNumber(in))
			{

			}

			uint64_t size() const
			{
				return n;
			}

			void insert(uint64_t const key, uint64_t p)
			{
				assert ( p <= n );
				assert ( b );

				uint64_t m = (1ull << (b-1));

				uint64_t left = 0;
				uint64_t right = n;

				uint64_t ib = 0;

				while ( ib < b-1 )
				{
					bool const bit = key & m;
					R->insertBit( ib * (n+1) + left + p, bit );

					if ( bit )
					{
						uint64_t const b0 = (ib*(n+1) + left) ? R->rank0((ib*(n+1) + left)-1) : 0;
						uint64_t const i0 = R->rank0((ib*(n+1) + right));
						uint64_t const o0 = R->rank0((ib*(n+1) + left + p));
						left += (i0-b0);
						p -= (o0-b0);
					}
					else
					{
						uint64_t const b1 = (ib*(n+1) + left) ? R->rank1((ib*(n+1) + left)-1) : 0;
						uint64_t const i1 = R->rank1((ib*(n+1) + right));
						uint64_t const o1 = R->rank1((ib*(n+1) + left + p));
						right -= (i1-b1);
						p -= (o1-b1);
					}

					++ib;
					m>>=1;
				}

				bool const bit = key & m;
				R->insertBit( (b-1) * (n+1) + left + p, bit );

				n++;
			}

			uint64_t insertAndRank(uint64_t const key, uint64_t p)
			{
				assert ( p <= n );
				assert ( b );

				uint64_t m = (1ull << (b-1));

				uint64_t left = 0;
				uint64_t right = n;

				uint64_t ib = 0;

				while ( ib < b-1 )
				{
					bool const bit = key & m;
					R->insertBit( ib * (n+1) + left + p, bit );

					if ( bit )
					{
						uint64_t const b0 = (ib*(n+1) + left) ? R->rank0((ib*(n+1) + left)-1) : 0;
						uint64_t const i0 = R->rank0((ib*(n+1) + right));
						uint64_t const o0 = R->rank0((ib*(n+1) + left + p));
						left += (i0-b0);
						p -= (o0-b0);
					}
					else
					{
						uint64_t const b1 = (ib*(n+1) + left) ? R->rank1((ib*(n+1) + left)-1) : 0;
						uint64_t const i1 = R->rank1((ib*(n+1) + right));
						uint64_t const o1 = R->rank1((ib*(n+1) + left + p));
						right -= (i1-b1);
						p -= (o1-b1);
					}

					++ib;
					m>>=1;
				}

				bool const bit = key & m;
				R->insertBit( (b-1) * (n+1) + left + p, bit );

				n++;

				if ( bit )
				{
					// number of one bits up to node
					uint64_t const b1 = ((b-1)*n + left) ? R->rank1( ((b-1)*n + left)-1) : 0;
					uint64_t const o1 = R->rank1( (b-1)*n + left + p);
					return o1-b1;
				}
				else
				{
					// number of one bits up to node
					uint64_t const b0 = ((b-1)*n + left) ? R->rank0( ((b-1)*n + left)-1) : 0;
					uint64_t const o0 = R->rank0( (b-1)*n + left + p );
					return o0-b0;
				}
			}

			void remove(uint64_t p)
			{
				// std::cerr << "Removing p=" << p << std::endl;

				assert ( p <= n );

				uint64_t m = (1ull << (b-1));

				uint64_t left = 0;
				uint64_t right = n;

				for ( uint64_t ib = 0; ib < b; ++ib, m>>=1 )
				{
					// std::cerr << "left=" << left << " right=" << right << " p=" << p << std::endl;

					uint64_t bitpos = ib*(n-1)+left+p;
					bool const bit = (*R)[bitpos];

					if ( bit )
					{
						uint64_t b0 = (ib*(n-1) + left) ? R->rank0((ib*(n-1) + left)-1) : 0;
						uint64_t i0 = (ib*(n-1) + right) ? R->rank0((ib*(n-1) + right)-1) : 0;
						// zero bits up to position
						uint64_t o0 = (ib*(n-1) + left + p) ? R->rank0((ib*(n-1) + left + p)-1) : 0;
						left += (i0-b0);
						p -= (o0-b0);
					}
					else
					{
						uint64_t b1 = (ib*(n-1) + left) ? R->rank1((ib*(n-1) + left)-1) : 0;
						uint64_t i1 = (ib*(n-1) + right) ? R->rank1((ib*(n-1) + right)-1) : 0;
						uint64_t o1 = (ib*(n-1) + left + p) ? R->rank1((ib*(n-1) + left + p)-1) : 0;
						right -= (i1-b1);
						p -= (o1-b1);
					}

					R->deleteBit( bitpos );
				}

				n -= 1;
			}

			uint64_t operator[](uint64_t p) const
			{
				assert ( p <= n );

				uint64_t m = (1ull << (b-1));

				uint64_t left = 0;
				uint64_t right = n;

				uint64_t key = 0;
				for ( uint64_t ib = 0; ib < b; ++ib, m>>=1 )
				{
					bool const bit = (*R)[ib*n+left+p];

					// std::cerr << "left=" << left << " right=" << right << " p=" << p << " bit=" << bit << std::endl;

					// one bit
					if ( bit )
					{
						key |= m;

						// zero bits before node
						uint64_t b0 = (ib*n + left) ? R->rank0((ib*n + left)-1) : 0;
						// zero bits up to end of node
						uint64_t i0 = (ib*n + right) ? R->rank0((ib*n + right)-1) : 0;

						// # 1 bits before node
						/*
						uint64_t b1 = (ib*n + left) ? R->rank1((ib*n + left)-1) : 0;
						*/

						// zero bits up to position
						uint64_t o0 = (ib*n + left + p) ? R->rank0((ib*n + left + p)-1) : 0;

						left += (i0-b0);
						p -= (o0-b0);
					}
					// zero bit
					else
					{
						uint64_t b1 = (ib*n + left) ? R->rank1((ib*n + left)-1) : 0;
						uint64_t i1 = (ib*n + right) ? R->rank1((ib*n + right)-1) : 0;
						uint64_t o1 = (ib*n + left + p) ? R->rank1((ib*n + left + p)-1) : 0;

						right -= (i1-b1);
						p -= (o1-b1);
					}
				}

				return key;
			}

			/**
			 * return number of indices j <= i such that a[j] == k, i.e. the rank of index i concerning symbol k
			 *
			 * @param k
			 * @param i
			 * @return rank of index i concerning symbol k
			 **/
			uint64_t rank(uint64_t const key, uint64_t i) const
			{
				assert ( i < n );

				// check whether value k requires too many bits (i.e. is too large)
				if ( key & (~((1u<<b) - 1)) )
					return 0;

				uint64_t left = 0, right = n, offset = 0, m = (1u<<(b-1));

				for ( uint64_t ib = 0; ib < b; ++ib, m>>=1, offset += n )
				{
					// number of 0 bits up to node
					uint64_t const o0 = (offset+left) ? R->rank0(offset+left-1) : 0;
					// number of 1 bits up to node
					uint64_t const o1 = (offset+left) ? R->rank1(offset+left-1) : 0;

					if ( key & m )
					{
						// number of 1 bits in node up to position i
						uint64_t const n1 = R->rank1(offset+left+i) - o1;

						// no symbols
						if ( ! n1 )
							return 0;

						// total number of 0 bits in node
						uint64_t const n0 = R->rank0(offset+right-1) - o0;

						// adjust left, right, i
						left = left + n0;
						i = n1 - 1;
					}
					else
					{
						// number of 0 bits in node up to position i
						uint64_t const n0 = R->rank0(offset+left+i) - o0;

						// no symbols
						if ( ! n0 )
							return 0;

						// total number of 1 bits in node
						uint64_t const n1 = R->rank1(offset+right-1) - o1;

						// adjust left, right, i
						right = right - n1;
						i = n0 - 1;
					}
				}

				return i+1;
			}
			/**
			 * return the index of the i'th occurence of k, or std::numeric_limits<uint64_t>::max(), if no such index exists
			 *
			 * @param key
			 * @param i
			 * @return index of the i'th occurence of key
			 **/
			uint64_t select(uint64_t const key, uint64_t i) const
			{
				// check whether value key requires too many bits (i.e. is too large)
				if ( key & (~((1u<<b) - 1)) )
					return std::numeric_limits<uint64_t>::max();

				uint64_t left = 0, right = n, offset = 0, m = (1u<<(b-1));

				// stack for b = log( Sigma ) numbers allocated on the runtime stack
		#if defined(_MSC_VER) || defined(__MINGW32__)
				uint64_t * S = reinterpret_cast<uint64_t *>(_alloca( b * sizeof(uint64_t) ));
		#else
				uint64_t * S = reinterpret_cast<uint64_t *>(alloca( b * sizeof(uint64_t) ));
		#endif

				// go downward
				for ( uint64_t ib = 0; ib < b; ++ib, m>>=1, offset += n )
				{
					*(S++) = left;

					if ( key & m )
						// adjust left
						left = left + R->rank0(offset+right-1) - ((offset+left) ? R->rank0(offset+left-1) : 0);
					else
						// adjust right
						right = right - (R->rank1(offset+right-1) - ((offset+left) ? R->rank1(offset+left-1) : 0));

					// interval too small?
					if ( i >= ( right - left ) )
						return std::numeric_limits<uint64_t>::max();
				}

				// go upwards
				m = 1;
				offset -= n;

				for ( uint64_t ib = 0; ib < b; ++ib, m <<= 1, offset -= n )
				{
					left = *(--S);

					if ( key & m )
						i = R->select1( ((offset + left) ? R->rank1(offset+left-1) : 0) + i ) - (offset+left);
					else
						i = R->select0( ((offset + left) ? R->rank0(offset+left-1) : 0) + i ) - (offset+left);

					// std::cerr << "key " << k << " left=" << left << " right=" << right << " m=" << m << " i=" << i << std::endl;
				}

				return i;
			}

		};

		template<unsigned int k, unsigned int w>
		std::ostream & operator<<(std::ostream & out, DynamicWaveletTree<k,w> const & DWT)
		{
			#if 0
			out << "DynamicWaveletTree<"<<k<<"," << w << ">(";
			#endif
			for ( uint64_t i = 0; i < DWT.n; ++i )
				out << DWT[i];
			#if 0
			out << ")";
			#endif
			return out;
		}
	}
}
#endif
