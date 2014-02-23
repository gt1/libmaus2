/*
    libmaus
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#if ! defined(LIBMAUS_LCS_BANDEDEDITDISTANCE_HPP)
#define LIBMAUS_LCS_BANDEDEDITDISTANCE_HPP

#include <libmaus/lcs/EditDistanceTraceContainer.hpp>
#include <libmaus/lcs/EditDistanceResult.hpp>
#include <iomanip>

namespace libmaus
{
	namespace lcs
	{		
		struct BandedEditDistance : public EditDistanceTraceContainer
		{
			typedef BandedEditDistance this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			private:			
			uint64_t const n; // columns
			uint64_t const n1; // n+1
			uint64_t const m;
			uint64_t const m1;
			uint64_t const k;
			uint64_t const k21;

			typedef std::pair < similarity_type, step_type > element_type;
			libmaus::autoarray::AutoArray<element_type> M;
			
			public:
			static bool validParameters(
				uint64_t const n,
				uint64_t const m,
				uint64_t const k				
			)
			{
				return 
					(n+1) >= (2*(k+1))
					&&
					(m+1) >= (2*(k+1))
					&&
					((std::max(n,m)-std::min(n,m)) <= k)
				;
			}
			
			BandedEditDistance(
				uint64_t const rn,
				uint64_t const rm,
				uint64_t const rk
			)
			: EditDistanceTraceContainer(rn+rm+1), 
			  n(rn), n1(n+1), m(rm), m1(m+1), k(rk), k21(2*k+1), M(m1 * k21,false)
			{
				if ( ! validParameters(n,m,k) )
				{
					libmaus::exception::LibMausException se;
					se.getStream() << "BandedEditDistance::BandedEditDistance(): parameters n=" << n << " m=" << m << " k=" << k << " are invalid." << std::endl;
					se.finish();
					throw se;
				}
			}
			
			element_type operator()(
				uint64_t const i, uint64_t const j
			) const
			{
				if ( 
				
					static_cast<int64_t>(i)-static_cast<int64_t>(j) >= -static_cast<int64_t>(k)
					&&
					static_cast<int64_t>(i)-static_cast<int64_t>(j) <=  static_cast<int64_t>(k)
					&&
					j < n1
					&&
					i < m1
				)
				{
					element_type const * p = M.begin();
				
					p += i*(k21);
					p += k;
					p += static_cast<int64_t>(j) - static_cast<int64_t>(i);
				
					return *p;
				}
				else
				{
					return element_type();
				}
			}
			
			std::pair<int64_t,int64_t> decode(element_type const * p)
			{
				uint64_t const off = p-M.begin();
				uint64_t const i = off / k21;
				uint64_t const o = off - (i * k21);
				uint64_t const j = static_cast<int64_t>(i)+o-k;
				return std::pair<int64_t,int64_t>(i,j);
			}
			
			std::string decodes(element_type const * p)
			{
				std::pair<int64_t,int64_t> P = decode(p);
				std::ostringstream ostr;
				ostr << "(" << P.first << "," << P.second << ")";
				return ostr.str();
			}
			
			template<typename iterator_a, typename iterator_b>
			EditDistanceResult process(
				iterator_a aa,
				iterator_b bb,
				similarity_type const gain_match = 1,
				similarity_type const penalty_subst = 1,
				similarity_type const penalty_ins = 1,
				similarity_type const penalty_del = 1
			)
			{
				if ( k )
				{
					element_type * p = M.begin();
					element_type * q = p;
					
					p += k;
					for ( uint64_t i = 0; i < (k+1); ++i )
						*(p++) = element_type(-static_cast<similarity_type>(i*penalty_del),STEP_DEL);
						
					iterator_a a = aa;
					iterator_b b = bb;
					/*
					 * rows on the left border of the matrix
					 */
					for ( uint64_t i = 1; i <= k; ++i )
					{
						p += (k-i);
						q += (k-i+1);
						
						typename std::iterator_traits<iterator_b>::value_type const bchar = *(b++);					

						// top
						*p = element_type(q->first-penalty_ins,STEP_INS);
						
						iterator_a const ae = a + (k+i);
						while ( a != ae )
						{
							// left
							similarity_type const left =  p->first - penalty_del;
							// diagonal match?
							bool const dmatch = ( *(a++) == bchar );
							// diagonal
							similarity_type const diag =
								dmatch 
								?
								(q->first + gain_match)
								:
								(q->first - penalty_subst);
							// move pointer in row above
							q++;
							// top
							similarity_type const top = q->first - penalty_ins;
							// move pointer in current row
							p++;
							
							if ( diag >= left )
							{
								if ( diag >= top )
									// diag
									*p = element_type(diag,dmatch ? STEP_MATCH : STEP_MISMATCH);
								else
									// top
									*p = element_type(top,STEP_INS);
							}
							else
							{
								if ( left >= top )
									// left
									*p = element_type(left,STEP_DEL);
								else
									// top
									*p = element_type(top,STEP_INS);
							}
						}
						
						p++;
						a -= (k+i);
					}
					
					assert ( a == aa );
					
					/*
					 * rows which do not touch any border
					 */
					for ( uint64_t i = k+1; (b != (bb+m)) && (i-k+k21-2 < n); ++i )
					{
						/*
						 * process [ i-k+1 , i-k+k21-1 ) on a
						 */	 
						typename std::iterator_traits<iterator_b>::value_type const bchar = *(b++);
						assert ( ((p-M.begin()) % k21) == 0 );
						assert ( ((q-M.begin()) % k21) == 0 );
						assert ( a-aa == i-k-1 );
						
						bool const af_dmatch = ( (*(a++)) == bchar);
						similarity_type af_diag = af_dmatch ? (q->first + gain_match) : (q->first - penalty_subst);
						q++;
						similarity_type af_top = q->first - penalty_ins;
						
						if ( af_diag >= af_top )
							*p = element_type(af_diag,af_dmatch ? STEP_MATCH : STEP_MISMATCH);
						else
							*p = element_type(af_top,STEP_INS);
						
						iterator_a const ae = a + (k21-1);
						// for ( uint64_t j = i-k ; j < i-k+k21-1; ++j )
						while ( a != ae )
						{
							// left
							similarity_type const left =  p->first - penalty_del;
							// diagonal match?
							bool const dmatch = ( (*(a++)) == bchar);
							// diagonal
							similarity_type const diag =
								dmatch 
								?
								(q->first + gain_match)
								:
								(q->first - penalty_subst);
							// move pointer in row above
							q++;
							// top
							similarity_type const top = q->first - penalty_ins;
							// move pointer in current row
							p++;
							
							if ( diag >= left )
							{
								if ( diag >= top )
									// diag
									*p = element_type(diag,dmatch ? STEP_MATCH : STEP_MISMATCH);
								else
									// top
									*p = element_type(top,STEP_INS);
							}
							else
							{
								if ( left >= top )
									// left
									*p = element_type(left,STEP_DEL);
								else
									// top
									*p = element_type(top,STEP_INS);
							}
						}
						
						p++;
						a -= (k21-1);
					}
					
					for ( uint64_t pskip = 1; (b - bb) != m; ++pskip )
					{
						assert ( ((p-M.begin()) % k21) == 0 );
						assert ( ((q-M.begin()) % k21) == 0 );
						
						typename std::iterator_traits<iterator_b>::value_type const bchar = *(b++);

						bool const af_dmatch = ( (*(a++)) == bchar);
						similarity_type af_diag = af_dmatch ? (q->first + gain_match) : (q->first - penalty_subst);
						q++;
						similarity_type af_top = q->first - penalty_ins;
						
						if ( af_diag >= af_top )
							*p = element_type(af_diag,af_dmatch ? STEP_MATCH : STEP_MISMATCH);
						else
							*p = element_type(af_top,STEP_INS);
						
						iterator_a const ae = aa + n;
						while ( a != ae )
						{
							// left
							similarity_type const left =  p->first - penalty_del;
							// diagonal match?
							bool const dmatch = ( (*(a++)) == bchar);
							// diagonal
							similarity_type const diag =
								dmatch 
								?
								(q->first + gain_match)
								:
								(q->first - penalty_subst);
							// move pointer in row above
							q++;
							// top
							similarity_type const top = q->first - penalty_ins;
							// move pointer in current row
							p++;
							
							if ( diag >= left )
							{
								if ( diag >= top )
									// diag
									*p = element_type(diag,dmatch ? STEP_MATCH : STEP_MISMATCH);
								else
									// top
									*p = element_type(top,STEP_INS);
							}
							else
							{
								if ( left >= top )
									// left
									*p = element_type(left,STEP_DEL);
								else
									// top
									*p = element_type(top,STEP_INS);
							}
						}
						
						p++;
						
						p += pskip;
						q += pskip;
						
						a -= (k21-pskip-1);
					}

					#if 1
					for ( uint64_t i = 0; i < m1; ++i )
					{
						for ( uint64_t j = 0; j < n1; ++j )
						{
							std::cerr << "(" 
								<< std::setw(4)
								<< (*this)(i,j).first 
								<< std::setw(0)
								<< "," 
								<< (*this)(i,j).second
								<< ","
								<< ((j > 0) ? aa[j-1] : ' ')
								<< ","
								<< ((i > 0) ? bb[i-1] : ' ')
								<< ")";
						}
						
						std::cerr << std::endl;
					}
					#endif
					
					p = M.begin() + (static_cast<int64_t>(m * k21) + (static_cast<int64_t>(k) - (static_cast<int64_t>(m)-static_cast<int64_t>(n))));
					
					step_type * tc = te;
					
					uint64_t numins = 0, numdel = 0, nummis = 0, nummat = 0;

					while ( p != (M.begin()+k) )
					{
						// std::cerr << AlignmentPrint::stepToString(p->second) << std::endl;
					
						*(--tc) = p->second;
						
						switch ( p->second )
						{
							case STEP_MATCH:
								p -= k21;
								nummat++;
								break;
							case STEP_MISMATCH:
								p -= k21;
								nummis++;
								break;
							case STEP_INS:
								numins++;
								p -= (k21-1);
								break;
							case STEP_DEL:
								numdel++;
								p -= 1; // (p-k21)+1;
								break;
						}
					}

					ta = tc;

					return EditDistanceResult(numins,numdel,nummat,nummis);
				}
				else
				{
					assert ( k == 0 );
					assert ( n == m );
					
					step_type * tc = te - n;
					ta = tc;
					uint64_t nummis = 0, nummat = 0;
					
					for ( uint64_t i = 0; i < n; ++i )
						if ( aa[i] == bb[i] )
						{
							nummat++;
							*(tc++) = STEP_MATCH;
						}
						else
						{
							nummis++;
							*(tc++) = STEP_MISMATCH;
						}

					return EditDistanceResult(0,0,nummat,nummis);
				}
			}
		};
	}
}
#endif

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

