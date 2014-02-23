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

#if ! defined(LIBMAUS_LCS_ALIGNMENTTRACECONTAINER_HPP)
#define LIBMAUS_LCS_ALIGNMENTTRACECONTAINER_HPP

#include <libmaus/lcs/PenaltyConstants.hpp>
#include <libmaus/autoarray/AutoArray.hpp>

namespace libmaus
{
	namespace lcs
	{
		struct AlignmentTraceContainer : public PenaltyConstants
		{
			private:
			AlignmentTraceContainer & operator=(AlignmentTraceContainer const &);
			AlignmentTraceContainer(AlignmentTraceContainer const &);
		
			public:
			virtual ~AlignmentTraceContainer() {}
		
			// trace
			::libmaus::autoarray::AutoArray<step_type> trace;
			//
			step_type * const te;
			step_type * ta;

			AlignmentTraceContainer(uint64_t const tracelen)
			: trace(tracelen), te(trace.end()), ta(te)
			{
			
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
		};
	}
}
#endif
