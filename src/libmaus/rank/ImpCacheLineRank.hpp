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

#if ! defined(IMPCACHELINERANK_HPP)
#define IMPCACHELINERANK_HPP

#include <libmaus/types/types.hpp>
#include <libmaus/rank/ERankBase.hpp>
#include <libmaus/select/ESelectBase.hpp>
#include <libmaus/bitio/getBit.hpp>
#include <libmaus/aio/SynchronousGenericOutput.hpp>
#include <libmaus/network/Socket.hpp>

namespace libmaus
{
	namespace rank
	{
		struct ImpCacheLineRank : ::libmaus::rank::ERankBase
		{
			typedef ImpCacheLineRank this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			
			static uint64_t const bitsperword = 8*sizeof(uint64_t);		
			static uint64_t const datawordsperblock = 6;
			static uint64_t const bitsperblock = bitsperword*datawordsperblock;

			uint64_t const n;
			uint64_t const datawords;
			uint64_t const indexwords;
			uint64_t const numblocks;
			::libmaus::autoarray::AutoArray<uint64_t, ::libmaus::autoarray::alloc_type_memalign_cacheline> A;

			#define LIBMAUS_RANK_IMPCACHELINERANK_STORENODEPOINTERS
			#if defined(LIBMAUS_RANK_IMPCACHELINERANK_STORENODEPOINTERS)
			ImpCacheLineRank * left;
			ImpCacheLineRank * right;
			ImpCacheLineRank * parent;
			#endif
			
			uint64_t byteSize() const
			{
				return 
					4*sizeof(uint64_t)+
					#if defined(LIBMAUS_RANK_IMPCACHELINERANK_STORENODEPOINTERS)
					3*sizeof(ImpCacheLineRank *)+
					#endif
					A.byteSize();
			}
			
			uint64_t serialise(std::ostream & out) const
			{
				uint64_t s = 0;
				s += ::libmaus::serialize::Serialize<uint64_t>::serialize(out,n);
				s += A.serialize(out);
				return s;
			}
			
			void serialise(::libmaus::network::SocketBase * socket) const
			{
				socket->writeSingle<uint64_t>(n);
				socket->writeMessageInBlocks<uint64_t,::libmaus::autoarray::alloc_type_memalign_cacheline>(A);
			}

			uint64_t deserialiseNumber(std::istream & in)
			{
				uint64_t n;
				::libmaus::serialize::Serialize<uint64_t>::deserialize(in,&n);
				return n;
			}
			
			ImpCacheLineRank(uint64_t const rn)
			: n(rn), datawords( (n+63)/64 ), indexwords( 2 * ((datawords+5)/6) ),
			  numblocks( (n+bitsperblock-1)/bitsperblock ),
			  A (datawords+indexwords,false)
			  #if defined(LIBMAUS_RANK_IMPCACHELINERANK_STORENODEPOINTERS)
			  , left(0), right(0), parent(0)
			  #endif
			{}

			ImpCacheLineRank(::libmaus::network::SocketBase * in)
			: n(in->readSingle<uint64_t>()), datawords( (n+63)/64 ), indexwords( 2 * ((datawords+5)/6) ), 
			  numblocks( (n+bitsperblock-1)/bitsperblock ),
			  A(in->readMessageInBlocks<uint64_t,::libmaus::autoarray::alloc_type_memalign_cacheline>())
			  #if defined(LIBMAUS_RANK_IMPCACHELINERANK_STORENODEPOINTERS)
			  , left(0), right(0), parent(0)
			  #endif
			{
			
			}

			ImpCacheLineRank(std::istream & in)
			: n(deserialiseNumber(in)), datawords( (n+63)/64 ), indexwords( 2 * ((datawords+5)/6) ), 
			  numblocks( (n+bitsperblock-1)/bitsperblock ),
			  A(in)
			  #if defined(LIBMAUS_RANK_IMPCACHELINERANK_STORENODEPOINTERS)
			  , left(0), right(0), parent(0)
			  #endif
			{
			
			}

			ImpCacheLineRank(std::istream & in, uint64_t & s)
			: n(deserialiseNumber(in)), datawords( (n+63)/64 ), indexwords( 2 * ((datawords+5)/6) ), 
			  numblocks( (n+bitsperblock-1)/bitsperblock ),
			  A(in,s)
			  #if defined(LIBMAUS_RANK_IMPCACHELINERANK_STORENODEPOINTERS)
			  , left(0), right(0), parent(0)
			  #endif
			{
				s += sizeof(uint64_t);
			}

