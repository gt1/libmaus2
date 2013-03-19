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

#if ! defined(LCS_HPP)
#define LCS_HPP

#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/lcs/BaseConstants.hpp>

namespace libmaus
{
	namespace lcs
	{
		template < typename _error_count_type >
		struct LCSBase : public ::libmaus::lcs::BaseConstants
		{		
			typedef _error_count_type error_count_type;
			typedef LCSBase<error_count_type> this_type;
			typedef ::libmaus::lcs::BaseConstants::step_type trace_element;
			// enum trace_element { SUBST_NONE, SUBST_CHAR, SUBST_IN, SUBST_DEL };
			typedef std::pair < error_count_type , trace_element > matrix_element_type;
			
			static std::string traceElementToString(trace_element const & t)
			{
				switch ( t )
				{
					case this_type::STEP_MATCH:
						return "STEP_MATCH";
					case this_type::STEP_MISMATCH:
						return "STEP_MISMATCH";
					case this_type::STEP_INS:
						return "STEP_INS";
					case this_type::STEP_DEL:
						return "STEP_DEL";
					default:
						return "SUBST_UNKNOWN";
				}	
			}
			
			template<typename iterator_a_type, typename iterator_b_type>
			static std::ostream & printAlignment(
				std::ostream & out,
				iterator_a_type A,
				iterator_b_type B,
				trace_element const * ta,
				trace_element const * te
				)
			{
				std::ostringstream linea, lineb, linec;
				
				bool ok = true;
				
				for ( ; ta != te ; ta++ )
				{
					switch ( *ta )
					{
						case this_type::STEP_MATCH:
							ok = ok && (*A == *B);
							linea << *(A++);
							lineb << *(B++);
							linec << ' ';
							break;
						case this_type::STEP_MISMATCH:
							ok = ok && (*A != *B);
							linea << *(A++);
							lineb << *(B++);
							linec << 'S';
							break;
						case this_type::STEP_INS:
							linea << *(A++);
							lineb << ' ';
							linec << 'I';
							break;
						case this_type::STEP_DEL:
							linea << ' ';
							lineb << *(B++);
							linec << 'D';
							break;
					}
				}
				
				out << linea.str() << std::endl;
				out << lineb.str() << std::endl;
				out << linec.str() << std::endl;
				
				assert ( ok );
				
				return out;
			}

			template<typename iterator_a_type, typename iterator_b_type>
			static bool testAlignment(
				iterator_a_type A,
				iterator_b_type B,
				trace_element const * ta,
				trace_element const * te
				)
			{
				bool ok = true;
				
				for ( ; ta != te ; ta++ )
				{
					switch ( *ta )
					{
						case this_type::STEP_MATCH:
							ok = ok && (*A == *B);
							A++; B++;
							break;
						case this_type::STEP_MISMATCH:
							ok = ok && (*A != *B);
							A++;
							B++;
							break;
						case this_type::STEP_INS:
							A++;
							break;
						case this_type::STEP_DEL:
							B++;
							break;
					}
				}
				
				return ok;
			}

			// all three: diagonal, previous row, previous column
			static inline matrix_element_type const & min3(matrix_element_type const & a, matrix_element_type const & b, matrix_element_type const & c)
			{
				if ( a.first <= b.first )
				{
					if ( a.first <= c.first )
					{
						return a;
					}
					else
					{
						return c;
					}
				}
				else
				{
					assert ( a.first > b.first );
					if ( b.first <= c.first )
					{
						return b;
					}
					else
					{
						return c;
					}
				}
			}

		};

		/* unbanded longest common subsequence computation */
		template<typename _error_count_type>
		struct LCS : public LCSBase<_error_count_type>
		{
			typedef LCSBase<_error_count_type> base_type;
			typedef typename base_type::error_count_type error_count_type;
			typedef typename base_type::matrix_element_type matrix_element_type;
			typedef typename base_type::trace_element trace_element;

			uint64_t const n; // rows
			uint64_t const m; // columns
			::libmaus::autoarray::AutoArray < matrix_element_type > M;
			::libmaus::autoarray::AutoArray < trace_element > trace;
			trace_element * ta;
			
			LCS(uint64_t const rn, uint64_t const rm)
			: n(rn), m(rm), M ( (n+1)*(m+1),false ), ta(trace.end())
			{
			
			}

			
			matrix_element_type const & matrix(uint64_t j, uint64_t i) const
			{
				return M [ j*(m+1) + i ];
			}
			matrix_element_type       & matrix(uint64_t j, uint64_t i)
			{
				return M [ j*(m+1) + i ];
			}
			
