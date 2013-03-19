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

#if ! defined(ERANK3C_HPP)
#define ERANK3C_HPP

#include <libmaus/types/types.hpp>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/rank/ERankBase.hpp>
#include <libmaus/rank/log2table.hpp>
#include <libmaus/bitio/BitWriter.hpp>
#include <libmaus/bitio/FastWriteBitWriter.hpp>

#include <cassert>
#include <iostream>

#include <libmaus/util/unique_ptr.hpp>

namespace libmaus
{
	namespace rank
	{
		/**
		 * three level (sbsize 2^16, lbsize 2^8, mbsize 2^4) rank index using ~.627 bits
		 * ( n / 512 + n / n / 8 + n / 2 ~ 0.627 ). The bit vector is stored in entropy coded form (enumerative code).
		 **/
		struct ERank3C : public ERankBase
		{
			public:
			typedef ::libmaus::bitio::FastWriteBitWriter2 writer_type;

			typedef ERank3C this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
		
			private:
			static unsigned int const sbbitwidth = 16;
			static unsigned int const lbbitwidth = 8;
			static unsigned int const mbbitwidth = 4;
			static uint64_t const sbsize = 1u<<sbbitwidth;
			static uint64_t const lbsize = 1u<<lbbitwidth;
			static uint64_t const mbsize = 1u<<mbbitwidth;
			static uint64_t const sbmask = sbsize-1;
			static uint64_t const lbmask = lbsize-1;
			static uint64_t const mbmask = mbsize-1;
		
			uint64_t const n;
			uint64_t n1;
			
			::libmaus::autoarray::AutoArray<uint16_t> CU;
			
			uint64_t const nums;
			// super block 1 bits
			::libmaus::autoarray::AutoArray<uint64_t> S;
			// super block code bits
			::libmaus::autoarray::AutoArray<uint64_t> SE;
			
			uint64_t const numl;
			// large block 1 bits
			::libmaus::autoarray::AutoArray<uint16_t> L;
			// large block code bits
			::libmaus::autoarray::AutoArray<uint16_t> LE;
			
			uint64_t const numm;
			::libmaus::autoarray::AutoArray<uint8_t> M;
			
			public:
			uint64_t byteSize() const
			{
				return
					2*sizeof(uint64_t) +
					CU.byteSize() +
					1*sizeof(uint64_t) +
					S.byteSize() +
					SE.byteSize() +
					1*sizeof(uint64_t) +
					L.byteSize() +
					LE.byteSize() +
					sizeof(uint64_t) +
					M.byteSize();
			}
			
			private:
			static void printBits(uint16_t num, ::std::ostream & out)
			{
				for ( uint64_t i = 0; i < 16; ++i )
					out << ((num & (1u<<(16-i-1))) != 0);
			}
			
			static uint16_t getBits(uint16_t const * const U, uint64_t const offset, unsigned int const num)
			{
				assert ( num <= mbsize );
			
				// modul
				uint64_t const m = offset & mbmask;
				// rest bits
				uint64_t const r = mbsize-m;
				
				// enough bits in one word
				if ( r >= num )
				{
					return (U[ offset >> mbbitwidth ] >> (r-num)) & ((1u<<num)-1);
				}
				// need to read two words
				else
				{
					uint16_t const high = (U [ offset >> mbbitwidth ] & ((1u<<r)-1)) << (num-r);
					uint16_t const low = U [ (offset >> mbbitwidth) + 1 ] >> (mbsize - (num-r));
					return (high | low);
				} 
			} 
			
			static uint16_t getBitsRef(uint16_t const * const U, uint64_t const offset, unsigned int const num)
			{
				uint64_t v = 0;
				
				for ( unsigned int i = 0; i < num; ++i )
				{
					v <<= 1;
					v |= ::libmaus::bitio::getBit2(U,offset+i);
				}
				
				return v;
			}

