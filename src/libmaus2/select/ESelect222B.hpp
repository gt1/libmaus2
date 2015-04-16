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

#if ! defined(ESELECT222B_HPP)
#define ESELECT222B_HPP

#include <libmaus2/select/ESelectBase.hpp>
#include <libmaus2/bitio/BitWriter.hpp>
#include <libmaus2/rank/ERankBase.hpp>
#include <libmaus2/rank/ERank222B.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/util/unique_ptr.hpp>
#include <cassert>
#include <stdexcept>
#include <memory>

#include <libmaus2/bitio/getBit.hpp>
#include <libmaus2/bitio/BitVector.hpp>

namespace libmaus2
{
	namespace select
	{
		/**
		 * three level select dictionary
		 **/
		template<bool sym>
		struct ESelect222B : public ESelectBase<sym>
		{
			typedef ESelect222B<sym> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			
			typedef ::libmaus2::bitio::BitWriter8 writer_type;

			private:
			static unsigned int const sbonewidth = 16;
			static unsigned int const mbonewidth =  8;
			static unsigned int const mibonewidth =  4;
			static uint64_t const sbones = 1ull << sbonewidth;
			static uint64_t const mbones = 1ull << mbonewidth;
			static uint64_t const mibones = 1ull << mibonewidth;

			static unsigned int const largesbwidth = 32;
			static uint64_t const largesbthres = 1ull << largesbwidth;
			
			static uint64_t const minipersuper = sbones / mbones;

			static uint64_t const largembwidth = 16;
			static uint64_t const largembthres = 1ull << largembwidth;
			
			uint64_t const * const UUUUUUUU;
			uint64_t const n;
			
			uint64_t const num1;
			
			uint64_t const numsuper;
			::libmaus2::autoarray::AutoArray < uint64_t > S;
			::libmaus2::autoarray::AutoArray < uint64_t > SL;
			::libmaus2::util::unique_ptr < ::libmaus2::rank::ERank222B >::type SLr;
			// large superblocks
			::libmaus2::autoarray::AutoArray < uint64_t > LS;
			// miniblocks
			::libmaus2::autoarray::AutoArray < uint32_t > M;
			// large miniblocks vector
			::libmaus2::autoarray::AutoArray < uint64_t > ML;
			::libmaus2::util::unique_ptr <  ::libmaus2::rank::ERank222B >::type MLr;
			// large miniblocks
			::libmaus2::autoarray::AutoArray < uint32_t > LM;
			// microblock boundaries
			::libmaus2::autoarray::AutoArray < uint16_t > MI;
	

			static uint64_t computeNum1(uint64_t const * const UUUUUUUU, uint64_t const rn)
			{
				if ( rn % 64 )
					throw ::std::runtime_error("libmaus2::select::ESelect222B: n is not multiple of miniblock size 64.");

				uint64_t const numwords = rn / 64;
				uint64_t num1 = 0;
				
				for ( uint64_t i = 0; i < numwords; ++i )
					num1 += ::libmaus2::rank::ERankBase::popcount8(ESelectBase<sym>::process(UUUUUUUU[i]));
					
				return num1;
			}
			
			/**
			 * superblock functions
			 **/
			bool isLargeSuperBlock(uint64_t const s) const
			{
				return ::libmaus2::bitio::getBit(SL.get(), s) != 0;
			}
			uint64_t getSmallSuperBlockIndex(uint64_t const s) const
			{
				return SLr->rank0(s) - 1;
			}
			uint64_t getLargeSuperBlockIndex(uint64_t const s) const
			{
				return SLr->rank1(s) - 1;
			}
			
			/**
			 * miniblock functions
			 **/
			uint64_t getNextMiniBlockStart(uint64_t const s, uint64_t m) const
			{
				assert ( ! isLargeSuperBlock(s) );
				
				if ( (m % minipersuper) != (minipersuper-1) )
					return S[s] + M[m+1];
				else
					return S[s+1];
			}
			
			uint64_t getMiniBlockSize(uint64_t const s, uint64_t const m) const
			{
				assert ( ! isLargeSuperBlock(s) );

				return getNextMiniBlockStart(s,m) - (S[s] + M[m]);
			}
			
