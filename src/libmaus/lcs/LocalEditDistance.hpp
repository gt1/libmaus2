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

#if !defined(LIBMAUS_LCS_LOCALEDITDISTANCE_HPP)
#define LIBMAUS_LCS_LOCALEDITDISTANCE_HPP

#include <libmaus/lcs/EditDistancePriorityType.hpp>
#include <libmaus/lcs/AlignmentPrint.hpp>
#include <libmaus/types/types.hpp>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/exception/LibMausException.hpp>
#include <libmaus/lcs/LocalEditDistanceResult.hpp>
#include <libmaus/lcs/LocalEditDistanceTraceContainer.hpp>
#include <libmaus/lcs/LocalBaseConstants.hpp>
#include <libmaus/lcs/LocalPenaltyConstants.hpp>
#include <libmaus/lcs/LocalAlignmentTraceContainer.hpp>
#include <map>
#include <iostream>
#include <iomanip>

namespace libmaus
{
	namespace lcs
	{
		template<
			libmaus::lcs::edit_distance_priority_type _edit_distance_priority = ::libmaus::lcs::del_ins_diag
		>
		struct LocalEditDistance : public LocalEditDistanceTraceContainer
		{
			static ::libmaus::lcs::edit_distance_priority_type const edit_distance_priority = _edit_distance_priority;
			typedef EditDistance<edit_distance_priority> this_type;
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
		
			private:
			uint64_t n; // rows
			uint64_t m; // columns
			uint64_t n1;
			uint64_t m1;

			// matrix stored column wise
			typedef std::pair < similarity_type, step_type > element_type;
			::libmaus::autoarray::AutoArray<element_type> M;
			
			void setup(
				uint64_t const rn,
				uint64_t const rm,
				uint64_t const /* rk */ = 0
			)
			{
				n = rn;
				m = rm;
				n1 = n+1;
				m1 = m+1;
			
				if ( M.size() < n1*m1 )
					M = ::libmaus::autoarray::AutoArray<element_type>(n1*m1,false);
				if ( LocalEditDistanceTraceContainer::capacity() < n+m )
					LocalEditDistanceTraceContainer::resize(n+m);
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

			public:
			LocalEditDistance()
			{
			}
			
			
			template<typename iterator_a, typename iterator_b>
			LocalEditDistanceResult process(
				iterator_a a, 
				uint64_t const n,
				iterator_b b,
				uint64_t const m,
				uint64_t const k = 0,
				similarity_type const gain_match = 1,
				similarity_type const penalty_subst = 1,
				similarity_type const penalty_ins = 1,
				similarity_type const penalty_del = 1
			)
			{
				setup(n,m,k);
				
				element_type * p = M.begin();

				element_type * maxel = p;

				for ( uint64_t i = 0; i < n1; ++i )
					*(p++) = element_type(0,STEP_RESET);
					
				element_type * q = M.begin();

				iterator_a const ae = a+n;
				iterator_b const be = b+m;				
				while ( b != be )
				{
					typename std::iterator_traits<iterator_b>::value_type const bchar = *(b++);
					
					assert ( (p-M.begin()) % n1 == 0 );
					assert ( (q-M.begin()) % n1 == 0 );
					
					// top
					int64_t const psc = q->first-penalty_ins;
					if ( psc >= 0 )
					{
						*p = element_type(psc,STEP_INS);

						if ( p->first > maxel->first )
							maxel = p;
					}
					else
						*p = element_type(0,STEP_RESET);
						
					
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
					
						switch ( edit_distance_priority )
						{
							case del_ins_diag:
								if ( left >= top )
								{
									if ( left >= diag )
										// left
										*p = element_type(left,STEP_DEL);
									else							
										// diag
										*p = element_type(diag,dmatch ? STEP_MATCH : STEP_MISMATCH);
								}
								// top >= left
								else
								{
									if ( top >= diag )
										// top
										*p = element_type(top,STEP_INS);
									else
										// diag
										*p = element_type(diag,dmatch ? STEP_MATCH : STEP_MISMATCH);
								}
								break;
							case diag_del_ins:
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
								break;
						}	

						if ( p->first < 0 )
							*p = element_type(0,STEP_RESET);
						else if ( p->first > maxel->first )
							maxel = p;
					}	
					
					p++;
					q++;				
				}
				
				b -= m;

				uint64_t i = n;
				uint64_t j = m;
				element_type * pq = maxel; // M.get() + j*n1 + i;
				
				uint64_t const offback = pq-M.begin();
				uint64_t const offbackb = offback / n1;
				uint64_t const offbacka = offback - (offbackb*n1);
				
				ta = te;
				
				uint64_t numdel = 0;
				uint64_t numins = 0;
				uint64_t nummat = 0;
				uint64_t nummis = 0;

				while ( pq->second != STEP_RESET )
				{
					*(--ta) = pq->second;
					
					switch ( pq->second )
					{
						// previous row
						case STEP_INS:
							pq -= n1;
							numins++;
							break;
						// previous column
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

				uint64_t const offfront = pq-M.begin();
				uint64_t const offfrontb = offfront / n1;
				uint64_t const offfronta = offfront - (offfrontb*n1);
				
				return LocalEditDistanceResult(numins,numdel,nummat,nummis,offfronta,n-offbacka,offfrontb,m-offbackb);
			}	
		};
	}
}
#endif
