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

#if !defined(GENERICEDITDISTANCE_HPP)
#define GENERICEDITDISTANCE_HPP

#include <libmaus/lcs/GenericAlignmentPrint.hpp>
#include <libmaus/types/types.hpp>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/exception/LibMausException.hpp>
#include <libmaus/lcs/EditDistanceResult.hpp>
#include <libmaus/lcs/BaseConstants.hpp>
#include <libmaus/lcs/PenaltyConstants.hpp>
#include <libmaus/lcs/TraceContainer.hpp>
#include <map>
#include <iostream>
#include <iomanip>

namespace libmaus
{
	namespace lcs
	{		
		struct GenericTraceContainer : public TraceContainer, public GenericAlignmentPrint
		{
			GenericTraceContainer(uint64_t const tracelen) : TraceContainer(tracelen) {}

			template<typename iterator>
			std::ostream & printAlignment(
				std::ostream & out, 
				iterator ita,
				iterator itb
			) const
			{
				GenericAlignmentPrint::printAlignment(out,ita,itb,ta,te);
				return out;
			}
			std::ostream & printAlignmentLines(
				std::ostream & out, std::string const & a, std::string const & b,
				uint64_t const rlinewidth
			) const
			{
				GenericAlignmentPrint::printAlignmentLines(out,a,b,rlinewidth,ta,te);
				return out;
			}
		};
		
		struct GenericEditDistance : public GenericTraceContainer
		{
			typedef GenericEditDistance this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
		
			private:
			uint64_t const n; // rows
			uint64_t const m; // columns
			uint64_t const n1;
			uint64_t const m1;

			// matrix stored column wise
			typedef std::pair < similarity_type, step_type > element_type;
			::libmaus::autoarray::AutoArray<element_type> M;

			public:
			GenericEditDistance(uint64_t const rn, uint64_t const rm, uint64_t const)
			: GenericTraceContainer(rn+rm), n(rn), m(rm), n1(n+1), m1(m+1), M( n1*m1, false )
			{
				// std::cerr << "n=" << n << " m=" << m << " n1=" << n1 << " m1=" << m1 << std::endl;
			}
			
			template<typename iterator_a, typename iterator_b>
			EditDistanceResult process(
				iterator_a a, iterator_b b,
				uint64_t const,
				uint64_t const,
				similarity_type const first_del_mult = 1,
				similarity_type const first_ins_mult = 1
				)
			{
				element_type * p = M.begin();
				
				// fill first column with zeroes
				*(p++) = element_type(0,STEP_MATCH);
				for ( uint64_t i = 1; i < n1; ++i )
					*(p++) = element_type(-i*penalty_ins*first_ins_mult,STEP_INS);
				
				// p = current column	
				// q = previous column
				element_type * q = M.begin();
				
				// fill dynamic programming matrix
				iterator_b const ba = b;
				iterator_b const be = b+m;
				iterator_a const ae = a+n;
				while ( b != be )
				{
					typename std::iterator_traits<iterator_b>::value_type const bchar = *(b++);
				
					#if 0
					assert ( (p-M.begin()) % n1 == 0 );
					assert ( (q-M.begin()) % n1 == 0 );
					#endif
				
					p->first = q->first - penalty_del * first_del_mult; // *
					p->second = STEP_DEL;
					
					// iterate over rows
					for ( iterator_a aa = a; aa != ae; ++aa )
					{
						// above
						similarity_type const top = (p++)->first - penalty_ins;
						// diagonal
						similarity_type const d = ((*aa) == bchar) ? gain_match : (-penalty_subst);
						similarity_type const diag = (q++)->first + d;
						// left
						similarity_type const left = q->first - penalty_del;
						
						if ( diag >= top )
						{
							if ( diag >= left )
								*p = element_type(diag,(d == gain_match)?STEP_MATCH:STEP_MISMATCH);
							else
								*p = element_type(left,STEP_DEL);
						}
						// diag < top
						else
						{
							if ( left >= top )
								*p = element_type(left,STEP_DEL);
							else
								*p = element_type(top,STEP_INS);
						}

					}

					p++;
					q++;
				}
				
				/* fill trace back array and build operation histogram */
				uint64_t i = n;
				uint64_t j = m;
				element_type * pq = M.get() + j*n1 + i;
				
				ta = te;
				
				uint64_t numdel = 0;
				uint64_t numin = 0;
				uint64_t nummat = 0;
				uint64_t nummis = 0;
				
				while ( pq != M.begin() )
				{
					*(--ta) = pq->second;
				
					switch ( pq->second )
					{
						case STEP_DEL:
							// go to previous column
							pq -= n1;
							numdel++;
							break;
						case STEP_INS:
							pq -= 1;
							numin++;
							break;
						case STEP_MATCH:
							pq -= (n1+1);
							nummat++;
							break;
						case STEP_MISMATCH:
							pq -= (n1+1);
							nummis++;
							break;
						default:
							break;
					}
				}

				return EditDistanceResult(numin,numdel,nummat,nummis);
			}
			
		};
		