			/**
			 * return superblock containing i th 1 bit,
			 * i.e. largest j such that S[j] < ii
			 **/
			uint64_t selectSuper(uint64_t const ii) const
			{
				// search largest superblock index s such that ii < S[s]
				uint64_t left = 0, right = nums;

				while ( right-left > 1 )
				{
					uint64_t const d = right-left;
					uint64_t const d2 = d>>1;
					uint64_t const mid = left + d2;

					// number of 1s is too large
					if ( S[mid] < ii )
						left = mid;
					else
						right = mid;
				}

				return left;
			}
			/**
			 * return large block containing i th 1 bit,
			 * i.e. largest j such that L[j] < ii
			 **/
			uint64_t selectLarge(uint64_t const s, uint64_t const iii) const
			{
				uint64_t const ii = iii - S[s];
				
				uint64_t left = (s << sbbitwidth) >>  lbbitwidth;
				uint64_t right = ::std::min( numl, ((s+1) << sbbitwidth) >>  lbbitwidth);

				while ( right-left > 1 )
				{
					uint64_t const d = right-left;
					uint64_t const d2 = d>>1;
					uint64_t const mid = left + d2;

					// number of 1s is too large
					if ( L[mid] < ii )
						left = mid;
					else
						right = mid;
				}

				return left;
			}
			/**
			 * return mini block containing i th 1 bit,
			 * i.e. largest j such that M[j] < ii
			 **/
			uint64_t selectMini(uint64_t const s, uint64_t const l, uint64_t const iii) const
			{
				uint64_t const ii = iii - S[s] - L[l];
				
				uint64_t left = (l << lbbitwidth) >>  mbbitwidth;
				uint64_t right = ::std::min( numm, ((l+1) << lbbitwidth) >>  mbbitwidth);

				while ( right-left > 1 )
				{
					uint64_t const d = right-left;
					uint64_t const d2 = d>>1;
					uint64_t const mid = left + d2;

					// number of 1s is too large
					if ( M[mid] < ii )
						left = mid;
					else
						right = mid;
				}

				return left;
			}
			
			// mini block cumulative popcount
			uint64_t m1(uint64_t const m) const
			{
				if ( m < numm )
					return S[(m << mbbitwidth)>>sbbitwidth] + L[(m << mbbitwidth)>>lbbitwidth] + M[m];
				else
					return n1;
			}
			
			// mini block single pop count
			uint64_t minipopcount(uint64_t const m) const
			{
				assert ( m < numm );
				return m1(m+1) - m1(m);
			}
			uint64_t minipos(uint64_t const m) const
			{
				return SE[ (m << mbbitwidth) >> sbbitwidth ] + 
					LE[ (m << mbbitwidth) >> lbbitwidth ] + 
					entropy_estimate_up( M[ m ], (m << mbbitwidth) & lbmask );
			}
			public:
			uint16_t uc(uint64_t const m) const
			{
				#if 0
				std::cerr << "Reading block " << m << " at position " << minipos(m) << " bitcount=" << minipopcount(m) 
					<< " codelen " << static_cast<int>(EC16.bits_n[minipopcount(m)])
					<< " code "  << getBits(CU.get(), minipos(m), EC16.bits_n[minipopcount(m)]) 
					<< std::endl;
				#endif
				uint64_t const b = minipopcount(m);	
				return DC16.decode( getBits ( CU.get(), minipos(m), EC16.bits_n[b] ), b);
			}
			