			bool isLargeMiniBlock(uint64_t const s, uint64_t const m) const
			{
				return getMiniBlockSize(s,m) > largembthres;
			}
			
			uint64_t getSmallMiniBlockIndex(uint64_t const m) const
			{
				return MLr->rank0(m) - 1;
			}
			uint64_t getLargeMiniBlockIndex(uint64_t const m) const
			{
				return MLr->rank1(m) - 1;
			}

			static ::std::string printBits(uint64_t v)
			{
				::std::string s(64,' ');
				::std::string::iterator a = s.begin();
				for ( uint64_t mask = 1ull << 63; mask ; mask >>=1 )
					*(a++) = ( (v&mask)?'1':'0' );
				return s;
			}
			

			public:
			uint64_t select1(uint64_t i) const
			{
				// superblock index
				uint64_t const s = i / sbones;
				
				// large superblock?
				if ( isLargeSuperBlock(s) )
				{
					::std::cerr << "large superblock encountered in select1." << ::std::endl;	
					return LS [ (getLargeSuperBlockIndex(s) * sbones) + (i - s*sbones) ];
				}
				
				// (absolute) miniblock index
				uint64_t const m = i / mbones;
				// index for M array (skipping over large superblocks)
				uint64_t const mp = m - (SLr->rank1(s) * minipersuper);
				// offset
				uint64_t const mm = m - (s * minipersuper);

				// large miniblock?			
				if ( ::libmaus2::bitio::getBit(ML.get(), mp) )
				{
					uint64_t const lmi = getLargeMiniBlockIndex(mp);
					// ::std::cerr << "large miniblock." << ::std::endl;
					return S[s] + LM [ mbones * lmi + (i - ( (sbones * s) + (mbones * mm) )) ];
				}
				
				// microblocks per miniblock
				uint64_t const micropersuper = sbones / mibones;
				uint64_t const micropermini = mbones / mibones;
				// pre microblock index
				uint64_t mi = i / mibones;
				// index for MI array
				uint64_t mip = mi - ( (SLr->rank1(s) * micropersuper) + (MLr->rank1(m) * micropermini));
				// offset
				uint64_t mimi = mi - (m * micropermini);
				
				i -= (sbones * s) + (mbones * mm) + (mibones * mimi);
				uint64_t j = S[s] + M[mp] + MI[mip];
				
				// assert ( ::libmaus2::bitio::getBit( UUUUUUUU , j ) == 1 );

				if ( ! i )
					return j;
					
				// we need to find i more 1 bits after the one at
				// position j
				
				// check rest in current word
				if ( (j + 1)%64 )
				{
					uint64_t const v = ESelectBase<sym>::process(UUUUUUUU[ (j+1)/ 64 ]);
					unsigned int const restbits = (64-(j+1)%64);
					uint64_t const restmask = ((1ull << restbits)-1);
					uint64_t const p = ::libmaus2::rank::ERankBase::popcount8( v & restmask );

					// selected bit is not in this word
					if ( p < i )
						j += restbits, i -= p;
					// selected bit is in this word
					else
						return 1 + (j + ESelectBase<sym>::select1(((v&restmask) << (64-restbits)),i-1));					
				}
				
				uint64_t w = (j+1)/64;
				unsigned int p;
					
				while ( (p=::libmaus2::rank::ERankBase::popcount8(ESelectBase<sym>::process(UUUUUUUU[w]))) < i )
				{
					j += 64;
					i -= p;
					w++;
				}

				return 1 + (j + ESelectBase<sym>::select1(ESelectBase<sym>::process(UUUUUUUU[w]),i-1));			
			}
			
