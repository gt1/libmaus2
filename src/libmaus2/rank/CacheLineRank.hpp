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

#if ! defined(CACHELINERANK_HPP)
#define CACHELINERANK_HPP

#include <libmaus2/types/types.hpp>
#include <libmaus2/rank/ERankBase.hpp>
#include <libmaus2/bitio/getBit.hpp>

namespace libmaus2
{
	namespace rank
	{
		template<unsigned int blocksize = 8>
		struct CacheLineRankTemplate : ::libmaus2::rank::ERankBase
		{
			typedef CacheLineRankTemplate<blocksize> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			uint64_t const n;
			uint64_t const datawords;
			uint64_t const indexwords;
			::libmaus2::autoarray::AutoArray<uint64_t, ::libmaus2::autoarray::alloc_type_memalign_cacheline> A;

			struct WriteContext
			{
				uint64_t bitpos;
				uint64_t w;
				uint64_t s;
				uint64_t sh;
				uint64_t * p;
				uint64_t * ps;

				WriteContext(uint64_t * rp)
				: bitpos(0), w(0), s(0), sh(0), p(rp), ps(p)
				{}

				void writeBit(bool const rb)
				{
					// std::cerr << "bitpos=" << bitpos << std::endl;

					// save sum if new block
					if ( bitpos == 0 )
					{
						ps = p;
						if ( blocksize == 8 )
							*(p++)=(s << 16);
						else
							*(p++)=s;
					}

					uint64_t const b = rb ? 1 : 0;
					w <<= 1;
					w |=  b;

					bitpos++;
					s += b;

					if ( (bitpos & 63) == 0 )
					{
						// save word if full
						*(p++) = w;
						// erase word
						// w = 0;
						// save half count
						if ( (blocksize == 8) && (bitpos == (blocksize/2)*64) )
						{
							uint64_t const halfs = s - ((*ps)>>16);
							assert ( halfs < (1ull<<16) );
							(*ps) |= halfs;
						}
						// if ( (bitpos % ((blocksize-1)*64)) == 0 )
						if ( bitpos == ((blocksize-1)*64) )
						{
							bitpos = 0;
						}
					}

				}
				void flush()
				{
					// finish word
					while ( bitpos & 63 )
						writeBit(0);
				}
			};

			CacheLineRankTemplate(uint64_t const rn)
			: n(rn), datawords( (n+63)/64 ), indexwords((datawords + (blocksize-2))/(blocksize-1)), A (datawords+indexwords,false)
			{}

			WriteContext getWriteContext()
			{
				return WriteContext(A.get());
			}

			bool operator[](uint64_t i) const
			{
				uint64_t const datablock = (i>>6) / (blocksize-1);
				uint64_t const inblock = i - ((datablock<<6)*(blocksize-1));
				uint64_t const inblockword = (inblock >> 6);
				uint64_t const inword = inblock - (inblockword<<6);
				uint64_t const * wordptr = A.get() + ((datablock * blocksize) + inblockword + 1);
				return ::libmaus2::bitio::getBit(wordptr,inword);
			}

			uint64_t rank1(uint64_t i) const
			{
				if ( blocksize == 8 )
				{
					// block i lies in
					uint64_t const datablock = (i>>6) / (blocksize-1);
					// bit position in block
					uint64_t const inblock = i - ((datablock<<6)*(blocksize-1));
					// word in block
					uint64_t const inblockword = (inblock >> 6);
					// which half of block
					uint64_t const inblockhalf = inblockword / (blocksize/2);
					// position of word in half
					uint64_t const inblockrest = inblockword - inblockhalf*(blocksize/2);
					// bit position in word
					uint64_t const inword = inblock - (inblockword<<6);
					// pointer to block
					uint64_t const * wordptr = A.get() + ((datablock * blocksize) );

					uint64_t s = (*(wordptr++));
					s = (s >> 16) + ((s & 0xFFFFull) >> (16*(1-inblockhalf)));
					wordptr += inblockhalf*(blocksize/2);

					assert ( inblockrest < 4 );

					#if 0
					for ( uint64_t j = 0; j < inblockrest; ++j )
						s += popcount8( *(wordptr++) );
					s += popcount8( (*(wordptr++)) , inword );
					return s;
					#else

					switch ( inblockrest )
					{
						case 3:
							s += popcount8( wordptr[0] );
							s += popcount8( wordptr[1] );
							s += popcount8( wordptr[2] );
							break;
						case 2:
							s += popcount8( wordptr[0] );
							s += popcount8( wordptr[1]);
							break;
						case 1:
							s += popcount8( wordptr[0] );
							break;
						default:
							break;
					}

					s += popcount8( wordptr[inblockrest], inword );
					return s;
					#endif
				}
				else
				{
					uint64_t const datablock = (i>>6) / (blocksize-1);
					uint64_t const inblock = i - ((datablock<<6)*(blocksize-1));
					uint64_t const inblockword = (inblock >> 6);
					uint64_t const inword = inblock - (inblockword<<6);
					uint64_t const * wordptr = A.get() + ((datablock * blocksize) );

					uint64_t s = (*(wordptr++));
					for ( uint64_t j = 0; j < inblockword; ++j )
						s += popcount8( *(wordptr++) );
					s += popcount8( (*(wordptr++)) , inword );

					return s;
				}
			}