			template<typename iterator_a_type, typename iterator_b_type>
			error_count_type compute(iterator_a_type A, iterator_b_type B)
			{
				for ( uint64_t i = 0; i <= n; ++i )
					matrix(i,0) = matrix_element_type(i,base_type::STEP_DEL);
				for ( uint64_t i = 0; i <= m; ++i )
					matrix(0,i) = matrix_element_type(i,base_type::STEP_INS);
				
				// column
				for ( uint64_t i = 1; i <= m; ++i )
					// row
					for ( uint64_t j = 1; j <= n; ++j )
					{
						// previous row
						matrix_element_type const a ( matrix(j-1,i-0).first + 1, base_type::STEP_DEL );
						// previous col
						matrix_element_type const b ( matrix(j-0,i-1).first + 1, base_type::STEP_INS );
						// char comp
						error_count_type const d = ((A[i-1]==B[j-1])?0:1);
						// diagonal
						matrix_element_type const c ( matrix(j-1,i-1).first + d, d?base_type::STEP_MISMATCH:base_type::STEP_MATCH);
						// set new value
						matrix(j,i) = min3(c,b,a);
					}
					
				uint64_t tc = m, tr = n;
				
				uint64_t tracelen = 0;
				while ( tc + tr )
				{
					tracelen += 1;
					
					switch ( matrix(tr,tc).second )
					{
						case base_type::STEP_MATCH:
						case base_type::STEP_MISMATCH:
							tc -= 1;
							tr -= 1;
							break;
						case base_type::STEP_INS:
							tc -= 1;
							break;
						case base_type::STEP_DEL:
							tr -= 1;
							break;
					}
					
				}

				trace = ::libmaus::autoarray::AutoArray < trace_element >(tracelen);
				ta = trace.end();

				tc = m; tr = n;

				while ( tc + tr )
				{
					*(--ta) = matrix(tr,tc).second;
					
					switch ( matrix(tr,tc).second )
					{
						case base_type::STEP_MATCH:
						case base_type::STEP_MISMATCH:
							tc -= 1;
							tr -= 1;
							break;
						case base_type::STEP_INS:
							tc -= 1;
							break;
						case base_type::STEP_DEL:
							tr -= 1;
							break;
					}
				}
				
				assert ( ta == trace.begin() );
				
				return matrix(n,m).first;
			}

			std::ostream & printTrace(std::ostream & out) const
			{
				for ( trace_element * tt = ta; tt != trace.end(); ++tt)
				{
					out << base_type::traceElementToString(*tt) << std::endl;
				}
				return out;		
			}

			template<typename iterator_a_type, typename iterator_b_type>
			std::ostream & printAlignment(
				std::ostream & out,
				iterator_a_type A,
				iterator_b_type B
				)
			{
				return base_type::printAlignment(out,A,B,ta,trace.end());
			}

			template<typename iterator_a_type, typename iterator_b_type>
			bool testAlignment(iterator_a_type A, iterator_b_type B)
			{
				return base_type::printAlignment(A,B,ta,trace.end());
			}
			
			std::ostream & print(std::ostream & out) const
			{
				// row
				for ( uint64_t j = 0; j <= n; ++j )
				{
					// column
					for ( uint64_t i = 0; i <= m; ++i )
					{
						out << (static_cast<uint64_t>(matrix(j,i).first)%10) << "(" << matrix(j,i).second << ")" << ";";
					}
					out << std::endl;
				}
				
				return out;
			}
		};

		/* banded longest common subsequence computation */
		template<typename _error_count_type>
		struct BandedLCS : public LCSBase<_error_count_type>
		{
			typedef LCSBase<_error_count_type> base_type;
			typedef typename base_type::error_count_type error_count_type;
			typedef typename base_type::matrix_element_type matrix_element_type;
			typedef typename base_type::trace_element trace_element;

			uint64_t const n;
			uint64_t const k;
			uint64_t const maxcolsize;
			::libmaus::autoarray::AutoArray < matrix_element_type > M;
			::libmaus::autoarray::AutoArray < uint64_t > O;
			::libmaus::autoarray::AutoArray < trace_element > trace;
			trace_element * ta;