		struct BandedEditDistance : public GenericTraceContainer
		{
			typedef BandedEditDistance this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
		
			private:
			uint64_t const n; // columns
			uint64_t const n1; // n+1
			uint64_t const k;
			uint64_t const k21;

			// matrix stored column wise
			typedef std::pair < similarity_type, step_type > element_type;
			::libmaus::autoarray::AutoArray<element_type> M;
						
			public:
			BandedEditDistance(uint64_t const rn, uint64_t const rm, uint64_t const rk)
			: GenericTraceContainer(rn+rm), n(rn), n1(n+1), k(rk), k21(2*k+1), M( n1 * k21 )
			{
				// std::cerr << "n=" << n << " m=" << m << " n1=" << n1 << " m1=" << m1 << std::endl;
			}

			void printMatrix(uint64_t const m) const
			{
				std::cerr << "matrix" << std::endl;
			
				std::map < std::pair<int64_t,int64_t>, element_type > E;
				
				// column
				for ( int64_t i = 0; i <= static_cast<int64_t>(n); ++i )
				{
					element_type const * p = M.begin() + i*k21;
					
					// row
					for ( int64_t j = i - static_cast<int64_t>(k); j <= i+static_cast<int64_t>(k); ++j, ++p )
					{
						if ( j >= 0 && j <= static_cast<int64_t>(m) )
						{
							E [ std::pair<int64_t,int64_t>(j,i) ] = *(p);
						}
					}
				}
				
				// row
				for ( int64_t j = 0; j <= static_cast<int64_t>(m); ++j )
				{
					// std::cerr << "row " << j << std::endl;
				
					for ( int64_t i = 0; i <= static_cast<int64_t>(n); ++i )
					{
						if ( 
							E.find(
								std::pair<int64_t,int64_t>(j,i)
							) != E.end() 
						)
						{
							element_type const e = E.find(std::pair<int64_t,int64_t>(j,i))->second;
							std::cerr << std::setw(3) << (e.first%100) << GenericAlignmentPrint::stepToString(e.second);
						}
						else
							std::cerr << std::setw(4)<< " ";
					}
					std::cerr << std::setw(0) << std::endl;
				}
			}

			static bool validParameters(uint64_t const n, uint64_t const m, uint64_t const k)
			{
				return 
					(m!=0)
					&&
					(n!=0)
					&&
					(k!=0)
					&&
					(m+1) > (2*k+1) && (n+1) > (k+1)
					&&
					std::max(m,n)-std::min(m,n) <= k
				;
			}
			
			static uint64_t maxk(uint64_t const n, uint64_t const m)
			{
				if ( ! m*n )
					return 0;
				else
					return std::min ( (m+1)/2-1,n-1 );
			}

