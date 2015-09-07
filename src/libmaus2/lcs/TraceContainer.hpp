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

#if ! defined(TRACECONTAINER_HPP)
#define TRACECONTAINER_HPP

#include <libmaus2/lcs/PenaltyConstants.hpp>

namespace libmaus2
{
	namespace lcs
	{
		struct TraceContainer : public PenaltyConstants
		{
			private:
			TraceContainer & operator=(TraceContainer const &);
			TraceContainer(TraceContainer const &);
		
			public:
			virtual ~TraceContainer() {}
		
			// trace
			::libmaus2::autoarray::AutoArray<step_type> trace;
			//
			step_type * const te;
			step_type * ta;

			TraceContainer(uint64_t const tracelen)
			: trace(tracelen), te(trace.end()), ta(te)
			{
			
			}
			
			int64_t getWindowMinScore(uint64_t const windowsize) const
			{
				step_type const * cl = ta;
				step_type const * cr = cl;
				// end of first window
				step_type const * const cw = cl + std::min(static_cast<ptrdiff_t>(windowsize),te-ta);
				int64_t score = 0;
				
				while ( cr != cw )
					switch ( *(cr++) )
					{
						case STEP_MATCH:
							score += gain_match;
							break;
						case STEP_MISMATCH:
							score -= penalty_subst;
							break;
						case STEP_INS:
							score -= penalty_ins;
							break;
						case STEP_DEL:
							score -= penalty_del;
							break;
						case STEP_RESET:
							break;
					}
					
				int64_t minscore = score;
				
				while ( cr != te )
				{
					switch ( *(cl++) )
					{
						case STEP_MATCH:
							score -= gain_match;
							break;
						case STEP_MISMATCH:
							score += penalty_subst;
							break;
						case STEP_INS:
							score += penalty_ins;
							break;
						case STEP_DEL:
							score += penalty_del;
							break;
						case STEP_RESET:
							break;
					}
					switch ( *(cr++) )
					{
						case STEP_MATCH:
							score += gain_match;
							break;
						case STEP_MISMATCH:
							score -= penalty_subst;
							break;
						case STEP_INS:
							score -= penalty_ins;
							break;
						case STEP_DEL:
							score -= penalty_del;
							break;
						case STEP_RESET:
							break;
					}
					
					#if 0
					int64_t const expected = getTraceScore(cl,cl+windowsize);
					std::cerr << "expected " << expected << " computed " << score << std::endl;
					#endif

					minscore = std::min(minscore,score);
				}	
				
				return minscore;
			}
			
			void invert()
			{
				std::reverse(ta,te);
				for ( step_type * tc = ta; tc != te; ++tc )
					if ( *tc == STEP_INS )
						*tc = STEP_DEL;
					else if ( *tc == STEP_DEL )
						*tc = STEP_INS;
			}
			
			static void invertUnmappedInPlace(std::string & t)
			{
				std::reverse(t.begin(),t.end());
				
				for ( uint64_t i = 0; i < t.size(); ++i )
					if ( t[i] == 'D' )
						t[i] = 'I';
					else if ( t[i] == 'I' )
						t[i] = 'D';
			}
			
			static std::string invertUnmapped(std::string const & s)
			{
				std::string t = s;
				invertUnmappedInPlace(t);
				return t;
			}
			
			template<typename iterator>
			void appendTrace(iterator i) const
			{
				for ( step_type const * tc = ta; tc != te; ++tc )
					*(i++) = *tc;
			}

			uint64_t getTraceLength() const
			{
				return te-ta;
			}
			
			template<typename it>
			static int32_t getTraceScore(it ta, it te)
			{
				int32_t score = 0;
				for ( it tc = ta; tc != te; ++tc )
				{
					switch ( *tc )
					{
						case STEP_MATCH:
							score += gain_match;
							break;
						case STEP_MISMATCH:
							score -= penalty_subst;
							break;
						case STEP_INS:
							score -= penalty_ins;
							break;
						case STEP_DEL:
							score -= penalty_del;
							break;
						case STEP_RESET:
							score = 0;
							break;
					}
				}
				return score;
			}
			
			template<typename it>
			static std::string traceToString(it ta, it te)
			{
				std::ostringstream ostr;

				for ( it tc = ta; tc != te; ++tc )
				{
					switch ( *tc )
					{
						case STEP_MATCH:
							ostr.put('+');
							break;
						case STEP_MISMATCH:
							ostr.put('-');
							break;
						case STEP_INS:
							ostr.put('I');
							break;
						case STEP_DEL:
							ostr.put('D');
							break;
						case STEP_RESET:
							ostr.put('R');
							break;
					}
				}
				
				return ostr.str();
			}
			
			std::string traceToString()
			{
				return traceToString(ta,te);
			}
			
			int32_t getTraceScore() const
			{
				return getTraceScore(ta,te);
			}

			uint64_t getFrontInserts() const
			{
				uint64_t i = 0;
				for ( step_type const * tc = ta; tc != te && (*tc == STEP_INS); ++tc )
					i++;
				return i;
			}
			uint64_t removeFrontInserts()
			{
				uint64_t const i = getFrontInserts();
				ta += i;
				return i;
			}

			uint64_t getBackDeletes() const
			{
				uint64_t i = 0;
				
				step_type const * tc = te;
				
				while ( tc != ta )
				{
					--tc;
					
					if ( *tc == STEP_DEL )
						i++;
					else
						break;
				}
				
				return i;
			}
			
			uint64_t removeBackDeletes()
			{
				uint64_t const d = getBackDeletes();
				::std::copy_backward( ta , te - d , te  );
				ta += d;
				return d;
			}

			uint64_t getConsecutiveMatches() const
			{
				uint64_t maxmat = 0, mat = 0;
				
				for ( step_type const * ta = this->ta; ta != this->te; ++ta )
					switch ( *ta )
					{
						case STEP_MISMATCH:
						case STEP_INS:
						case STEP_DEL:
						case STEP_RESET:
							mat = 0;
							break;
						case STEP_MATCH:
							mat += 1;
							maxmat = std::max(maxmat,mat);
							break;
					}
					
				return maxmat;
			}
			
		};
	}
}
#endif