			BandedLCS(uint64_t const rn, uint64_t const rk)
			: n(rn), k(std::min(rk,n/2)), maxcolsize(2*k+1), M ( (n+1+k) * maxcolsize, false ), O(n+k,false), trace(n+k,false), ta(trace.end())
			{
				assert ( n >= k );
				
				for ( uint64_t i = 0; i < k; ++i )
					O[i] = 0;
				for ( uint64_t i = k; i < n+k; ++i )
					O[i] = 1;
			}


			// previous row same column
			static inline matrix_element_type const & min2_1(matrix_element_type const & a, matrix_element_type const & b)
			{
				if ( a.first <= b.first )
				{
					return a;
				}
				else
				{
					return b;
				}
			}

			// previous column same row
			static inline matrix_element_type const & min2_2(matrix_element_type const & a, matrix_element_type const & b)
			{
				if ( a.first <= b.first )
				{
					return a;
				}
				else
				{
					return b;
				}
			}
			
			// #define BANDED_DEBUG

			template<typename iterator_a_type, typename iterator_b_type>
			error_count_type compute(iterator_a_type A, iterator_b_type B, uint64_t const m)
			{
				assert ( m >= n );
				assert ( m-n <= k );
			
				typedef typename ::std::iterator_traits<iterator_a_type>::value_type key_type;
			
				matrix_element_type * prev_col = M.begin();

				#if defined(BANDED_DEBUG)
				std::map < std::pair<uint64_t,uint64_t>, matrix_element_type > debmap;
				#endif
				
				prev_col[0] = matrix_element_type(0,BaseConstants::STEP_MATCH);
				#if defined(BANDED_DEBUG)
				debmap [ std::pair<uint64_t,uint64_t> ( 0,0 ) ] = prev_col[0];
				#endif

				/* initialize first column */
				for ( uint64_t i = 1; i <= k; ++i )
				{
					prev_col [ i ] = matrix_element_type(i,BaseConstants::STEP_DEL);
					#if defined(BANDED_DEBUG)
					debmap [ std::pair<uint64_t,uint64_t> ( 0,i ) ] = prev_col[i];
					#endif
				}

				/* initialize columns 1 to k */
				for ( uint64_t i = 1; i <= k; ++i )
				{
					#if defined(BANDED_DEBUG)
					assert ( (prev_col-M.begin()) % maxcolsize == 0 );
					#endif
					
					key_type const ca = A[i-1];
				
					matrix_element_type * this_col = prev_col + maxcolsize;
					
					(*this_col) = matrix_element_type(i,BaseConstants::STEP_INS);

					#if defined(BANDED_DEBUG)
					std::cerr << "(2*) i=" << i << " j=" << 0 << " : " << static_cast<uint64_t>(this_col[0].first) << std::endl;
					debmap [ std::pair<uint64_t,uint64_t> (i,0) ] = this_col[0];
					#endif
					
					for ( uint64_t j = 1; j < i+k; ++j )
					{
						// std::cerr << "(2) i=" << i << " j=" << j << std::endl;
					
						// previous row same column
						matrix_element_type const a(this_col[j-1].first+1,BaseConstants::STEP_DEL);
						// previous column same row
						matrix_element_type const b(prev_col[j  ].first+1,BaseConstants::STEP_INS);
						// character comp
						error_count_type const d = ((ca==B[j-1])?0:1);
						// diagonal
						matrix_element_type const c(prev_col[j-1].first+d,d?BaseConstants::STEP_MISMATCH:BaseConstants::STEP_MATCH);
						// set value
						this_col[j] = min3(c,a,b);

						#if defined(BANDED_DEBUG)
						std::cerr << "(2) i=" << i << " j=" << j << " : " << static_cast<uint64_t>(this_col[j].first) << std::endl;
						debmap [ std::pair<uint64_t,uint64_t> (i,j) ] = this_col[j];
						#endif
					}
					
					// row j = i+k
					// previous row same column
					matrix_element_type const a(this_col[i+k-1].first+1,BaseConstants::STEP_DEL);
					// character comparison
					error_count_type const d = ((ca==B[i+k-1])?0:1);
					// diagonal
					matrix_element_type const c(prev_col[i+k-1].first + d,d?BaseConstants::STEP_MISMATCH:BaseConstants::STEP_MATCH);
					// set
					this_col[i+k] = min2_1(c,a);

					#if defined(BANDED_DEBUG)			
					std::cerr << "(2**) i=" << i << " j=" << (i+k) << " : " << static_cast<uint64_t>(this_col[i+k].first) << std::endl;
					debmap [ std::pair<uint64_t,uint64_t> (i,i+k) ] = this_col[i+k];
					#endif
					
					prev_col = this_col;
				}
				
				/* initialize columns k+1 to n-k */
				matrix_element_type * this_col = prev_col + maxcolsize;
				
				for ( uint64_t i = k+1; i <= n-k; ++i )
				{
					#if defined(BANDED_DEBUG)			
					assert ( (prev_col - M.begin()) % (maxcolsize) == 0 );
					assert ( (prev_col - M.begin())/maxcolsize == (i-1) );
					std::cerr << (prev_col-M.begin()) << std::endl;
					#endif
					
					key_type const ca = A[i-1];
					
					/* char comp */
					error_count_type const ad = ((ca==B[i-k-1])?0:1);
					/* diagonal */
					matrix_element_type const aa((*(prev_col++)).first + ad,ad?BaseConstants::STEP_MISMATCH:BaseConstants::STEP_MATCH);
					/* previous column same row */
					matrix_element_type const ab( (*prev_col).first + 1, BaseConstants::STEP_INS);
					/* set value */
					matrix_element_type prev_val = *(this_col++) = min2_2(aa,ab);

					#if defined(BANDED_DEBUG)
					std::cerr << "(3*) i=" << i << " j=" << (i-k) << std::endl;
					debmap [ std::pair<uint64_t,uint64_t> (i,i-k) ] = prev_val;
					#endif
					
					for ( uint64_t j = i-k+1; j < i+k; ++j )
					{
						/* same column previous row */
						matrix_element_type const a ( prev_val.first + 1 , BaseConstants::STEP_DEL);
						/* character comp */
						error_count_type const d = ((ca==B[j-1])?0:1);
						/* diagonal */
						matrix_element_type const b ( (*(prev_col++)).first + d, d?BaseConstants::STEP_MISMATCH:BaseConstants::STEP_MATCH);
						/* previous column same row */
						matrix_element_type const c ( (*prev_col).first + 1, BaseConstants::STEP_INS);
						prev_val = (*(this_col)++) = min3(b,a,c);

						#if defined(BANDED_DEBUG)
						std::cerr << "(3) i=" << i << " j=" << j << " : " << static_cast<uint64_t>(prev_val.first) << std::endl;
						debmap [ std::pair<uint64_t,uint64_t> (i,j) ] = prev_val;
						#endif
					}

					/* same column previous row */
					matrix_element_type const ba ( prev_val.first + 1, BaseConstants::STEP_DEL );
					/* char comp */
					error_count_type const bd = ((ca==B[i+k-1])?0:1);
					/* diagonal */
					matrix_element_type const bb ( (*(prev_col++)).first + bd, bd?BaseConstants::STEP_MISMATCH:BaseConstants::STEP_MATCH);
					prev_val = *(this_col++) = min2_1(bb,ba);

					#if defined(BANDED_DEBUG)
					assert ( (this_col - M.begin()) % (maxcolsize) == 0 );
					assert ( (this_col - M.begin()) / (maxcolsize) == i+1 );

					std::cerr << "(3**) i=" << i << " j=" << (i+k) << std::endl;
					debmap [ std::pair<uint64_t,uint64_t> (i,i+k) ] = prev_val;
					#endif
				}
				
				/* final columns */
				assert ( k <= (n+1) );
				// std::cerr << "Final columns " << ((n+1)-k) << " to " << m << std::endl;
				for ( uint64_t i = ((n+1)-k); i <= m; ++i )
				{
					#if defined(BANDED_DEBUG)
					std::cerr << "prev " << (prev_col-M.begin()) << std::endl;
					std::cerr << "this " << (this_col-M.begin()) << std::endl;
					assert ( (prev_col-M.begin()) % maxcolsize == 0 );
					assert ( (this_col-M.begin()) % maxcolsize == 0 );
					#endif
					
					key_type const ca = A[i-1];
				
					// this_col = prev_col + maxcolsize;
					
					#if defined(BANDED_DEBUG)
					std::cerr << "Should do column " << i << " start at " << (i-k) << std::endl;
					#endif
					
					assert ( (k+1) <= i );
					
					/* char comp */
					error_count_type const ad = ((ca==B[i-k-1])?0:1);
					/* diagonal */
					matrix_element_type const aa ( (*(prev_col++)).first + ad, ad?BaseConstants::STEP_MISMATCH:BaseConstants::STEP_MATCH);
					/* same row previous column */
					matrix_element_type const ab ( (*prev_col).first + 1, BaseConstants::STEP_INS);
					/* set value */
					matrix_element_type prev_val = (*(this_col++)) = min2_2(aa,ab);
					
					#if defined(BANDED_DEBUG)
					std::cerr << "(4*) i=" << i << " j=" << (i-k) << " : " << static_cast<uint64_t>(prev_val.first) << " ad " << static_cast<uint64_t>(ad) << std::endl;
					debmap [ std::pair<uint64_t,uint64_t> (i,i-k) ] = prev_val;
					#endif
					
					for ( uint64_t j = i-k+1; j <= n; ++j )
					{
						/* same column previous row */
						matrix_element_type const a ( prev_val.first + 1, BaseConstants::STEP_DEL );
						/* char comp */
						error_count_type const d = ((ca==B[j-1])?0:1);
						/* diagonal */
						matrix_element_type const b ( (*(prev_col++)).first + d, d?BaseConstants::STEP_MISMATCH:BaseConstants::STEP_MATCH);
						/* same row previous column */
						matrix_element_type const c ( (*prev_col).first + 1, BaseConstants::STEP_INS );
						prev_val = (*(this_col++)) = min3(b,a,c);

						#if defined(BANDED_DEBUG)
						std::cerr << "(4) i=" << i << " j=" << j << " : " << static_cast<uint64_t>(prev_val.first) << std::endl;
						debmap [ std::pair<uint64_t,uint64_t> (i,j) ] = prev_val;
						#endif
					}
					
					#if defined(BANDED_DEBUG)
					std::cerr << "i=" << i << " Should add " << (k - (n-i) ) << " :: " << (2*k)-(n+k-i) << std::endl;
					#endif
					
					prev_col += (k-(n-i));
					this_col += (k-(n-i));
				}
				
				/* extract LCS and store trace */
				/* the trace prefers substitution over indels */
				this_col = M.begin() + m*maxcolsize;
				uint64_t tr = k-(m-n);
				error_count_type const e = this_col[tr].first;
				
				if ( e <= k )
				{
					uint64_t tc = m;
					trace_element * t = trace.end();
							
					while ( tc + tr )
					{
						#if defined(BANDED_DEBUG)
						std::cerr << "e=" << e << " tc=" << tc << " tr=" << tr << " sec=" << base_type::traceElementToString(this_col[tr].second) << std::endl;
						#endif

						*(--t) = this_col[tr].second;

						switch ( this_col[tr].second )
						{
							case BaseConstants::STEP_MATCH:
							case BaseConstants::STEP_MISMATCH:
								this_col -= maxcolsize;
								tc -= 1;
								tr = tr - 1 + O[tc];
								break;
							case BaseConstants::STEP_INS:
								this_col -= maxcolsize;
								tc -= 1;
								assert ( tc < O.size() );
								tr = tr + O[tc];
								break;
							case BaseConstants::STEP_DEL:
								tr -= 1;
								break;
						}
					}
					
					ta = t;
				}
				else
				{
					ta = trace.end();
				}
			
				return e;
			}
			