			public:
			/**
			 * constructor
			 * @param U bit vector
			 * @param rn length of bit vector
			 **/	
			ERank3C(uint16_t const * const U, uint64_t const rn) 
			: n(rn), 
			  nums( ERANK2DIVUP(n,sbsize) ),
			  S( nums ),
			  SE( nums ),
			  numl( ERANK2DIVUP(n,lbsize) ),
			  L( numl ),
			  LE( numl ),
			  numm( ERANK2DIVUP(n,mbsize) ),
			  M( numm )
			{
				uint64_t sc = 0;
				uint64_t scc = 0;
			
				uint64_t l = 0, m = 0;
				// superblock counter s
				for ( uint64_t s = 0 ; s < nums; ++s )
				{
					// super block 1 bits
					S[s] = sc;
					// coded bits
					SE[s] = scc;
					// check pointer
					assert ( S[s] == sc );
					
					// large block 1 bits
					uint64_t lc = 0;
					// large block code bits
					uint64_t lcc = 0;
					// large block
					for ( uint64_t tl = 0; tl < sbsize/lbsize && l < numl; ++l, ++tl )
					{
						// large block 1 bits
						L[l] = lc;
						// coded bits
						LE[l] = lcc;
						// check pointer
						assert ( L[l] == lc );
						
						// miniblock 1 bits
						uint64_t mc = 0;
						// miniblock coded bits
						uint64_t cc = 0;
						// miniblock counter
						for ( uint64_t tm = 0; tm < (lbsize/mbsize) && m < numm; ++m, ++tm )
						{
							M[m] = mc; // 1 bits miniblock accu
							assert ( M[m] == mc );
							
							uint64_t const est = entropy_estimate_up(
								M[m] /* number of 1 bits */,
								tm << mbbitwidth /* total number of bits */
							);
							
							assert ( est >= cc );
							cc = est;						

							
							uint64_t const b = popcount2(U[m]); // number of 1 bits
							uint64_t eb = EC16.bits_n[b]; // number of coded bits
							cc += eb; // update coded bits
							mc += b;  // update mini block 1 bit accu
						} 
						lc += mc;  // update large block 1 bits
						lcc += cc; // update large block coded bits
						assert ( cc < lbsize );
					}
					scc += lcc; // update super block codec bits
					sc += lc;   // update super block 1 bits
				}
				
				// total number of 1 bits
				n1 = sc;
				
				// ::std::cerr << "bits " << n << " coded bits " << scc << ::std::endl;

				// coded bit vector				
				CU = ::libmaus::autoarray::AutoArray<uint16_t>((scc + 15) / 16 , false );
				::libmaus::bitio::BitWriter2 W(CU.get());

				l = 0, m = 0;
				for ( uint64_t s = 0 ; s < nums; ++s )
					for ( uint64_t tl = 0; tl < sbsize/lbsize && l < numl; ++l, ++tl )
					{
						uint64_t o = SE[s] + LE[l]; // code offset

						uint64_t mc = 0; // total 1 bits accu
						uint64_t cc = 0; // code position
						for ( uint64_t tm = 0; tm < (lbsize/mbsize) && m < numm; ++m, ++tm )
						{
							// estimate for code position
							uint64_t const est = entropy_estimate_up(M[m],tm << mbbitwidth);
							assert ( minipos(m) == o + est );
							assert ( est >= cc );
							
							while ( cc < est )
							{
								// write pad bits
								W.writeBit(0);
								++cc;
							}

							#if 0
							if ( m < 600 )
							{
								std::cerr << "Writing block " << m << "=";
								printBits(U[m],std::cerr);
								std::cerr << " at " << est << " M[]=" 
									<< static_cast<uint64_t>(M[m]) 
									<< " bits=" << popcount2(U[m])
									<< " codelen=" << static_cast<int>(EC16.bits_n[popcount2(U[m])])
									<< " code=" << EC16.encode(U[m])
									<< std::endl;
							}
							#endif

							// count bits
							uint64_t const b = popcount2(U[m]);
							// encode mask
							uint16_t const e = EC16.encode(U[m]);
							assert ( DC16.decode ( e, b ) == U[m] );
							// number of coded bits
							uint64_t const eb = EC16.bits_n[b];
							// write bits
							W.write(e,eb);
							cc += eb;
							// count total bits
							mc += b;
						}
					}
				W.flush();
				
				#if 1
				for ( uint64_t i = 0; i < numm; ++i )
				{
					if ( U[i] != uc(i) )
						std::cerr << "Failure for block " << i << " of " << numm << std::endl;
					assert ( U[i] == uc(i) );
				}
					
				for ( uint64_t offset = 0; offset < n; ++offset )
					for ( uint64_t len = 0; len <= 16 && offset+len < n; ++len )
						assert ( getBits(U,offset,len) == getBitsRef(U,offset,len) );
				#endif
			}
			
			/**
			 * return number of 1 bits up to (and including) index i
			 * @param i
			 * @return population count
			 **/
			uint64_t rank1(uint64_t i) const
			{
				uint64_t const m = i>>mbbitwidth;
				return S[i>>sbbitwidth] + L[i>>lbbitwidth] + M[m] + popcount2(uc(m),i - (m<<mbbitwidth));
			} 
			/**
			 * return number of 0 bits up to (and including) index i
			 * @param i
			 * @return inverse population count
			 **/
			uint64_t rank0(uint64_t i) const
			{
				return (i+1) - rank1(i);
			}
			/**
			 * Return the position of the ii'th 0 bit. This function is implemented using a 
			 * binary search on the rank1 function.
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
			#if 0
			/**
			 * Return the position of the ii'th 0 bit. This function is implemented using a 
			 * binary search on the rank0 function.
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
			#else		
			/**
			 * Return the position of the ii'th 1 bit. This function is implemented using a 
			 * binary search on the rank0 function.
			 **/
			uint64_t select1(uint64_t const ii) const
			{
				uint64_t i = ii+1;

				// ::std::cerr << "in.";
				uint64_t const s = selectSuper(i);
				uint64_t const l = selectLarge(s,i);
				uint64_t const m = selectMini(s,l,i);
				i -= S[s]; 
				i -= L[l];
				i -= M[m];
				// ::std::cerr << "out." << ::std::endl;
				
				uint16_t const v = uc(m);
				
				uint64_t left = 0, right = 1u<<mbbitwidth;
				while ( right-left )
				{
					uint64_t const d = right-left;
					uint64_t const d2 = d>>1;
					uint64_t const mid = left + d2;
					uint64_t const rmid = popcount2( v, mid );

					// number of ones is too small
					if ( rmid < i )
						left = mid+1;
					// number of ones is too large
					else if ( rmid > i )
						right = mid;
					// if this is the leftmost occurence in the interval, return it
					else if ( (!mid) || (popcount2( v, mid-1 ) != i) )
					{
						return (m<<mbbitwidth)+mid;
					}
					// otherwise, go on and search to the left
					else
						right = mid;
				}
				
				return n;
			}
			#endif

		};
	}
}
#endif
