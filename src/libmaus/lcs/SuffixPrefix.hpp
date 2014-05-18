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

#if !defined(SUFFIXPREFIX_HPP)
#define SUFFIXPREFIX_HPP

#include <libmaus/types/types.hpp>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/lcs/TraceContainer.hpp>
#include <libmaus/lcs/GenericAlignmentPrint.hpp>
#include <libmaus/lcs/SuffixPrefixResult.hpp>
#include <libmaus/lcs/SuffixPrefixAlignmentPrint.hpp>
#include <map>
#include <iostream>

#include <libmaus/lcs/GenericEditDistance.hpp>

namespace libmaus
{
	namespace lcs
	{
		struct ScoreGeneric : public PenaltyConstants
		{
			template<typename char_type>
			static int32_t score(char_type const a, char_type const b)
			{
				return (a==b)?gain_match:(-penalty_subst);
			}
		};

		struct ScoreDna5 : public PenaltyConstants
		{
			template<typename char_type>
			static int32_t score(char_type const a, char_type const b)
			{
				if ( a == 'N' || b == 'N' )
					return -penalty_subst;
				else
					return (a==b)?gain_match:(-penalty_subst);
			}
		};
	
		template<typename _score_type>
		struct SuffixPrefixTemplate : public TraceContainer
		{
			typedef _score_type score_type;
			typedef SuffixPrefixTemplate<score_type> this_type;
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
		
			typedef int32_t similarity_type;
			typedef std::pair < similarity_type, step_type > element_type;

			private:
			uint64_t const n; // rows
			uint64_t const m; // columns
			uint64_t const n1;
			uint64_t const m1;

			// matrix stored column wise
			::libmaus::autoarray::AutoArray<element_type> M;

			public:
			// trace for best infix alignment
			TraceContainer infixtrace;
	
			SuffixPrefixTemplate(uint64_t const rn, uint64_t const rm)
			: TraceContainer(rn+rm), n(rn), m(rm), n1(n+1), m1(m+1), M( n1*m1, false ), infixtrace(rn+rm)
			{
				// std::cerr << "n=" << n << " m=" << m << " n1=" << n1 << " m1=" << m1 << std::endl;
			}
			
