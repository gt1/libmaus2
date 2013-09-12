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

#if ! defined(OSINDUCE_HPP)
#define OSINDUCE_HPP

#include <libmaus/suffixsort/SuccinctFactorList.hpp>
#include <libmaus/bitio/CompactArray.hpp>
#include <libmaus/util/IncreasingList.hpp>
#include <libmaus/wavelet/WaveletTree.hpp>
#include <libmaus/wavelet/toWaveletTreeBits.hpp>
#include <libmaus/math/isqrt.hpp>

namespace libmaus
{
	namespace suffixsort
	{
		// #define INDUCE_DEBUG

		enum induce_mode { mode_sort, mode_induce };

		template<induce_mode mode, typename iterator>
		::libmaus::bitio::CompactArray::unique_ptr_type induce(
			iterator it, // text
			uint64_t const n, // length of text
			uint64_t const b, // bits per symbol
			uint64_t const sentinel, // sentinel symbol
			::libmaus::suffixsort::SuccinctFactorList< iterator > & sast, // leftmost S type substrings/suffixes
			uint64_t zrank, // rank of position 0
			bool const leftmostIsL, // leftmost position in text is L type
			bool const verbose
		)
		{
			assert ( n );

			// alphabet size (2^b)
			uint64_t const k = n ? ((*std::max_element(it,it+n))+1) : 0;
			uint64_t const kfragsize = std::max ( static_cast<uint64_t>(1024ull) , ::libmaus::math::isqrt(k) );
			uint64_t const kruns = ( k + kfragsize - 1 ) / kfragsize;
			uint64_t const wtsize = sast.size();
			
			if ( verbose )
				std::cerr << "(n=" << n << ",k=" << k << ",kruns=" << kruns << ",kfragsize=" << kfragsize << ")";

			#if defined(INDUCE_DEBUG)
			std::cerr << "\n\n induce \n\n" << std::endl;
			#endif
			
			#if defined(INDUCE_DEBUG)
			std::cerr << "String: " << std::string(it,it+n) << std::endl;
			std::cerr << "S* (size " << wtsize << ", blocks " << (sast.bhigh-sast.blow) <<")" ":" << std::endl;
			sast.print(std::cerr);
			std::pair < uint64_t, uint64_t > eihist = sast.getStorageHistogram();
			std::cerr << "storage: explicit=" << eihist.first << " implicit=" << eihist.second;
			std::cerr << "\n\n";
			#endif

			typedef ::libmaus::suffixsort::SuccinctFactorList<iterator> list_type;
			typedef typename list_type::unique_ptr_type list_ptr_type;
			typedef ::libmaus::util::unique_ptr< std::vector < std::vector < uint8_t > > >::type VVptr;

			// character histogram
			::libmaus::autoarray::AutoArray<uint64_t> Clow( k+1 );
			for ( uint64_t i = 0; i < n; ++i )
				Clow [ it[i] ] ++;
			// compute prefix sums
			Clow.prefixSums();

			if ( verbose )
				std::cerr << "(Clow.bs=" << Clow.byteSize() << ")";
			
			// clones
			::libmaus::autoarray::AutoArray<uint64_t> Hlow;
			::libmaus::autoarray::AutoArray<uint64_t> Hhigh;

			/* BWT */
			::libmaus::bitio::CompactArray::unique_ptr_type BBB;
			::libmaus::bitio::CompactArray::unique_ptr_type E;
			#if defined(INDUCE_DEBUG)
			VVptr B_L;
			VVptr B_S;
			#endif
			uint64_t ecnt = 0;
			
			// allocate for computing BWT from sorted s* suffixes
			if ( mode == mode_induce )
			{
				::libmaus::bitio::CompactArray::unique_ptr_type tBBB(new ::libmaus::bitio::CompactArray(n,b));
				BBB = UNIQUE_PTR_MOVE(tBBB);
				::libmaus::bitio::CompactArray::unique_ptr_type tE(new ::libmaus::bitio::CompactArray(wtsize,b));
				E = UNIQUE_PTR_MOVE(tE);
			
				#if defined(INDUCE_DEBUG)
				B_L = VVptr(new std::vector < std::vector < uint8_t > >(k));
				B_S = VVptr(new std::vector < std::vector < uint8_t > >(k));
				#endif
				
				if ( verbose )
				{
					std::cerr << "(BBB->byteSize()=" << BBB->byteSize() << ")";
					std::cerr << "(E->byteSize()=" << E->byteSize() << ")";
				}
			}

			/* name computation */
			::libmaus::autoarray::AutoArray < uint64_t > LSbv;
			::libmaus::autoarray::AutoArray < uint64_t > PSast;
			::libmaus::autoarray::AutoArray < uint64_t > Psi;
			// sorted lists of s* substrings
			::libmaus::autoarray::AutoArray<list_ptr_type> sortedL;
			::libmaus::autoarray::AutoArray<list_ptr_type> sortedS;
			::libmaus::bitio::CompactArray::unique_ptr_type wtbase;
			// Phi lists
			::libmaus::util::IncreasingList::unique_ptr_type Phi;
			::libmaus::autoarray::AutoArray<uint64_t> PhiBase;
			// wavelet tree for s* substrings
			::libmaus::autoarray::AutoArray<uint64_t> wtbits;
			::libmaus::wavelet::WaveletTree< ::libmaus::rank::ERank222B,uint64_t>::unique_ptr_type wt;
			
			// histogram prefix sums
			uint64_t wtcnt = 0;
			::libmaus::autoarray::AutoArray<uint64_t> PhiBaseC;

			// allocate for sorting s* substrings
			if ( mode == mode_sort )
			{
				LSbv = ::libmaus::autoarray::AutoArray < uint64_t > ( (n+63)/64 );
				PSast = ::libmaus::autoarray::AutoArray < uint64_t > ( (n+63)/64 );
				Psi = ::libmaus::autoarray::AutoArray < uint64_t > ( (n+63)/64 );
				sortedL = ::libmaus::autoarray::AutoArray<list_ptr_type>( k );
				sortedS = ::libmaus::autoarray::AutoArray<list_ptr_type>( k );
				::libmaus::bitio::CompactArray::unique_ptr_type twtbase(new ::libmaus::bitio::CompactArray(wtsize,b));
				wtbase = UNIQUE_PTR_MOVE(twtbase);
				for ( uint64_t i = 0; i < sortedL.size(); ++i )
				{
					list_ptr_type tsortedLi(new list_type(it,n,b,sentinel));
					sortedL[i] = UNIQUE_PTR_MOVE(tsortedLi);
				}
				for ( uint64_t i = 0; i < sortedS.size(); ++i )
				{
					list_ptr_type tsortedSi(new list_type(it,n,b,sentinel));
					sortedS[i] = UNIQUE_PTR_MOVE(tsortedSi);
				}
				::libmaus::util::IncreasingList::unique_ptr_type tPhi(new ::libmaus::util::IncreasingList(n,b));
				Phi  = UNIQUE_PTR_MOVE(tPhi);
				Phi->put(0,0);
				Hlow = Clow.clone();
				Hhigh = ::libmaus::autoarray::AutoArray<uint64_t>(k+1,false);
				std::copy(Clow.begin()+1,Clow.end(),Hhigh.begin());

				#if defined(INDUCE_DEBUG)	
				if ( verbose )
					std::cerr 
						<< "LSbv=" << LSbv.byteSize() << " "
						<< "PSast=" << PSast.byteSize() << " "
						<< "Psi=" << Psi.byteSize() << " "
						<< "wtbase=" << wtbase->byteSize() << " "
						<< "Phi=" << Phi->byteSize() << " "
						<< std::endl;
				#endif

				PhiBase = Hhigh.clone();
			}

			/**
			 * terminator symbol
			 **/
			uint64_t const term = it[n-1];
			
			if ( mode == mode_sort )
			{
				for ( typename list_type::const_iterator ita =
					sast.begin(); ita != sast.end(); ++ita )
				{
					uint64_t const c = ita.peek();
					wtbase->set(wtcnt++,c);
					PhiBase[c]--;
				}

				// bucket sort by last character
				::libmaus::autoarray::AutoArray<list_ptr_type> Sast( k );
				for ( uint64_t i = 0; i < Sast.size(); ++i ) 
				{
					list_ptr_type Sasti(new list_type(it,n,b,sentinel));
					Sast[i] = UNIQUE_PTR_MOVE(Sasti);
				}

				/**
				 * copy given s* type substrings to their buckets
				 **/
				if ( verbose )
					std::cerr << "(distributing to S*...";
				while ( ! sast.empty() )
				{
					uint64_t const c = sast.peek();								
					Sast[c]->copy(sast);
					sast.pop();
				}
				if ( verbose )
					std::cerr << ")";
					
				if ( verbose )
					std::cerr << "(Copying back to sast...";

				for ( uint64_t i = 0; i < k; ++i )
					while ( ! Sast[i]->empty() )
					{
						sast.copy( *(Sast[i]) );
						Sast[i]->pop();
					}

				if ( verbose )
					std::cerr << ")" << std::endl;

				if ( verbose )
					std::cerr << "(constructing wt...";
				assert ( wtcnt == wtsize );
				wtbits = ::libmaus::wavelet::toWaveletTreeBits( wtbase.get(), false );
				::libmaus::wavelet::WaveletTree< ::libmaus::rank::ERank222B,uint64_t>::unique_ptr_type twt(new ::libmaus::wavelet::WaveletTree< ::libmaus::rank::ERank222B,uint64_t>(wtbits,wtsize,b));
				wt = UNIQUE_PTR_MOVE(twt);

				if ( verbose )
					std::cerr << ")";

				#if defined(INDUCE_DEBUG)
				for ( uint64_t i = 0; i < wtsize; ++i )
					std::cerr << static_cast<uint8_t>((*wt)[i]);
				std::cerr << std::endl;
				#endif

				// copy of PhiBase
				PhiBaseC = PhiBase.clone();
			}
			
			// turning point of leftmost s* substring if its first char is l type
			uint64_t ltp = 0;

			if ( verbose )
				std::cerr << "(inducing L type...";

			/* general */
			::libmaus::autoarray::AutoArray<list_ptr_type> Lex( kruns );
			for ( uint64_t i = 0; i < Lex.size(); ++i ) 
			{
				list_ptr_type tLexi(new list_type(it,n,b,sentinel));
				Lex[i] = UNIQUE_PTR_MOVE(tLexi);
			}
			
			list_ptr_type LS = list_ptr_type(new list_type(it,n,b,sentinel));
			
			for ( uint64_t krun = 0; krun < kruns; ++krun )
			{
				if ( verbose )
					std::cerr << "(" << (krun+1) << "/" << kruns;
			
				uint64_t const klow = krun * kfragsize;
				uint64_t const khigh = std::min(klow + kfragsize, k);

				#if 0
				::libmaus::autoarray::AutoArray<list_ptr_type> LSin( khigh-klow );
				for ( uint64_t i = 0; i < LSin.size(); ++i ) 
					LSin[i] = list_ptr_type(new list_type(it,n,b,sentinel));
				#endif

				if ( verbose )
					std::cerr << "(Setting up Lin...";

				::libmaus::autoarray::AutoArray<list_ptr_type> Lin( khigh-klow );
				for ( uint64_t i = 0; i < Lin.size(); ++i ) 
				{
					list_ptr_type tLin(new list_type(it,n,b,sentinel));
					Lin[i] = UNIQUE_PTR_MOVE(tLin);
				}
					
				while ( ! Lex[krun]->empty() )
				{
					uint64_t const c = Lex[krun]->peekPre();
					Lin [ c - klow ] -> copy ( *(Lex[krun]) );
					Lex[krun]->pop();
				}
				
				if ( verbose )
					std::cerr << ")";

				/**
				 * left to right run for inducing L type suffixes
				 **/
				for ( uint64_t i = klow; i < khigh; ++i )
				{
					#if defined(INDUCE_DEBUG)
					if ( ! Lin[i-klow]->empty() )
						std::cerr << "\n--- Entering L[" << static_cast<uint8_t>(i) << "] loop ---\n\n";
					#endif

					while ( ! Lin[i-klow]->empty() )
					{
						uint64_t const c = Lin[i-klow]->peek();
						// rank of this suffix
						uint64_t const r = Clow[i]++;
						
						if ( mode == mode_induce )
						{
							if ( c != sentinel )
							{
								#if defined(INDUCE_DEBUG)
								(*B_L)[i].push_back(c);
								#endif
								BBB -> set ( r, c );

								#if defined(INDUCE_DEBUG)
								std::cerr << "adding " << static_cast<uint8_t>(c) << " to B_L[" << static_cast<uint8_t>(i)
									<< "] as per L[" << static_cast<uint8_t>(i) << "]=" << Lin[i-klow]->printSingle() << std::endl;
								#endif
							}
							else
							{
								#if defined(INDUCE_DEBUG)
								(*B_L)[i].push_back(term);
								#endif
								BBB ->set ( r, term );
							}
						}

						#if defined(INDUCE_DEBUG)
						std::cerr << "moving " << Lin[i-klow]->printSingle() << " rank " << r << " from L[" << static_cast<uint8_t>(i) << "] to ";
						#endif
						
						if ( c < i )
						{
							#if defined(INDUCE_DEBUG)
							std::cerr << "LS[" << static_cast<uint8_t>(i) << "] as " << Lin[i-klow]->printSingle() << std::endl;
							#endif
							
							#if 0
							LSin[i-klow]->copy( *(Lin[i-klow]) );
							#endif
							LS->copy ( *(Lin[i-klow]) );
							
							if ( mode == mode_sort )
								::libmaus::bitio::putBit(LSbv.get(),r,1);
						}
						else
						{
							
							if ( c == sentinel )
							{
								if ( mode == mode_sort )
								{
									#if defined(INDUCE_DEBUG)
									std::cerr << "trash (reached sentinel)" << std::endl;
									#endif
									sortedL[i]->copyReset(*(Lin[i-klow]));
									::libmaus::bitio::putBit(PSast.get(),r,1);
									ltp = r;
								}
							}
							else
							{							
								Lin[i-klow]->cycle();
								
								if ( c / kfragsize == krun )
									Lin[c-klow]->copy( *(Lin[i-klow]) );
								else
									Lex[c/kfragsize] -> copy( *(Lin[i-klow]) );

								#if defined(INDUCE_DEBUG)
								std::cerr << "L[" << static_cast<uint8_t>(c) << "] as " << Lin[i-klow]->printSingle();
								#endif
														
								if ( mode == mode_sort )
								{
									uint64_t const tr = Hlow[c]++;

									#if defined(INDUCE_DEBUG)
									std::cerr << " rank " << tr;
									std::cerr << std::endl;
									#endif

									#if defined(INDUCE_DEBUG)
									std::cerr << "PhiL: c = " << static_cast<uint8_t>(c) << ": tr = " << tr << " -> " << " r = " << r << "(" << n*c+r << ")" << std::endl;
									#endif
									
									assert ( tr > r );

									// tr is L type, r is L type
									Phi->put (tr, c*n+r);
								}
								else
								{
									#if defined(INDUCE_DEBUG)
									std::cerr << std::endl;
									#endif
								}
							}
						}
									
						Lin[i-klow]->pop();
					}

					if ( (! sast.empty()) && (sast.peek() == i) )
					{
						#if defined(INDUCE_DEBUG)
						std::cerr << "\n--- Entering Sast[" << static_cast<uint8_t>(i) << "] loop ---\n\n";
						#endif
					}
					
					while ( (! sast.empty()) && (sast.peek() == i) )
					{
						sast.cycle();
					
						uint64_t const c = sast.peek();

						#if defined(INDUCE_DEBUG)
						std::cerr << "moving " << sast.printSingle() 
							<< " from S*[" << static_cast<uint8_t>(i) << "]" 
							<< " to L[" << static_cast<uint8_t>(c) << "]";
						#endif
						
						sast.cycle();
						
						#if defined(INDUCE_DEBUG)
						std::cerr << " as " << sast.printSingle();
						#endif
						
						if ( mode == mode_induce )
						{
							#if defined(INDUCE_DEBUG)
							std::cerr << std::endl;
							#endif

							// insert terminator at correct position in E
							if ( (!leftmostIsL) && (ecnt == zrank) )
								E->set(ecnt++,term);
								// E.push_back(term);

							E->set(ecnt++,c);
							// E.push_back(c);			

							#if defined(INDUCE_DEBUG)
							std::cerr << "Pushing letter " << static_cast<uint8_t>(c) << " to E." << std::endl;
							#endif
						}
						
						if ( c / kfragsize == krun )
							Lin[c-klow]->copy( sast );
						else
							Lex[c/kfragsize]->copy( sast );
						sast.pop();

						if ( mode == mode_sort )
						{
							uint64_t const r = PhiBase[i]++;
							uint64_t const tr = Hlow[c]++;

							#if defined(INDUCE_DEBUG)
							std::cerr << " from rank " << r << " to rank " << tr;
							#endif
							
							#if defined(INDUCE_DEBUG)
							std::cerr << std::endl;
							#endif
							
							// tr is L type, r is S type (S*)
							Phi->put(tr, c*n+r);
							assert ( tr > r );

							#if defined(INDUCE_DEBUG)
							std::cerr << "PhiL: c = " << static_cast<uint8_t>(c) << ": tr = " << tr << " -> " << " r = " << r << "(" << n*c+r << ")" << std::endl;
							#endif
							
							// mark rank of unsorted substring
							::libmaus::bitio::putBit( Psi.get() , r, true );
						}
					}
				}

				#if 0
				if ( verbose )
					std::cerr << "(Copying LS...";
				for ( uint64_t j = 0; j < LSin.size(); ++j ) 
					while ( !LSin[j]->empty() )
					{
						LS -> copy ( *(LSin[j]) );
						LSin[j]->pop();
					}
				if ( verbose )
					std::cerr << "done)";
				#endif
				
				
				if ( verbose )
					std::cerr << ")";
			}
			
			Lex = ::libmaus::autoarray::AutoArray<list_ptr_type>();
			Clow.release();
			
			if ( verbose )
				std::cerr << "(Reversing LS...";
				
			LS->reverse();
			
			if ( verbose )
				std::cerr << ")";
						
			if ( verbose )
				std::cerr << ")";

			// insert terminator at end if leftmost (position 0) is largest suffix
			if ( mode == mode_induce )
			{
				if ( (!leftmostIsL) && (ecnt == zrank) )
				{
					E->set(ecnt++,term);
					// E.push_back(term);
				}
			}

			#if defined(INDUCE_DEBUG)
			std::cerr << "\f";
			#endif

			#if defined(INDUCE_DEBUG)
			if ( mode == mode_sort )
			{
				std::cerr << "E=" << std::string(
					::libmaus::bitio::CompactArray::const_iterator(E.get()),
					::libmaus::bitio::CompactArray::const_iterator(E.get()) + ecnt
					) << std::endl;
			}
			#endif
			
			
			#if defined(INDUCE_DEBUG)
			std::cerr << "\n\n*****\n\n";
			#endif
			
			uint64_t lsbvcnt = n;

			if ( verbose )
				std::cerr << "(inducing S type...";

			::libmaus::autoarray::AutoArray<list_ptr_type> Sex( kruns );
			for ( uint64_t i = 0; i < Sex.size(); ++i ) 
			{
				list_ptr_type tSexi(new list_type(it,n,b,sentinel));
				Sex[i] = UNIQUE_PTR_MOVE(tSexi);
			}
			Sex[term/kfragsize]->push(n-1,n);
			Sex[term/kfragsize]->cycle();

			// character histogram shifted by one
			::libmaus::autoarray::AutoArray<uint64_t> Chigh( k+1 );
			for ( uint64_t i = 0; i < n; ++i )
				Chigh [ it[i] ] ++;
			// compute prefix sums
			Chigh.prefixSums();
			for ( uint64_t i = 1; i < Chigh.size(); ++i )
				Chigh [ i - 1 ] = Chigh[i];

			#if defined(INDUCE_DEBUG)
			std::cerr << "Pushed " << Sex[term/kfragsize]->printSingle() << " into S[" << static_cast<uint8_t>(term) << "]" << std::endl;
			#endif

			/*
			 * right to left for inducing s type suffixes
			 */
			for ( uint64_t ikrun = 0; ikrun < kruns; ++ikrun )
			{
				if ( verbose )
					std::cerr << "(" << (ikrun+1) << "/" << kruns;
					
				uint64_t const krun = kruns-ikrun-1;

				uint64_t const klow = krun * kfragsize;
				uint64_t const khigh = std::min ( klow + kfragsize , k );
				uint64_t const ksize = khigh-klow;

				if ( verbose )
					std::cerr << "(Setting up Sin...";

				::libmaus::autoarray::AutoArray<list_ptr_type> Sin( khigh-klow );
				for ( uint64_t i = 0; i < Sin.size(); ++i ) 
				{
					list_ptr_type Sini(new list_type(it,n,b,sentinel));
					Sin[i] = UNIQUE_PTR_MOVE(Sini);
				}
					
				while ( ! Sex[krun]->empty() )
				{
					uint64_t const c = Sex[krun]->peekPre();
					Sin [ c - klow ] -> copy ( *(Sex[krun]) );
					Sex[krun]->pop();
				}
				
				if ( verbose )
					std::cerr << ")";

			
				for ( uint64_t ii = 0; ii < ksize; ++ii )
				{
					uint64_t const i = khigh-ii-1;
					
					if ( ! Sin[i-klow]->empty() )
					{
						#if defined(INDUCE_DEBUG)
						std::cerr << "\n--- Entering S[" << static_cast<uint8_t>(i) << "] loop ---\n\n";
						std::cerr << "strings\n";
						Sin[i-klow]->print(std::cerr);
						#endif
					}
					
					while ( ! Sin[i-klow]->empty() )
					{
						#if defined(INDUCE_DEBUG)
						std::cerr << "processing " << Sin[i-klow]->printSingle() << std::endl;
						#endif
					
						uint64_t const c = Sin[i-klow]->peek();
						uint64_t const r = --Chigh[i];

						if ( mode == mode_induce )
						{
							if ( c <= i )
							{
								#if defined(INDUCE_DEBUG)
								std::cerr << "adding " << static_cast<uint8_t>(c) << " to B_S[" << static_cast<uint8_t>(i) << "] as per "
									<< Sin[i-klow]->printSingle() << std::endl;
								#endif

								#if defined(INCUDE_DEBUG)
								B_S[i].push_back(c);
								#endif

								BBB -> set( r, c );
							}
							else
							{
								assert ( ecnt );
								
								uint64_t const c2 = E->get(--ecnt); // E.back(); E.pop_back();
							
								#if defined(INDUCE_DEBUG)
								std::cerr << "adding " << static_cast<uint8_t>(c2) << " to B_S[" << static_cast<uint8_t>(i) << "] as per "
									<< " reaching sentinel on " << Sin[i-klow]->printSingle() << std::endl;
								#endif
								#if defined(INDUCE_DEBUG)
								(*B_S)[i].push_back(c2);
								#endif

								BBB -> set ( r , c2 );
							}
						}

						#if defined(INDUCE_DEBUG)
						std::cerr << "moving " << Sin[i-klow]->printSingle() << " rank " << r << " from S[" << static_cast<uint8_t>(i) << "] to ";
						#endif

						if ( c <= i )
						{
							Sin[i-klow]->cycle();
							
							#if defined(INDUCE_DEBUG)
							std::cerr << "S[" << static_cast<uint8_t>(c) << "] as " << Sin[i-klow]->printSingle();
							#endif
					
							if ( c/kfragsize == krun )
								Sin[c-klow]->copy( *(Sin[i-klow]) );
							else
								Sex[c/kfragsize]->copy(*(Sin[i-klow]));
							
							if ( mode == mode_sort )
							{
								uint64_t const tr = --Hhigh[c];
								assert ( tr < r );
								
								#if defined(INDUCE_DEBUG)
								std::cerr << " rank " << tr;
								std::cerr << std::endl;
								#endif

								Phi->put  ( tr, n*c+r);
								#if defined(INDUCE_DEBUG)
								std::cerr << "PhiS: c = " << static_cast<uint8_t>(c) << ": tr = " << tr << " -> " << " r = " << r << "(" << n*c+r << ")" << std::endl;
								#endif
							}
							else
							{
								#if defined(INDUCE_DEBUG)
								std::cerr << std::endl;
								#endif
							}
						}
						else
						{
							#if defined(INDUCE_DEBUG)
							std::cerr << "trash (reached sentinel)" << std::endl;
							#endif
							
							if ( mode == mode_sort )
							{
								sortedS[i]->copyReset(*(Sin[i-klow]));
								::libmaus::bitio::putBit(PSast.get(),r,1);
							}
						}
						
						Sin[i-klow]->pop();
					}
					
					#if 0
					if ( ! LS[i]->empty() )
					{
						#if defined(INDUCE_DEBUG)
						std::cerr << "\n--- Entering LS[" << static_cast<uint8_t>(i) << "] loop ---\n\n";
						std::cerr << "strings\n";
						LS[i]->print(std::cerr);
						#endif		
					}
					
					while ( ! LS[i]->empty() )
					{
						uint64_t const c = LS[i]->peek();
						assert ( c != sentinel );

						if ( mode == mode_sort )
						{
							while ( ! ::libmaus::bitio::getBit(LSbv.get(),--lsbvcnt) ) {}
						}

						#if defined(INDUCE_DEBUG)
						std::cerr << "moving " << LS[i]->printSingle() << " from LS[" << static_cast<uint8_t>(i) << "] ";
						if ( mode == mode_sort )
						{
							std::cerr << "rank " << lsbvcnt;
						}
						std::cerr << " to ";
						#endif
						LS[i]->cycle();

						#if defined(INDUCE_DEBUG)
						std::cerr << "S[" << static_cast<uint8_t>(c) << "] as " << LS[i]->printSingle();
						#endif
						S[c]->copy( *(LS[i]) );
						LS[i]->pop();
						
						if ( mode == mode_sort )
						{
							uint64_t const r = lsbvcnt;
							uint64_t const tr = --Hhigh[c];
							assert ( tr < r );

							#if defined(INDUCE_DEBUG)
							std::cerr << " rank " << tr;
							std::cerr << std::endl;
							#endif

							#if defined(INDUCE_DEBUG)			
							std::cerr << "PhiS: c = " << static_cast<uint8_t>(c) << ": tr = " << tr << " -> " << " r = " << r << "(" << n*c+r << ")" << std::endl;
							#endif
							Phi->put  ( tr, n*c+r );
						}
					}
					#else
					if ( (! LS->empty()) && (LS->peekPre() == i) )
					{
						#if defined(INDUCE_DEBUG)
						std::cerr << "\n--- Entering LS[" << static_cast<uint8_t>(i) << "] loop ---\n\n";
						std::cerr << "strings\n";
						LS->print(std::cerr);
						#endif		
					}
					
					while ( (! LS->empty()) && (LS->peekPre() == i) )
					{
						uint64_t const c = LS->peek();
						assert ( c != sentinel );

						if ( mode == mode_sort )
						{
							while ( ! ::libmaus::bitio::getBit(LSbv.get(),--lsbvcnt) ) {}
						}

						#if defined(INDUCE_DEBUG)
						std::cerr << "moving " << LS->printSingle() << " from LS[" << static_cast<uint8_t>(i) << "] ";
						if ( mode == mode_sort )
						{
							std::cerr << "rank " << lsbvcnt;
						}
						std::cerr << " to ";
						#endif
						LS->cycle();

						#if defined(INDUCE_DEBUG)
						std::cerr << "S[" << static_cast<uint8_t>(c) << "] as " << LS->printSingle();
						#endif
						if ( c/kfragsize == krun )
							Sin[c-klow]->copy( *(LS) );
						else
							Sex[c/kfragsize]->copy ( *LS );
							
						LS->pop();
						
						if ( mode == mode_sort )
						{
							uint64_t const r = lsbvcnt;
							uint64_t const tr = --Hhigh[c];
							assert ( tr < r );

							#if defined(INDUCE_DEBUG)
							std::cerr << " rank " << tr;
							std::cerr << std::endl;
							#endif

							#if defined(INDUCE_DEBUG)			
							std::cerr << "PhiS: c = " << static_cast<uint8_t>(c) << ": tr = " << tr << " -> " << " r = " << r << "(" << n*c+r << ")" << std::endl;
							#endif
							Phi->put  ( tr, n*c+r );
						}
					}
					#endif
				}
			}
			if ( verbose )
				std::cerr << ")";
			
			if ( mode == mode_sort )
			{
				if ( leftmostIsL )
					::libmaus::bitio::putBit(LSbv.get(),ltp,1);
				::libmaus::bitio::putBit(LSbv.get(),0,1);
				
				#if defined(INDUCE_DEBUG)
				std::cerr << "Turning points: ";
				for ( uint64_t i = 0; i < n; ++i )
					if ( ::libmaus::bitio::getBit(LSbv.get(),i) )
						std::cerr << i << ";";
				std::cerr << std::endl;
				#endif

				if ( verbose )
					std::cerr << "(producing sorted list...";
				list_type sorted(it,n,b,sentinel);
				for ( uint64_t i = 0; i < (k); ++i )
				{
					while ( !sortedL[i]->empty() )
					{
						sorted.copyReset( *(sortedL[i]) );
						sortedL[i]->pop();
					}
					sortedS[i]->reverse();
					while ( !sortedS[i]->empty() )
					{
						sorted.copyReset( *(sortedS[i]) );
						sortedS[i]->pop();
					}
				}
				if ( verbose )
					std::cerr << ")";
				
				typename list_type::const_iterator sflc, sflcp;
				uint64_t urank = 0, urankcnt = 1;

				if ( verbose )
					std::cerr << "(counting ranks...";
				sflc = sorted.begin(), sflcp = sorted.begin(); sflc++;
				for ( ; sflc != sorted.end(); ++sflc, ++sflcp )
					if ( sflcp < sflc )
						urankcnt++;
				if ( verbose )
					std::cerr << ")";

				unsigned int const urankbits = ::libmaus::math::numbits(urankcnt-1);
				::libmaus::bitio::CompactArray::unique_ptr_type reduced(new ::libmaus::bitio::CompactArray(sorted.size(),urankbits));

				#if defined(INDUCE_DEBUG)
				std::cerr << "reduced alphabet size " << urankcnt << " bits "<< urankbits << std::endl;
				#endif
				
				if ( verbose )
					std::cerr << "(setting up Phi...";

				Phi->setup();

				if ( verbose )
					std::cerr << ")";

				if ( verbose )
					std::cerr << "(computing reduced string...";
				#if defined(INDUCE_DEBUG)
				std::cerr << "Ranks of sorted S* substrings:" << std::endl;
				#endif
				sflc = sorted.begin(), sflcp = sorted.begin();
				for ( uint64_t i = 0; i < n; ++i )
				{
					if ( (i & (1024*1024-1)) == 0 )
						if ( verbose )
							std::cerr << "(" << (i/(1024*1024)) << ")";
				
					if ( ::libmaus::bitio::getBit(PSast.get(),i) )
					{
						#if defined(INDUCE_DEBUG)
						std::cerr << i << ": ";
						#endif

						if ( i != 0 )
						{
							
							uint64_t j = i;
							while ( ! ::libmaus::bitio::getBit(LSbv.get(),j) )
							{
								j = (*Phi)[j] % n;
								#if defined(INDUCE_DEBUG)
								std::cerr << j << ";";
								#endif
							}
							#if defined(INDUCE_DEBUG)
							std::cerr << "*";
							#endif
							while ( ! ::libmaus::bitio::getBit(Psi.get(),j) )
							{
								j = (*Phi)[j] % n;
								
								#if defined(INDUCE_DEBUG)
								std::cerr << j << ";";
								#endif
							}
							
							#if defined(INDUCE_DEBUG)
							// std::cerr << "y=" << y << " size " << sflc.size() << std::endl;
							#endif
							
							uint64_t const c = sflc.peek();
							uint64_t const relrank = j - PhiBaseC[c];
							uint64_t const sastpos = wt->select(c,relrank);

							#if defined(INDUCE_DEBUG)
							std::cerr << " peek " << static_cast<uint8_t>(c);
							std::cerr << " relrank " << relrank << " sastpos " << sastpos;
							#endif
							
							if ( sflcp < sflc )
								urank++;
								
							reduced->set( sastpos, urank );
							
							++sflcp;
						}
						else
						{
							#if defined(INDUCE_DEBUG)
							std::cerr << " sastpos " << wtsize;
							#endif
							
							// set terminator of reduced string
							reduced->set(wtsize,0);
						}
						
						#if defined(INDUCE_DEBUG)
						uint64_t const l = sflc.size();
						std::cerr << " ";
						sflc.print(std::cerr);
						std::cerr << " ";
						for ( uint64_t j = 0; j < l; ++j )
							std::cerr << static_cast<uint8_t>(sflc[j]);
						std::cerr << " (" << l << ")";
						
						std::cerr << " " << "urank=" << urank;
						
						std::cerr << std::endl;
						#endif
				
						++sflc;
					}
				}
				if ( verbose )
					std::cerr << ")";
				#if defined(INDUCE_DEBUG)
				std::cerr << std::endl;
				#endif
				
				#if defined(INDUCE_DEBUG)
				std::cerr << "reduced ";
				for ( uint64_t i = 0; i < reduced->size(); ++i )
					std::cerr << "(" << reduced->get(i) << ")";
				std::cerr << std::endl;
				#endif
				
				#if defined(INDUCE_DEBUG)
				std::cerr << "Sorted S* type " << std::endl;
				sorted.print(std::cerr);
				#endif
				
				return UNIQUE_PTR_MOVE(reduced);
			}
			else
			{
				#if defined(INDUCE_DEBUG)	
				std::vector < uint8_t > B;
				std::vector < uint8_t > Bp;
				for ( uint64_t i = 0; i < (k); ++i )
				{
					if ( (*B_L)[i].size() )
					{
						B.push_back('L');	
						B.push_back(toupper(i));
						for ( uint64_t j = 0; j < (*B_L)[i].size(); ++j )
							B.push_back ( (*B_L)[i][j] );
						for ( uint64_t j = 0; j < (*B_L)[i].size(); ++j )
							Bp.push_back ( (*B_L)[i][j] );
					}
					if ( (*B_S)[i].size() )
					{
						B.push_back('S');	
						B.push_back(toupper(i));
						for ( uint64_t j = 0; j < (*B_S)[i].size(); ++j )
							B.push_back ( (*B_S)[i][(*B_S)[i].size()-j-1] );		
						for ( uint64_t j = 0; j < (*B_S)[i].size(); ++j )
							Bp.push_back ( (*B_S)[i][(*B_S)[i].size()-j-1] );		
					}
				}
				#endif

				#if defined(INDUCE_DEBUG)
				for ( uint64_t i = 0; i < B.size(); ++i )
					std::cerr << B[i];
				std::cerr << std::endl;
				#endif

				#if defined(INDUCE_DEBUG)
				std::cerr << std::string(Bp.begin(),Bp.end()) << std::endl;
				#endif
				
				#if defined(INCUDE_DEBUG)
				assert ( Bp.size() == BBB->size() );
				for ( uint64_t i = 0; i < BBB->size(); ++i )
					assert ( Bp[i] == BBB->get(i) );
				#endif
					
				return UNIQUE_PTR_MOVE(BBB);
			}
		}
	}
}
#endif
