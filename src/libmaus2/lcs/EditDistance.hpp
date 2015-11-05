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

#if !defined(LIBMAUS2_LCS_EDITDISTANCE_HPP)
#define LIBMAUS2_LCS_EDITDISTANCE_HPP

#include <libmaus2/lcs/EditDistancePriorityType.hpp>
#include <libmaus2/lcs/EditDistanceTraceContainer.hpp>
#include <libmaus2/lcs/AlignmentPrint.hpp>
#include <libmaus2/types/types.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/exception/LibMausException.hpp>
#include <libmaus2/lcs/EditDistanceResult.hpp>
#include <libmaus2/lcs/BaseConstants.hpp>
#include <libmaus2/lcs/PenaltyConstants.hpp>
#include <libmaus2/lcs/AlignmentTraceContainer.hpp>
#include <libmaus2/lcs/Aligner.hpp>
#include <map>
#include <iostream>
#include <iomanip>

namespace libmaus2
{
	namespace lcs
	{
		template<
			libmaus2::lcs::edit_distance_priority_type _edit_distance_priority = ::libmaus2::lcs::del_ins_diag
		>
		struct EditDistance : public EditDistanceTraceContainer, public Aligner
		{
			static ::libmaus2::lcs::edit_distance_priority_type const edit_distance_priority = _edit_distance_priority;
			typedef EditDistance<edit_distance_priority> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef EditDistanceResult result_type;

			private:
			uint64_t n; // rows
			uint64_t m; // columns
			uint64_t n1;
			uint64_t m1;

			// matrix stored column wise
			typedef std::pair < similarity_type, step_type > element_type;
			::libmaus2::autoarray::AutoArray<element_type> M;

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
					M = ::libmaus2::autoarray::AutoArray<element_type>(n1*m1,false);
				if ( EditDistanceTraceContainer::capacity() < n+m )
					EditDistanceTraceContainer::resize(n+m);
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
			EditDistance()
			{
			}

			// pure edit distance
			void align(uint8_t const * a,size_t const l_a,uint8_t const * b,size_t const l_b)
			{
				process(a,l_a,b,l_b,0 /* k */, 0 /* match gain */,1,1,1);
			}

			template<typename iterator_a, typename iterator_b>
			EditDistanceResult process(
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

				return EditDistanceResult(numins,numdel,nummat,nummis);
			}

			AlignmentTraceContainer const & getTraceContainer() const
			{
				return *this;
			}
		};
	}
}
#endif