			template<typename iterator_a, typename iterator_b>
			SuffixPrefixResult process(iterator_a a, iterator_b b)
			{
				element_type * p = M.begin();
				
				// fill first column with zeroes
				*(p++) = element_type(0,STEP_MATCH);
				for ( uint64_t i = 1; i < n1; ++i )
					*(p++) = element_type(0,STEP_INS);
				
				// p = current column	
				// q = previous column
				element_type * q = M.begin();
				
				similarity_type maxscore = 0;
				uint32_t maxcol = 0;
				
				// fill dynamic programming matrix, rows a, columns b
				// outer iteration over columns (b)
				iterator_b const ba = b;
				iterator_b const be = b+m;
				iterator_b const be1 = be-1;
				iterator_a const ae = a+n;

				element_type * pl = p;
				element_type * plmax = p;
				// similarity_type maxinfixscore = std::numeric_limits<similarity_type>::min();

				if ( m )
				{
					while ( b != be1 )
					{
						typename std::iterator_traits<iterator_b>::value_type const bchar = *(b++);
						
						p->first = q->first;
						p->second = STEP_DEL;
						
						// iterate over rows
						for ( iterator_a aa = a; aa != ae; ++aa )
						{
							// above
							similarity_type const top = (p++)->first - penalty_ins;
							// diagonal
							similarity_type const d = // ((*aa) == bchar) ? gain_match : (-penalty_subst);
								score_type::score(*aa,bchar);
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

						// check last row for new maximum
						if ( p->first >= maxscore )
						{
							maxscore = p->first;
							maxcol = (b-ba);
						}
						
						p++;
						q++;
					}
					
					typename std::iterator_traits<iterator_b>::value_type const bchar = *(b++);
					assert ( b == be );
					
					pl = p;
					plmax = pl;
					p->first = q->first;
					p->second = STEP_DEL;
					
					// iterate over rows
					for ( iterator_a aa = a; aa != ae; ++aa )
					{
						// above
						similarity_type const top = (p++)->first - penalty_ins;
						// diagonal
						similarity_type const d = // ((*aa) == bchar) ? gain_match : (-penalty_subst);
							score_type::score(*aa,bchar);
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
						
						if ( p->first >= plmax->first )
							plmax = p;
					}

					// check last row for new maximum
					if ( p->first >= maxscore )
					{
						maxscore = p->first;
						maxcol = (b-ba);
					}
					
					p++;
					q++;
				
					// maxinfixscore = plmax->first;	
				}
				
				// length of suffix of b we do not use
				uint64_t const bclip = (m-maxcol);
				
				/* fill trace back array and build operation histogram */
				uint64_t i = n;
				uint64_t j = m1-bclip-1;
				element_type * pq = M.get() + j*n1 + i;
				element_type * pz = M.get() + n1;
				
				ta = te;
				
				uint64_t numdel = 0;
				uint64_t numin = 0;
				uint64_t nummat = 0;
				uint64_t nummis = 0;
				
				while ( pq >= pz )
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

				// length of prefix of a we skip
				uint64_t const aclip = pq-M.begin();
				
				infixtrace.ta = infixtrace.te;
				element_type * ipq = plmax;
				uint64_t infixnumdel = 0;
				uint64_t infixnumin = 0;
				uint64_t infixnummat = 0;
				uint64_t infixnummis = 0;

				while ( ipq >= pz )
				{
					*(--infixtrace.ta) = ipq->second;
				
					switch ( ipq->second )
					{
						case STEP_DEL:
							// go to previous column
							ipq -= n1;
							infixnumdel++;
							break;
						case STEP_INS:
							ipq -= 1;
							infixnumin++;
							break;
						case STEP_MATCH:
							ipq -= (n1+1);
							infixnummat++;
							break;
						case STEP_MISMATCH:
							ipq -= (n1+1);
							infixnummis++;
							break;
						default:
							break;
					}
				}

				// length of prefix of a we skip for infix
				uint64_t const infixaclippref = ipq-M.begin();
				// length of suffix of a we skip for infix
				uint64_t const infixaclipsuff = n-(plmax-pl);
				// score of infix alignment
				uint64_t const infixscore = plmax->first;
				
				if ( m )
				{
					step_type * ic = infixtrace.te;
					typename std::iterator_traits<iterator_b>::value_type const bchar = *(be-1);
					iterator_b bc = be;
					uint64_t itcnt = 0;
					
					#if 0
					std::ostringstream ostr;
					ostr << "\n\n ---- \n\n";
					ostr << "last char of b is " << bchar << std::endl;
					#endif

					do
					{
						--ic;
						--bc;
						
						if ( (*ic != STEP_MATCH) || (*bc != bchar) )
							break;
						
						#if 0							
						ostr << "iteration, step " << *ic << " char " << *bc << std::endl;
						#endif
						++itcnt;
					} while ( ic != infixtrace.ta );

					#if 0
					ostr << "step " << *ic << " char " << *bc << std::endl;
					#endif

					if ( (*ic == STEP_DEL) && (*bc == bchar) && itcnt )
					{
						#if 0
						ostr << "Case found." << std::endl;

						ostr << infixtrace.traceToString() << std::endl;
						printInfixAlignment(a,ba,
							SuffixPrefixResult(
								aclip,bclip,numin,numdel,nummat,nummis,
								infixaclippref,
								infixaclipsuff,
								infixnumin,
								infixnumdel,
								infixnummat,
								infixnummis,
								infixscore
							)		
							,ostr);
						#endif

						std::swap(*ic,*(infixtrace.te-1));
						
						#if 0
						ostr << infixtrace.traceToString() << std::endl;

						printInfixAlignment(a,ba,
							SuffixPrefixResult(
								aclip,bclip,numin,numdel,nummat,nummis,
								infixaclippref,
								infixaclipsuff,
								infixnumin,
								infixnumdel,
								infixnummat,
								infixnummis,
								infixscore
							)		
							,ostr);

						std::cerr << ostr.str();
						#endif
					}
				}			
				
				return SuffixPrefixResult(
					aclip,bclip,numin,numdel,nummat,nummis,
					infixaclippref,
					infixaclipsuff,
					infixnumin,
					infixnumdel,
					infixnummat,
					infixnummis,
					infixscore
				);
			}

			template<typename iterator_a, typename iterator_b>
			void printInfixAlignment(
				iterator_a a, iterator_b b,
				SuffixPrefixResult const & SPR,
				std::ostream & out
			)
			{
				GenericAlignmentPrint::printAlignmentLines(
					out,
					std::string(a+SPR.infix_aclip_left,a+(n-SPR.infix_aclip_right)),
					std::string(b,b+m),
					80,
					infixtrace.ta,infixtrace.te);			
			}
			
			std::ostream & printAlignment(
				std::ostream & out, std::string const & a, std::string const & b,
				SuffixPrefixResult const & SPR
			) const
			{
				SuffixPrefixAlignmentPrint::printAlignment(out,a,b,SPR,ta,te);
				return out;
			}
			std::ostream & printAlignmentLines(
				std::ostream & out, std::string const & a, std::string const & b,
				SuffixPrefixResult const & SPR,
				uint64_t const rlinewidth
			) const
			{
				SuffixPrefixAlignmentPrint::printAlignmentLines(out,a,b,SPR,rlinewidth,ta,te);
				return out;
			}
		};

		typedef SuffixPrefixTemplate<ScoreGeneric> SuffixPrefix;
		typedef SuffixPrefixTemplate<ScoreDna5> SuffixPrefixDna5;
	}
}
#endif
