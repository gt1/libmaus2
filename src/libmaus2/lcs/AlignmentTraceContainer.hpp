/*
    libmaus2
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

#if ! defined(LIBMAUS2_LCS_ALIGNMENTTRACECONTAINER_HPP)
#define LIBMAUS2_LCS_ALIGNMENTTRACECONTAINER_HPP

#include <libmaus2/lcs/PenaltyConstants.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>

namespace libmaus2
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
			::libmaus2::autoarray::AutoArray<step_type> trace;
			//
			step_type * te;
			step_type * ta;

			AlignmentTraceContainer(uint64_t const tracelen = 0)
			: trace(tracelen), te(trace.end()), ta(te)
			{
			
			}
			
			void reset()
			{
				ta = te = trace.begin();
			}
			
			void push(AlignmentTraceContainer const & O)
			{
				size_t const pre = getTraceLength();
				size_t const oth = O.getTraceLength();
				
				if ( pre + oth > trace.size() )
				{
					trace.resize(pre + oth);
					
					ta = trace.begin();
					te = trace.begin()+pre;
				}
				
				for ( size_t i = 0; i < oth; ++i )
					*(te++) = O.ta[i];
			}
			
			void resize(uint64_t const tracelen)
			{
				trace = ::libmaus2::autoarray::AutoArray<step_type>(tracelen,false);
				te = trace.end();
				ta = te;
			}
			
			uint64_t capacity() const
			{
				return trace.size();
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
			
			bool operator==(AlignmentTraceContainer const & o) const
			{
				if ( getTraceLength() != o.getTraceLength() )
					return false;
				
				step_type *  tc =   ta;
				step_type * otc = o.ta;
				
				while ( tc != te )
					if ( *(tc++) != *(otc++) )
						return false;
						
				return true;
			}
			
			bool operator!=(AlignmentTraceContainer const & o) const
			{
				return ! operator==(o);
			}
		};
	}
}
#endif