#if !defined(LIBMAUS_LCS_EDITDISTANCE_HPP)
#define LIBMAUS_LCS_EDITDISTANCE_HPP

#include <libmaus/lcs/EditDistanceTraceContainer.hpp>
#include <libmaus/lcs/AlignmentPrint.hpp>
#include <libmaus/types/types.hpp>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/exception/LibMausException.hpp>
#include <libmaus/lcs/EditDistanceResult.hpp>
#include <libmaus/lcs/BaseConstants.hpp>
#include <libmaus/lcs/PenaltyConstants.hpp>
#include <libmaus/lcs/AlignmentTraceContainer.hpp>
#include <map>
#include <iostream>
#include <iomanip>

namespace libmaus
{
	namespace lcs
	{		
		struct EditDistance : public EditDistanceTraceContainer
		{
			typedef EditDistance this_type;
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
			EditDistance(uint64_t const rn, uint64_t const rm, uint64_t const = 0)
			: EditDistanceTraceContainer(rn+rm), n(rn), m(rm), n1(n+1), m1(m+1), M( n1*m1, false )
			{
				// std::cerr << "n=" << n << " m=" << m << " n1=" << n1 << " m1=" << m1 << std::endl;
			}
			
			element_type operator()(uint64_t const i, uint64_t const j) const
			{
				return M [ i * n1 + j ];
			}
			