			void checkSanity() const
			{
				uint64_t R[2] = {0,0};
				
				for ( uint64_t i = 0; i < n; ++i )
				{
					bool const bit = (*this)[i];
					R [ bit ] ++;
					
					if ( bit )
					{
						assert ( R[bit] == rank1(i) );
						assert ( R[!bit] == rank0(i) );
					}
					else
					{
						assert ( R[bit] == rank0(i) );
						assert ( R[!bit] == rank1(i) );
					}
				}		
			}


			struct WriteContext
			{
				uint64_t bitpos;
				uint64_t w;
				uint64_t s;
				uint64_t * p;
				uint64_t * ps;
				
				WriteContext(uint64_t * rp = 0)
				: bitpos(0), w(0), s(0), p(rp), ps(p)
				{}
				
				void writeBit(bool const rb)
				{				
					// save sum if new block
					if ( bitpos == 0 )
					{
						ps = p;
						*(p++) = s; // super block
						*(p++) = 0; // in block
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
						// in block	
						ps[1] |= (s - ps[0]) << (9*(bitpos>>6));

						if ( bitpos == 6*64 )
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
			
			struct WriteContextExternal
			{
				typedef WriteContextExternal this_type;
				typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				
				static unique_ptr_type construct(std::string const & filename, uint64_t const n, bool const writeHeader = true)
				{
					unique_ptr_type ptr(new this_type(filename,n,writeHeader));
					return UNIQUE_PTR_MOVE(ptr);
				}
				static unique_ptr_type construct(std::ostream & out, uint64_t const n, bool const writeHeader = true)
				{
					unique_ptr_type ptr(new this_type(out,n,writeHeader));
					return UNIQUE_PTR_MOVE(ptr);
				}
			
				uint64_t bitpos;
				uint64_t w;
				uint64_t s;
				
				::libmaus::autoarray::AutoArray<uint64_t> B;
				uint64_t * p;
				uint64_t * ps;
				
				::libmaus::util::unique_ptr<std::ofstream>::type Postr;
				::std::ostream & ostr;
				::libmaus::aio::SynchronousGenericOutput<uint64_t>::unique_ptr_type out;
				
				uint64_t blockswritten;
				
				WriteContextExternal(std::string const & filename, uint64_t const n, bool const writeHeader = true)
				: bitpos(0), w(0), s(0), 
				  B(8), p(B.begin()), ps(p),
				  Postr(new std::ofstream(filename.c_str(),std::ios::binary)),
				  ostr(*Postr),
				  blockswritten(0)
				{
					if ( writeHeader )
					{
						uint64_t const words = ((((n+63)/64)+5)/6)*8;
						// write n
						::libmaus::serialize::Serialize<uint64_t>::serialize(ostr,n);
						// write auto array header
						::libmaus::serialize::Serialize<uint64_t>::serialize(ostr,words);
					}
					// instantiate output buffer
					::libmaus::aio::SynchronousGenericOutput<uint64_t>::unique_ptr_type tout(
                                                new ::libmaus::aio::SynchronousGenericOutput<uint64_t>(ostr,64*1024));
					out = UNIQUE_PTR_MOVE(tout);
				}

				WriteContextExternal(std::ostream & rostr, uint64_t const n, bool const writeHeader = true)
				: bitpos(0), w(0), s(0), 
				  B(8), p(B.begin()), ps(p),
				  ostr(rostr),
				  blockswritten(0)
				{
					if ( writeHeader )
					{
						uint64_t const words = ((((n+63)/64)+5)/6)*8;
						// write n
						::libmaus::serialize::Serialize<uint64_t>::serialize(ostr,n);
						// write auto array header
						::libmaus::serialize::Serialize<uint64_t>::serialize(ostr,words);
					}
					// instantiate output buffer
					::libmaus::aio::SynchronousGenericOutput<uint64_t>::unique_ptr_type tout(
                                                new ::libmaus::aio::SynchronousGenericOutput<uint64_t>(ostr,64*1024));
					out = UNIQUE_PTR_MOVE(tout);
				}
				~WriteContextExternal()
				{
					flush();
				}
				
				void writeBit(bool const rb)
				{				
					// save sum if new block
					if ( bitpos == 0 )
					{
						*(p++) = s; // super block
						*(p++) = 0; // in block
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
						// in block	
						ps[1] |= (s - ps[0]) << (9*(bitpos>>6));

						// if buffer is full
						if ( bitpos == 6*64 )
						{
							p = B.begin();
							for ( uint64_t i = 0; i < 8; ++i )
								out->put(B[i]);
							bitpos = 0;
							blockswritten++;
						}
					}
					
				}
				void flush()
				{
					// finish word
					while ( bitpos )
						writeBit(0);
					out->flush();
					ostr.flush();
				}
				uint64_t wordsWritten() const
				{
					return blockswritten*B.size();
				}
			};
			
			WriteContext getWriteContext()
			{
				return WriteContext(A.begin());
			}

			bool operator[](uint64_t const i) const
			{
				uint64_t const datablock = (i>>7)/3; // 6*64 = 3*128 bits per block
				uint64_t const inblockbit = i-((datablock<<7)*3);
				uint64_t const inblockword = (inblockbit >> 6);
				uint64_t const inword = inblockbit - (inblockword<<6);
				uint64_t const * wordptr = A.get() + ((datablock << 3) + inblockword + 2);
				return ::libmaus::bitio::getBit(wordptr,inword);
			}
			
			uint64_t inverseSelect1(uint64_t const i, unsigned int & sym) const
			{
				uint64_t const datablock = (i>>7)/3; // 6*64 = 3*128 bits per block
				uint64_t const inblockbit = i-((datablock<<7)*3);
				uint64_t const inblockword = (inblockbit >> 6);
				uint64_t const inword = inblockbit - (inblockword<<6);
				uint64_t const * blockptr = A.get() + (datablock << 3);
				
				sym = ::libmaus::bitio::getBit(blockptr+inblockword+2,inword);
				uint64_t const r1 = blockptr[0] + ((blockptr[1] >> (inblockword*9)) & ((1ull<<9)-1)) +
						popcount8(blockptr[2+inblockword],inword);
			
				return r1;			
			}

			uint64_t rank1(uint64_t i) const
			{
				// std::cerr << "rank1(" << i << "),  n=" << n << std::endl;
			
				uint64_t const datablock = (i>>7)/3; // 6*64 = 3*128 bits per block
				uint64_t const inblockbit = i-((datablock<<7)*3);
				uint64_t const inblockword = (inblockbit >> 6);
				uint64_t const inword = inblockbit - (inblockword<<6);
				uint64_t const * blockptr = A.get() + (datablock << 3);
				
				return
					blockptr[0] +
					((blockptr[1] >> (inblockword*9)) & ((1ull<<9)-1)) +
					popcount8(blockptr[2+inblockword],inword);
			}
			
			uint64_t slowRank1(uint64_t i) const
			{
				uint64_t fullblocks = (i/64)/6;
				
				uint64_t s = 0;
				uint64_t const * p = A.get();
				for ( uint64_t j = 0; j < fullblocks; ++j )
				{
					assert ( *p == s );
					p++;
					p++;

					for ( uint64_t k = 0; k < 6; ++k )
						s += popcount8(*(p++));
				}
				
				i -= fullblocks*6*64;
				assert ( *p == s );
				p++;
				p++;
				
				while ( i >= 64 )
				{
					i -= 64;
					s += popcount8(*(p++));
				}
				
				s += popcount8(*p,i);
				
				return s;
			}

			uint64_t rankm1(uint64_t i) const
			{
				uint64_t const datablock = (i>>7)/3; // 6*64 = 3*128 bits per block
				uint64_t const inblockbit = i-((datablock<<7)*3);
				uint64_t const inblockword = (inblockbit >> 6);
				uint64_t const inword = inblockbit - (inblockword<<6);
				uint64_t const * blockptr = A.get() + (datablock << 3);
				
				return
					blockptr[0] +
					((blockptr[1] >> (inblockword*9)) & ((1ull<<9)-1)) +
					popcount8m1(blockptr[2+inblockword],inword);
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
			 * return number of 0 bits up to (and excluding) index i
			 * @param i
			 * @return inverse population count
			 **/
			uint64_t rankm0(uint64_t const i) const
			{
				return i - rankm1(i);
			}
			
			uint64_t selectOneBlock(uint64_t const i) const
			{
				uint64_t left = 0, right = numblocks;				
				
				while ( right-left > 1 )
				{
					uint64_t const mid = (left+right)>>1;
					uint64_t const blockoffset = (mid << 3);
					uint64_t const prerank = A[blockoffset];
					
					if ( i < prerank )
						right = mid;
					else
						left = mid;
				}
				
				return left;
			}
			
			uint64_t select1(uint64_t i) const
			{
				// select large block
				uint64_t const block = selectOneBlock(i);
				uint64_t const blockoffset = block<<3;
				uint64_t const * const P = A.begin()+blockoffset;
				i -= *P;
				
				// select small block
				unsigned int subblock = 0;
				unsigned int subblockp1shift = 9;
				unsigned int subsub;
				unsigned int subsubsub = 0;
				while ( i >= (subsub=((P[1] >> subblockp1shift) & ((1ull<<9)-1))) )
				{
					subblock++;
					subblockp1shift += 9;
					subsubsub = subsub;
				}				
				i -= subsubsub;
				
				// select bit in small block
				// word
				uint64_t v = P[2+subblock];
				
				// now find correct position in word
				uint64_t woff = 0;
				uint64_t lpcnt;

				// 64 bits left, figure out if bit is in upper or lower half of word
				if ( i >= (lpcnt=::libmaus::rank::PopCnt4<sizeof(int)>::popcnt4(v >> 32)) )
					i -= lpcnt, v &= 0xFFFFFFFFUL, woff += 32;
				else
					v >>= 32;
				// 32 bits left
				if ( i >= (lpcnt=::libmaus::rank::PopCnt4<sizeof(int)>::popcnt4(v >> 16)) )
					i -= lpcnt, v &= 0xFFFFUL, woff += 16;
				else
					v >>= 16;

				// 16 bits left
				if ( i >= (lpcnt=::libmaus::rank::PopCnt4<sizeof(int)>::popcnt4(v >> 8)) )
					i -= lpcnt, v &= 0xFFUL, woff += 8;
				else
					v >>= 8;
				// 8 bits left
				if ( i >= (lpcnt=::libmaus::rank::PopCnt4<sizeof(int)>::popcnt4(v >> 4)) )
					i -= lpcnt, v &= 0xFUL, woff += 4;
				else
					v >>= 4;
				// 4 bits left
				if ( i >= (lpcnt=::libmaus::rank::PopCnt4<sizeof(int)>::popcnt4(v >> 2)) )
					i -= lpcnt, v &= 0x3UL, woff += 2;
				else
					v >>= 2;
				// 2 bits left
				if ( i >= (lpcnt=::libmaus::rank::PopCnt4<sizeof(int)>::popcnt4(v >> 1)) )
					i -= lpcnt, v &= 0x1UL, woff += 1;
				else
					v >>= 1;

				// done				
				return block*bitsperblock + subblock*bitsperword + woff;
			}
			
			/**
			 * Return the position of the ii'th 0 bit. This function is implemented using a 
			 * binary search on the rank1 function.
			 **/
			uint64_t select1Slow(uint64_t const ii) const
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

			uint64_t excess(uint64_t const i) const
			{
				assert ( rank1(i) >= rank0(i) );
				return rank1(i)-rank0(i);
			}
			int64_t excess(uint64_t const i, uint64_t const j) const
			{
				return static_cast<int64_t>(excess(i)) - static_cast<int64_t>(excess(j));
			}

		};
		struct ImpCacheLineRankSelectAdapter
		{
			typedef ImpCacheLineRankSelectAdapter this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
		
			ImpCacheLineRank const & ICLR;
			::libmaus::select::ESelectBase<1> ESB;
			
			ImpCacheLineRankSelectAdapter(ImpCacheLineRank const & rICLR)
			: ICLR(rICLR), ESB()
			{
			
			}
			

			uint64_t select1(uint64_t i) const
			{
				// select large block
				uint64_t const block = ICLR.selectOneBlock(i);
				uint64_t const blockoffset = block<<3;
				uint64_t const * const P = ICLR.A.begin()+blockoffset;
				i -= *P;
				
				// select small block
				unsigned int subblock = 0;
				unsigned int subblockp1shift = 9;
				unsigned int subsub;
				unsigned int subsubsub = 0;
				while ( i >= (subsub=((P[1] >> subblockp1shift) & ((1ull<<9)-1))) )
				{
					subblock++;
					subblockp1shift += 9;
					subsubsub = subsub;
				}				
				i -= subsubsub;
				
				// select bit in small block
				// word
				uint64_t v = P[2+subblock];
				
				// now find correct position in word
				uint64_t woff = 0;
				uint64_t lpcnt;

				// 64 bits left, figure out if bit is in upper or lower half of word
				if ( i >= (lpcnt=::libmaus::rank::PopCnt4<sizeof(int)>::popcnt4(v >> 32)) )
					i -= lpcnt, v &= 0xFFFFFFFFUL, woff += 32;
				else
					v >>= 32;
				// 32 bits left
				if ( i >= (lpcnt=::libmaus::rank::PopCnt4<sizeof(int)>::popcnt4(v >> 16)) )
					i -= lpcnt, v &= 0xFFFFUL, woff += 16;
				else
					v >>= 16;

				// 16 bits left, use lookup table for rest
				return 
					block*ImpCacheLineRank::bitsperblock + subblock*ImpCacheLineRank::bitsperword + woff +
					ESB.R [ ((v&0xFFFFull) << 4) | i ];
			}
		};
	}
}
#endif