			/**
			 * processing
			 **/
			template<typename iterator_a, typename iterator_b>
			EditDistanceResult process(
				iterator_a a, 
				iterator_b b, 
				uint64_t const,
				uint64_t const m,
				similarity_type const first_del_mult = 1,
				similarity_type const first_ins_mult = 1)
			{
				// p = current column	
				// q = previous column
				element_type * p = M.begin();
				element_type * q = M.begin();	
				
				if ( k < std::max(n,m)-std::min(n,m) )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Distance allowed does not allow difference in string length in "
						<< ::libmaus::util::Demangle::demangle<this_type>() << "::process()";
					se.finish();
					throw se;
				}
				if ( ! validParameters(n,m,k) )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Invalid parameters in "
						<< ::libmaus::util::Demangle::demangle<this_type>() << "::process() (k too large)";
					se.finish();
					throw se;					
				}

				// fill first column
				p += k;
				(*p++) = element_type(0,STEP_MATCH);
				for ( uint64_t i = 0; i < k; ++i )
					*(p++) = element_type(-(i+1)*penalty_del*first_del_mult,STEP_DEL);
					
				// check for n
				uint64_t firstlooplimit = std::min(k,n1);
				for ( uint64_t i = 0; i < firstlooplimit; ++i )
				{
					// std::cerr << "i=" << i << std::endl;
				
					typename ::std::iterator_traits<iterator_a>::value_type const ca = a[i];
					#if defined(BANDEDLCSDEBUG)
					std::cerr << "\n---\na[" << i << "]=" << a[i] << std::endl;
					#endif

					assert ( (p - M.begin()) % k21 == 0 );
					assert ( (q - M.begin()) % k21 == 0 );
				
					q += (k-i);
					p += ((k-i)-1);
					
					#if defined(BANDEDLCSDEBUG)
					std::cerr << "---" << std::endl;
					std::cerr << "p-shift " << ((k-i)-1) << std::endl;
					std::cerr << "q-shift " << (k-i) << std::endl;
					#endif
										
					// first row has only left
					*p = element_type(q->first - penalty_ins*first_ins_mult,STEP_INS);
					
					uint64_t const secondlooplimit = std::min(k+i,m);
					for ( uint64_t j = 0; j < secondlooplimit; ++j )
					{
						typename ::std::iterator_traits<iterator_b>::value_type const cb = b[j];
						#if defined(BANDEDLCSDEBUG)
						std::cerr << "b[" << j << "]=" << cb << std::endl;
						#endif
						
						similarity_type const d = (ca==cb)?gain_match:-penalty_subst;
						
						similarity_type const diag = (q++)->first + d;
						similarity_type const left = q->first - penalty_ins;
						similarity_type const top = (p++)->first - penalty_del;
						
						if ( diag >= left )
						{
							if ( diag >= top )
								*p = element_type(diag,(ca==cb) ? STEP_MATCH : STEP_MISMATCH);
							else
								*p = element_type(top,STEP_DEL);
						}
						else
						{
							if ( top >= left )
								*p = element_type(top,STEP_DEL);
							else
								*p = element_type(left,STEP_INS);
						}
					}
					if ( secondlooplimit < k+i )
					{
						p += k+i-secondlooplimit;
						q += k+i-secondlooplimit;
					}

					if ( k+i < m )
					{
						// final row has no left neighbour	
						typename ::std::iterator_traits<iterator_b>::value_type const cb = b[k+i];
						#if defined(BANDEDLCSDEBUG)
						std::cerr << "b[" << k+i << "]=" << cb << std::endl;
						#endif

						similarity_type const d = (ca==cb)?gain_match:-penalty_subst;	
						similarity_type const diag = (q++)->first + d;
						similarity_type const top = (p++)->first - penalty_del;
							
						if ( diag >= top )
							*p = element_type(diag,(ca==cb) ? STEP_MATCH : STEP_MISMATCH);
						else
							*p = element_type(top,STEP_DEL);
					}
					else
					{
						p++;
						q++;
					}

					p++;

					/*
					std::cerr << "qdif = " << (q-M.begin()) % k21 << std::endl;
					std::cerr << "pdif = " << (p-M.begin()) % k21 << std::endl;
					std::cerr << "k21 = " << k21 << std::endl;
					*/
				}

				assert ( (p - M.begin()) % k21 == 0 );
				assert ( (q - M.begin()) % k21 == 0 );

				bool havefullcols = (m+1) > k21 && n1 > (k+1);
				uint64_t const fullcols = havefullcols ? std::min((m+1)-k21,n1-(k+1)) : 0;

