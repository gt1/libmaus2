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


#if ! defined(BITVECTOR_HPP)
#define BITVECTOR_HPP

#include <libmaus2/bitio/getBit.hpp>
#include <libmaus2/bitio/putBit.hpp>
#include <libmaus2/bitio/getBits.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/rank/ERank222B.hpp>
#include <libmaus2/rank/ERank3C.hpp>
#include <libmaus2/util/iterator.hpp>
#include <libmaus2/random/Random.hpp>
#include <libmaus2/util/NumberSerialisation.hpp>

namespace libmaus2
{
	namespace bitio
	{
		template<typename _data_type>
		struct BitVectorTemplate
		{
			typedef _data_type data_type;
			typedef BitVectorTemplate<data_type> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			static unsigned int const bitsperword = 8*sizeof(data_type);
			static unsigned int const bitsperwordshift = ::libmaus2::math::MetaLog2<bitsperword>::log;

			uint64_t n;
			::libmaus2::autoarray::AutoArray<_data_type> A;

			typedef ::libmaus2::util::AssignmentProxy<this_type,bool> BitVectorProxy;

			uint64_t byteSize() const
			{
				return sizeof(uint64_t) + A.byteSize();
			}

			static ::libmaus2::autoarray::AutoArray<data_type> deserialiseArray(std::istream & in)
			{
				::libmaus2::autoarray::AutoArray<data_type> A(in);
				return A;
			}

			std::pair<data_type *, unsigned int> getWord(uint64_t const i)
			{
				uint64_t const word = i / (8*sizeof(data_type));
				uint64_t const bit = i - word*(8*sizeof(data_type));
				return std::pair<data_type *,unsigned int>(A.get()+word,bit);
			}

			void serialise(std::ostream & out) const
			{
				::libmaus2::util::NumberSerialisation::serialiseNumber(out,n);
				A.serialize(out);
			}

			BitVectorTemplate(std::istream & in)
			: n(::libmaus2::util::NumberSerialisation::deserialiseNumber(in)), A(in)
			{

			}

			static uint64_t getW(uint64_t const rn, uint64_t const pad = 0)
			{
				uint64_t const w = (rn+bitsperword-1)/bitsperword + pad;
				return w;
			}

			BitVectorTemplate(uint64_t const rn, uint64_t const pad = 0) : n(rn), A( getW(rn,pad) ) {}

			void setup(uint64_t const rn, uint64_t const pad = 0)
			{
				n = rn;
				A.release();
				A = ::libmaus2::autoarray::AutoArray<_data_type>(getW(rn,pad));
			}

			void ensureSize(uint64_t const rn, uint64_t const pad = 0)
			{
				if ( A.size() < getW(rn,pad) )
					setup(rn,pad);
			}

			virtual ~BitVectorTemplate() {}

			data_type const * get() const
			{
				return A.get();
			}

			bool get(uint64_t const i) const
			{
				return ::libmaus2::bitio::getBit(A.get(),i);
			}
			bool operator[](uint64_t const i) const
			{
				return get(i);
			}
			void set(uint64_t const i, bool b)
			{
				::libmaus2::bitio::putBit(A.get(),i,b);
			}
			void setSync(uint64_t const i, bool b)
			{
				#if defined(LIBMAUS2_HAVE_SYNC_OPS)
				::libmaus2::bitio::putBitSync(A.get(),i,b);
				#else
				::libmaus2::exception::LibMausException se;
				se.getStream() << "BitVector::setSync() called, but compiled binary does not support sync ops." << std::endl;
				se.finish();
				throw se;
				#endif
			}
			bool setSync(uint64_t const i)
			{
				#if defined(LIBMAUS2_HAVE_SYNC_OPS)
				return ::libmaus2::bitio::setBitSync(A.get(),i);
				#else
				::libmaus2::exception::LibMausException se;
				se.getStream() << "BitVector::setSync() called, but compiled binary does not support sync ops." << std::endl;
				se.finish();
				throw se;
				#endif
			}
			bool eraseSync(uint64_t const i)
			{
				#if defined(LIBMAUS2_HAVE_SYNC_OPS)
				return ::libmaus2::bitio::eraseBitSync(A.get(),i);
				#else
				::libmaus2::exception::LibMausException se;
				se.getStream() << "BitVector::setSync() called, but compiled binary does not support sync ops." << std::endl;
				se.finish();
				throw se;
				#endif
			}
			BitVectorProxy operator[](uint64_t const i)
			{
				return BitVectorProxy(this,i);
			}
			uint64_t size() const
			{
				return n;
			}

			uint64_t next1(uint64_t i) const
			{
				return next1(A.get(),i);
			}

			static uint64_t next1(uint64_t const * A, uint64_t i)
			{
				uint64_t word = (i >> bitsperwordshift);
				unsigned int const imask = i&(bitsperword-1);
				uint64_t v = (A[word] << (imask));
				uint64_t add = bitsperword-imask;

				while ( ! v )
				{
					i += add;
					v = A[++word];
					add = bitsperword;
				}

				return i + __builtin_clzll(v << (64-bitsperword));
			}

			uint64_t next1slow(uint64_t i) const
			{
				while ( ! get(i) )
					i++;

				return i;
			}
			uint64_t prev1slow(uint64_t i) const
			{
				while ( ! get(i) )
					--i;
				return i;
			}
			uint64_t prev1(uint64_t i) const
			{
				uint64_t word = (i>>bitsperwordshift);
				unsigned int imask= i&(bitsperword-1);
				uint64_t v = (A[word] >> ((bitsperword-1)-imask));
				unsigned int add = imask+1;

				while ( ! v )
				{
					i -= add;
					v = A[--word];
					add = bitsperword;
				}

				return i - __builtin_ctzll(v << (64-bitsperword));
			}