			std::ostream & printTrace(std::ostream & out) const
			{
				for ( trace_element * tt = ta; tt != trace.end(); ++tt)
				{
					out << base_type::traceElementToString(*tt) << std::endl;
				}
				return out;		
			}

			template<typename iterator_a_type, typename iterator_b_type>
			std::ostream & printAlignment(
				std::ostream & out,
				iterator_a_type A,
				iterator_b_type B
				)
			{
				return base_type::printAlignment(out,A,B,ta,trace.end());
			}
			
			std::vector<trace_element> getTrace() const
			{
				return std::vector<trace_element>(
					static_cast<trace_element const *>(ta),
					static_cast<trace_element const *>(trace.end())
				);
			}

			template<typename iterator_a_type, typename iterator_b_type>
			bool testAlignment(iterator_a_type A, iterator_b_type B)
			{
				return base_type::testAlignment(A,B,ta,trace.end());
			}
			
			std::ostream & print(std::ostream & out) const
			{
				// row
				for ( uint64_t j = 0; j < maxcolsize; ++j )
				{
					// column
					for ( uint64_t i = 0; i <= n+k; ++i )
						out << (static_cast<uint64_t>( M[ i*maxcolsize + j ].first )%10) << ";";
					out << std::endl;
				}
				
				return out;
			}

		};
	}
}
#endif