			ESelect222B(uint64_t const * const rUUUUUUUU, uint64_t const rn)
			: UUUUUUUU(rUUUUUUUU), n(rn), num1(computeNum1(UUUUUUUU,rn)),
			  numsuper ( (num1 + sbones -1 ) / sbones ), S(numsuper+1,false)
			{
				if ( ! n )
					return;

				if ( n % 64 )
					throw ::std::runtime_error("::libmaus2::select::ESelect222B: n is not multiple of miniblock size 64.");
					
				uint64_t const numwords = n / 64;
				
				/**
				 * assign superblock boundaries
				 **/
				uint64_t j = 0;
				uint64_t o = 0;
				int64_t s = -1;
				for ( uint64_t i = 0; i < numwords; ++i )
				{
					uint64_t const v = ESelectBase<sym>::process(UUUUUUUU[i]);
					
					for ( uint64_t mask = 1ull << 63; mask; mask >>= 1, j++ )
						if ( v & mask )
						{
							if ( o % sbones == 0 )
								S[ ++s ] = j;
							o += 1;
						}						
				}
				S[++s] = j;
				
				/**
				 * construct large superblock vector
				 **/
				SL = ::libmaus2::autoarray::AutoArray<uint64_t> ( (numsuper + 63)/64 );
				writer_type WSL (SL.get());
				
				for ( uint64_t i = 0; i < numsuper; ++i )
					WSL.writeBit( (S[i+1] - S[i] > largesbthres) ? true : false );
				WSL.flush();
				
				/**
				 * rank dictionary for large superblock vector
				 **/	
				::libmaus2::util::unique_ptr <  ::libmaus2::rank::ERank222B >::type tSLr (
                                        new ::libmaus2::rank::ERank222B( SL.get(), ((numsuper + 63)/64)*64 )
                                );
				SLr = UNIQUE_PTR_MOVE(tSLr);
				
				uint64_t numlargesuper = SLr->rank1(numsuper - 1);
				uint64_t numsmallsuper = SLr->rank0(numsuper - 1);
				assert ( numsuper == numlargesuper + numsmallsuper );

				if ( numlargesuper )
					LS = autoarray::AutoArray<uint64_t> (numlargesuper * sbones);
				if ( numsmallsuper ) 
					M = autoarray::AutoArray < uint32_t > (numsmallsuper * minipersuper );
					
				int64_t m = -1;
				bool islargesuper = false;
				s = -1; j = 0; o = 0;

				/**
				 * fill miniblock boundaries and large superblocks
				 **/
				uint64_t ls = 0;
				for ( uint64_t i = 0; i < numwords; ++i )
				{
					uint64_t const v = ESelectBase<sym>::process(UUUUUUUU[i]);
					
					for ( uint64_t mask = 1ull << 63; mask; mask >>= 1, j++ )
						if ( v & mask )
						{
							// next superblock
							if ( (o % sbones) == 0 )
							{
								++s;
								islargesuper = isLargeSuperBlock(s);
								assert ( S[s] == j );
							}
							// assign miniblock boundaries
							if ( (!islargesuper) && (( o % mbones ) == 0) )
							{
								M [ ++m ] = j - S[s];
								assert ( M[m] + S[s] == j );
							}
							// large superblock entry
							if ( islargesuper )
							{
								LS[ ls++ ] = j;
							}
							o += 1;						
						}
				}
				m += 1;
				// std::cerr << "m=" << m << " numsuper " << numsuper << " numsmallsuper " << numsmallsuper << " numsmallsuper * minipersuper = " << numsmallsuper * minipersuper << std::endl;
				assert ( static_cast<uint64_t>(m) <= numsmallsuper * minipersuper );
				
				/**
				 * allocate large miniblock vector
				 **/
				ML = autoarray::AutoArray < uint64_t > ( (m + 63) / 64 );
				writer_type WML (ML.get());

				m = -1; islargesuper = false; s = -1; j = 0; o = 0;

				/**
				 * fill large miniblock vector
				 **/
				for ( uint64_t i = 0; i < numwords; ++i )
				{
					uint64_t const v = ESelectBase<sym>::process(UUUUUUUU[i]);

					for ( uint64_t mask = 1ull << 63; mask; mask >>= 1, j++ )
						if ( v & mask )
						{
							if ( (o % sbones) == 0 )
							{
								++s;
								islargesuper = isLargeSuperBlock(s);
								assert ( S[s] == j );
							}
							if ( (!islargesuper) && (( o % mbones ) == 0) )
							{
								++m;
								
								WML . writeBit ( isLargeMiniBlock(s,m) ? true : false );
								
								assert ( M[m] + S[s] == j );
							}
							o += 1;						
						}
				}
				m += 1;
				WML.flush();
				/**
				 * construct rank dictionary for large miniblock vector
				 **/
				::libmaus2::util::unique_ptr< ::libmaus2::rank::ERank222B >::type tMLr ( new  ::libmaus2::rank::ERank222B( ML.get(), ((m + 63)/64)*64 ) );
				MLr = UNIQUE_PTR_MOVE(tMLr);
				
				uint64_t largemini = MLr->rank1(m - 1);
				uint64_t smallmini = MLr->rank0(m - 1);
				assert ( largemini + smallmini == static_cast<uint64_t>(m) );
				
				if ( largemini )
					LM = autoarray::AutoArray<uint32_t>(largemini * mbones,false);

				s = -1; m = -1; j = 0; o = 0;
				islargesuper = false;
				bool islargemini = false;
				int64_t lm = -1;

				/**
				 * fill large miniblock array
				 **/
				for ( uint64_t i = 0; i < numwords; ++i )
				{
					uint64_t const v = ESelectBase<sym>::process(UUUUUUUU[i]);
					
					for ( uint64_t mask = 1ull << 63; mask; mask >>= 1, j++ )
						if ( v & mask )
						{
							if ( (o % sbones) == 0 )
							{
								++s;
								islargesuper = isLargeSuperBlock(s);
								assert ( S[s] == j );
							}
							if ( (!islargesuper) && (( o % mbones ) == 0) )
							{
								++m;
								islargemini = bitio::getBit(ML.get(), m);
								assert ( M[m] + S[s] == j );
							}
							if ( (!islargesuper) && (islargemini) )
							{
								LM[++lm] = j - S[s];	
								assert ( j == LM[lm] + S[s] );
							}
							o += 1;						
						}
				}

				// std::cerr << "smallmini=" << smallmini << " largemini=" << largemini << std::endl;

				s = -1; m = -1; j = 0; o = 0;
				islargesuper = false;
				islargemini = false;

				/**
				 * fill large miniblock array
				 **/
				int64_t mi = -1;
				for ( uint64_t i = 0; i < numwords; ++i )
				{
					uint64_t const v = ESelectBase<sym>::process(UUUUUUUU[i]);
					
					for ( uint64_t mask = 1ull << 63; mask; mask >>= 1, j++ )
						if ( v & mask )
						{
							if ( (o % sbones) == 0 )
							{
								++s;
								islargesuper = isLargeSuperBlock(s);
								assert ( S[s] == j );
							}
							if ( (!islargesuper) && (( o % mbones ) == 0) )
							{
								++m;
								islargemini = bitio::getBit(ML.get(), m);
								assert ( M[m] + S[s] == j );
							}
							if ( (!islargesuper) && (!islargemini) && ((o % mibones) == 0) )
							{
								++mi;					
							}
							o += 1;						
						}
				}
				++mi;

				MI = autoarray::AutoArray<uint16_t>(mi,false);

				// std::cerr << "Number of microblocks is " << mi << std::endl;

				s = -1; m = -1; mi = -1; j = 0; o = 0;
				islargesuper = false;
				islargemini = false;

				/**
				 * fill microblock boundaries
				 **/
				for ( uint64_t i = 0; i < numwords; ++i )
				{
					uint64_t const v = ESelectBase<sym>::process(UUUUUUUU[i]);
					
					for ( uint64_t mask = 1ull << 63; mask; mask >>= 1, j++ )
						if ( v & mask )
						{
							if ( (o % sbones) == 0 )
							{
								++s;
								islargesuper = isLargeSuperBlock(s);
								assert ( S[s] == j );
							}
							if ( (!islargesuper) && (( o % mbones ) == 0) )
							{
								++m;
								islargemini = bitio::getBit(ML.get(), m);
								assert ( M[m] + S[s] == j );
							}
							if ( (!islargesuper) && (!islargemini) && ((o % mibones) == 0) )
							{
								MI[++mi] = j - M[m] - S[s];
								assert ( j == S[s] + M[m] + MI[mi] );
							}
							o += 1;						
						}
				}
				++mi;
				
			}

			uint64_t byteSize() const
			{
				return 
					3*sizeof(uint64_t) +
					sizeof(uint64_t *) +
					S.byteSize() +
					SL.byteSize() +
					SLr->byteSize() +
					LS.byteSize() +
					M.byteSize() +
					ML.byteSize() +
					MLr->byteSize() +
					LM.byteSize() +
					MI.byteSize() +
					ESelectBase<sym>::R.byteSize();
			}

		};
	}
}
#endif