			static int lsbSlow(uint64_t bb)
			{
				uint64_t mask = 1ull;
				int i = 0;
				while ( ! (bb&mask) )
				{
					mask <<= 1;
					i++;
				}

				return i;
			}

			/**
			 * imported from http://chessprogramming.wikispaces.com/BitScan
			 * published under the Creative Commons Attribution Share-Alike 3.0 License
			 * bitScanForward
			 * @author Matt Taylor
			 * @param bb bitboard to scan
			 * @precondition bb != 0
			 * @return index (0..63) of least significant one bit
			 */
			static int lsb(uint64_t bb)
			{
				static const int lsb_64_table[64] =
				{
					63, 30,  3, 32, 59, 14, 11, 33,
					60, 24, 50,  9, 55, 19, 21, 34,
					61, 29,  2, 53, 51, 23, 41, 18,
					56, 28,  1, 43, 46, 27,  0, 35,
					62, 31, 58,  4,  5, 49, 54,  6,
					15, 52, 12, 40,  7, 42, 45, 16,
					25, 57, 48, 13, 10, 39,  8, 44,
					20, 47, 38, 22, 17, 37, 36, 26
				};

				assert (bb != 0);
				bb ^= bb - 1;
				uint32_t const folded = static_cast<uint32_t>(bb) ^ static_cast<uint32_t>(bb >> 32);
				return lsb_64_table[folded * 0x78291ACF >> 26];
			}

			static void testLSB()
			{
				srand(time(0));
				for ( uint64_t i = 0; i < 1ull<<18; ++i )
				{
					uint64_t const v = ::libmaus2::random::Random::rand64();
					assert ( lsbSlow(v) == lsb(v) );
				}
				for ( uint64_t i = 1; i < 1ull << 34; ++i )
					assert ( lsbSlow(i) == lsb(i) );

			}
		};

		typedef BitVectorTemplate<uint64_t> BitVector8;
		typedef BitVectorTemplate<uint16_t> BitVector2;
		typedef BitVector8 BitVector;

		inline std::ostream & operator<<(std::ostream & out, BitVector const & B)
		{
			for ( uint64_t i = 0; i < B.size(); ++i )
				out << static_cast<unsigned int>(B[i]);
			return out;

		}

		struct IndexedBitVector : public BitVector
		{
			typedef IndexedBitVector this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			typedef ::libmaus2::rank::ERank222B rank_type;
			typedef rank_type::unique_ptr_type rank_ptr_type;

			rank_ptr_type index;

			IndexedBitVector(uint64_t const n, uint64_t const pad = 0) : BitVector(n,pad), index() {}
			IndexedBitVector(std::istream & in) : BitVector(in), index() {}

			void setupIndex()
			{
				rank_ptr_type tindex(new rank_type(A.get(),A.size()*64));
				index = UNIQUE_PTR_MOVE(tindex);
			}

			uint64_t rank1(uint64_t const i) const
			{
				return index->rank1(i);
			}

			uint64_t rankm1(uint64_t const i) const
			{
				return index->rankm1(i);
			}

			uint64_t rank0(uint64_t const i) const
			{
				return index->rank0(i);
			}

			uint64_t excess(uint64_t const i) const
			{
				assert ( rank1(i) >= rank0(i) );
				return rank1(i)-rank0(i);
			}
			int64_t excess(uint64_t const i, uint64_t const j) const
			{
				return static_cast<int64_t>(excess(i)) - static_cast<int64_t>(excess(j));
			}

			uint64_t select1(uint64_t const i) const
			{
				return index->select1(i);
			}

			uint64_t select0(uint64_t const i) const
			{
				return index->select0(i);
			}

			uint64_t byteSize() const
			{
				return
					BitVector::byteSize() +
					(index.get() ? index->byteSize() : 0);
			}
		};

		struct IndexedBitVectorCompressed : public BitVector2
		{
			typedef IndexedBitVectorCompressed this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			typedef ::libmaus2::rank::ERank3C rank_type;
			typedef rank_type::unique_ptr_type rank_ptr_type;

			rank_ptr_type index;

			IndexedBitVectorCompressed(uint64_t const n, uint64_t const pad = 0) : BitVector2(n,pad), index() {}

			void setupIndex()
			{
				rank_ptr_type tindex(new rank_type(A.get(),A.size()*64));
				index = UNIQUE_PTR_MOVE(tindex);
			}

			uint64_t rank1(uint64_t const i) const
			{
				return index->rank1(i);
			}

			uint64_t rank0(uint64_t const i) const
			{
				return index->rank0(i);
			}

			uint64_t excess(uint64_t const i) const
			{
				assert ( rank1(i) >= rank0(i) );
				return rank1(i)-rank0(i);
			}
			int64_t excess(uint64_t const i, uint64_t const j) const
			{
				return static_cast<int64_t>(excess(i)) - static_cast<int64_t>(excess(j));
			}

			uint64_t select1(uint64_t const i) const
			{
				return index->select1(i);
			}

			uint64_t select0(uint64_t const i) const
			{
				return index->select0(i);
			}

			uint64_t byteSize() const
			{
				return
					BitVector2::byteSize() +
					(index.get() ? index->byteSize() : 0);
			}
		};

	}
}
#endif