				element_type * tt = 0;
				
				if ( havefullcols )
				{
					// std::cerr << "full cols " << fullcols << std::endl;
					for ( uint64_t i = k; i < k+fullcols; ++i )
					{
						typename ::std::iterator_traits<iterator_a>::value_type const ca = a[i];
						#if defined(BANDEDLCSDEBUG)
						std::cerr << "\n---\na[" << i << "]=" << a[i] << std::endl;
						#endif

						assert ( (p - M.begin()) % k21 == 0 );
						assert ( (q - M.begin()) % k21 == 0 );
						assert ( p != M.end() );
						assert ( q != M.end() );

						// std::cerr << "j start " << i-k << std::endl;

						// first row has only diagonal
						//uint64_t j = i - k;
						// std::cerr << "j=" << j << std::endl;
						
						// std::cerr << "*** i-k = " << i-k << std::endl;
						
						#if defined(BANDEDLCSDEBUG)
						std::cerr << "b[" << i-k << "]=" << b[i-k] << std::endl;
						#endif
						if ( b[i-k] == ca )
						{
							similarity_type const diag = (q++)->first + gain_match;
							similarity_type const left = q->first - penalty_ins;

							if ( diag >= left )
								*p = element_type(diag,STEP_MATCH);
							else
								*p = element_type(left,STEP_INS);						}
						else
						{
							similarity_type const diag = (q++)->first - penalty_subst;
							similarity_type const left = q->first - penalty_ins;
							
							if ( diag >= left )
								*p = element_type(diag,STEP_MISMATCH);
							else
								*p = element_type(left,STEP_INS);							
						}
						
						for ( uint64_t j = i-k+1; j < i-k+k21-1; ++j )
						{
							typename ::std::iterator_traits<iterator_b>::value_type const cb = b[j];
							#if defined(BANDEDLCSDEBUG)
							std::cerr << "b[" << j << "]=" << b[j] << std::endl;
							#endif
							
							similarity_type const d = (ca == cb) ? gain_match : (-penalty_subst);
							similarity_type const diag = (q++)->first + d;
							similarity_type const left = q->first - penalty_ins;
							similarity_type const top = (p++)->first - penalty_del;
							
							if ( diag >= left )
							{
								if ( diag >= top )
									*p = element_type(diag,(ca==cb) ? STEP_MATCH : STEP_MISMATCH);
								else
									*p = element_type(top,STEP_DEL);
							}
							else
							{
								if ( top >= left )
									*p = element_type(top,STEP_DEL);
								else
									*p = element_type(left,STEP_INS);
							}
						}
						
						// last row
						typename ::std::iterator_traits<iterator_b>::value_type const cb = b[i-k+k21-1];
						#if defined(BANDEDLCSDEBUG)
						std::cerr << "b[" << i-k+k21-1 << "]=" << b[i-k+k21-1] << std::endl;
						#endif
						similarity_type const d = (ca == cb) ? gain_match : (-penalty_subst);
						similarity_type const diag = (q++)->first + d;
						similarity_type const top = (p++)->first - penalty_del;
						
						if ( diag >= top )
							*p = element_type(diag,(ca==cb)?STEP_MATCH:STEP_MISMATCH);
						else
							*p = element_type(top,STEP_DEL);
							
						tt = p;

						p++;
						
						// *p = element_type(q->first - penalty_ins,STEP_INS);
						
						/*
						std::cerr << "pmod " << (p - M.begin()) % k21 << std::endl;
						std::cerr << "qmod " << (q - M.begin()) % k21 << std::endl;
						*/
					}
				}
				
				#if defined(BANDEDLCSDEBUG)
				printMatrix(m);
				#endif
				
				#if defined(BANDEDLCSDEBUG)
				std::cerr << "n=" << n << " fullcols=" << fullcols << " k=" << k << " firstlooplimit=" << firstlooplimit << std::endl;
				#endif
				