			template<typename iterator_a, typename iterator_b>
			void printMatrix(iterator_a a, iterator_b b) const
			{
				element_type const * p = M.begin();
				for ( uint64_t i = 0; i < m1; ++i )
				{
					for ( uint64_t j = 0; j < n1; ++j, ++p )
					{
						std::cerr << "(" 
							<< std::setw(4)
							<< p->first 
							<< std::setw(0)
							<< "," 
							<< p->second 
							<< ","
							<< ((j > 0) ? a[j-1] : ' ')
							<< ","
							<< ((i > 0) ? b[i-1] : ' ')
							<< ")";
					}
					
					std::cerr << std::endl;
				}
			
			}
			
			template<typename iterator_a, typename iterator_b>
			EditDistanceResult process(
				iterator_a a, iterator_b b,
				similarity_type const gain_match = 1,
				similarity_type const penalty_subst = 1,
				similarity_type const penalty_ins = 1,
				similarity_type const penalty_del = 1
			)
			{
			
				element_type * p = M.begin();

				int64_t firstpen = 0;
				for ( uint64_t i = 0; i < n1; ++i, firstpen -= penalty_del )
					*(p++) = element_type(firstpen,STEP_DEL);
					
				element_type * q = M.begin();

				iterator_a const ae = a+n;
				iterator_b const be = b+m;				
				while ( b != be )
				{
					typename std::iterator_traits<iterator_b>::value_type const bchar = *(b++);
					
					assert ( (p-M.begin()) % n1 == 0 );
					assert ( (q-M.begin()) % n1 == 0 );
					
					// top
					*p = element_type(q->first-penalty_ins,STEP_INS);
					
					for ( iterator_a aa = a; aa != ae; ++aa )
					{
						// left
						similarity_type const left =  p->first - penalty_del;
						// diagonal match?
						bool const dmatch = (*aa == bchar);
						// diagonal
						similarity_type const diag =
							dmatch 
							?
							(q->first + gain_match)
							:
							(q->first - penalty_subst);
						// move pointer in row above
						q++;
						// top
						similarity_type const top = q->first - penalty_ins;
						// move pointer in current row
						p++;
						
						if ( diag >= left )
						{
							if ( diag >= top )
								// diag
								*p = element_type(diag,dmatch ? STEP_MATCH : STEP_MISMATCH);
							else
								// top
								*p = element_type(top,STEP_INS);
						}
						else
						{
							if ( left >= top )
								// left
								*p = element_type(left,STEP_DEL);
							else
								// top
								*p = element_type(top,STEP_INS);
						}
					}	
					
					p++;
					q++;				
				}
				
				b -= m;

				uint64_t i = n;
				uint64_t j = m;
				element_type * pq = M.get() + j*n1 + i;

				ta = te;
				
				uint64_t numdel = 0;
				uint64_t numins = 0;
				uint64_t nummat = 0;
				uint64_t nummis = 0;

				while ( pq != M.begin() )
				{
					*(--ta) = pq->second;
					
					switch ( pq->second )
					{
						// previous row
						case STEP_INS:
							pq -= n1;
							numins++;
							break;
						// previos column
						case STEP_DEL:
							pq -= 1;
							numdel++;
							break;
						// diagonal
						case STEP_MATCH:
							pq -= (n1+1);
							nummat++;
							break;
						// diagonal
						case STEP_MISMATCH:
							pq -= (n1+1);
							nummis++;
							break;
						default:
							break;
					}
				}
				
				return EditDistanceResult(numins,numdel,nummat,nummis);
			}	
		};
	}
}
#endif