			uint64_t rankm1(uint64_t i) const
			{
				if ( blocksize == 8 )
				{
					// block i lies in
					uint64_t const datablock = (i>>6) / (blocksize-1);
					// bit position in block
					uint64_t const inblock = i - ((datablock<<6)*(blocksize-1));
					// word in block
					uint64_t const inblockword = (inblock >> 6);
					// which half of block
					uint64_t const inblockhalf = inblockword / (blocksize/2);
					// position of word in half
					uint64_t const inblockrest = inblockword - inblockhalf*(blocksize/2);
					// bit position in word
					uint64_t const inword = inblock - (inblockword<<6);
					// pointer to block
					uint64_t const * wordptr = A.get() + ((datablock * blocksize) );

					uint64_t s = (*(wordptr++));
					s = (s >> 16) + ((s & 0xFFFFull) >> (16*(1-inblockhalf)));
					wordptr += inblockhalf*(blocksize/2);

					for ( uint64_t j = 0; j < inblockrest; ++j )
						s += popcount8( *(wordptr++) );
					s += popcount8m1( (*(wordptr++)) , inword );

					return s;
				}
				else
				{
					uint64_t const datablock = (i>>6) / (blocksize-1);
					uint64_t const inblock = i - ((datablock<<6)*(blocksize-1));
					uint64_t const inblockword = (inblock >> 6);
					uint64_t const inword = inblock - (inblockword<<6);
					uint64_t const * wordptr = A.get() + ((datablock * blocksize) );

					uint64_t s = (*(wordptr++));
					for ( uint64_t j = 0; j < inblockword; ++j )
						s += popcount8( *(wordptr++) );
					s += popcount8m1( (*(wordptr++)) , inword );

					return s;
				}
			}

			/**
			 * return number of 0 bits up to (and including) index i
			 * @param i
			 * @return inverse population count
			 **/
			uint64_t rank0(uint64_t const i) const
			{
				return (i+1) - rank1(i);
			}
			/**
			 * Return the position of the ii'th 0 bit. This function is implemented using a
			 * binary search on the rank1 function.
			 **/
			uint64_t select1(uint64_t const ii) const
			{
				uint64_t const i = ii+1;

				uint64_t left = 0, right = n;

				while ( (right-left) )
				{
					uint64_t const d = right-left;
					uint64_t const d2 = d>>1;
					uint64_t const mid = left + d2;

					// number of ones is too small
					if ( rank1(mid) < i )
						left = mid+1;
					// number of ones is too large
					else if ( rank1(mid) > i )
						right = mid;
					// if this is the leftmost occurence in the interval, return it
					else if ( (!mid) || (rank1(mid-1) != i) )
						return mid;
					// otherwise, go on and search to the left
					else
						right = mid;
				}

				return n;
			}
			/**
			 * Return the position of the ii'th 0 bit. This function is implemented using a
			 * binary search on the rank0 function.
			 **/
			uint64_t select0(uint64_t const ii) const
			{
				uint64_t const i = ii+1;

				uint64_t left = 0, right = n;

				while ( (right-left) )
				{
					uint64_t const d = right-left;
					uint64_t const d2 = d>>1;
					uint64_t const mid = left + d2;

					// number of ones is too small
					if ( rank0(mid) < i )
						left = mid+1;
					// number of ones is too large
					else if ( rank0(mid) > i )
						right = mid;
					// if this is the leftmost occurence in the interval, return it
					else if ( (!mid) || (rank0(mid-1) != i) )
						return mid;
					// otherwise, go on and search to the left
					else
						right = mid;
				}

				return n;
			}

		};

		typedef CacheLineRankTemplate<8> CacheLineRank8;
		typedef CacheLineRankTemplate<4> CacheLineRank4;
		typedef CacheLineRankTemplate<2> CacheLineRank2;
	}
}
#endif