				for ( uint64_t i = firstlooplimit + fullcols; i < n; ++i )
				{
					// std::cerr << "Process column " << (i+1) << std::endl;
					
					// std::cerr << "j start " << i-firstlooplimit << std::endl;
					
					if ( i-firstlooplimit < m )
					{
						typename ::std::iterator_traits<iterator_a>::value_type const ca = a[i];
						#if defined(BANDEDLCSDEBUG)
						std::cerr << "\n---\na[" << i << "]=" << a[i] << std::endl;
						#endif

						assert ( (p - M.begin()) % k21 == 0 );
						assert ( (q - M.begin()) % k21 == 0 );
						
						#if defined(BANDEDLCSDEBUG)
						std::cerr << "b[" << i-firstlooplimit << "]=" << b[i-firstlooplimit] << std::endl;
						#endif
						if ( b[i-firstlooplimit] == ca )
						{
							similarity_type const diag = (q++)->first + gain_match;
							similarity_type const left = q->first - penalty_ins;

							if ( diag >= left )
								*p = element_type(diag,STEP_MATCH);
							else
								*p = element_type(left,STEP_INS);						
						}
						else
						{
							similarity_type const diag = (q++)->first - penalty_subst;
							similarity_type const left = q->first - penalty_ins;
							
							if ( diag >= left )
								*p = element_type(diag,STEP_MISMATCH);
							else
								*p = element_type(left,STEP_INS);							
						}
						
						for ( uint64_t j = i-firstlooplimit+1; j < m; ++j )
						{
							typename ::std::iterator_traits<iterator_b>::value_type const cb = b[j];
							#if defined(BANDEDLCSDEBUG)
							std::cerr << "b[" << j << "]=" << b[j] << std::endl;
							#endif
							
							similarity_type const d = (ca == cb) ? gain_match : (-penalty_subst);
							similarity_type const diag = (q++)->first + d;
							similarity_type const left = q->first - penalty_ins;
							similarity_type const top = (p++)->first - penalty_del;
							
							if ( diag >= left )
							{
								if ( diag >= top )
									*p = element_type(diag,(ca==cb) ? STEP_MATCH : STEP_MISMATCH);
								else
									*p = element_type(top,STEP_DEL);
							}
							else
							{
								if ( top >= left )
									*p = element_type(top,STEP_DEL);
								else
									*p = element_type(left,STEP_INS);
							}
						}
						
						tt = p;
						
						p++;
						
						p += i - (firstlooplimit+fullcols) + 1;
						q += i - (firstlooplimit+fullcols) + 1;

						#if defined(BANDEDLCSDEBUG)
						std::cerr << "pmod " << (p - M.begin()) % k21 << std::endl;
						std::cerr << "qmod " << (q - M.begin()) % k21 << std::endl;
						#endif
					}
				}
				
				
				// std::cerr << "trace offset " << tt- (M.begin()+n*k21) << std::endl;;

				// printMatrix(m);

				assert ( tt );
				// similarity_type const score = tt->first;
				step_type * tc = te;
				
				uint64_t numins = 0, numdel = 0, nummis = 0, nummat = 0;
				
				while ( tt != (M.begin()+k) )
				{
					#if 0
					std::cerr << "col " << (tt-M.begin()) / k21 << std::endl;
					std::cerr << "row " << (tt-M.begin()) % k21 << std::endl;
					std::cerr << AlignmentPritn::stepToString(tt->second) << std::endl;
					#endif
				
					*(--tc) = tt->second;
					
					switch ( tt->second )
					{
						case STEP_MATCH:
							tt -= k21;
							nummat++;
							break;
						case STEP_MISMATCH:
							tt -= k21;
							nummis++;
							break;
						case STEP_DEL:
							numdel++;
							tt -= 1;
							break;
						case STEP_INS:
							numins++;
							tt = (tt-k21)+1;
							break;
					}
				}
				ta = tc;
				
				#if 0
				for ( step_type * tq = tc; tq != te; ++tq )
					std::cerr << AlignmentPrint::stepToString(*tq);
				std::cerr << std::endl;
				#endif
				
				return EditDistanceResult(numins,numdel,nummat,nummis);
			}
		};
	}
}
#endif
