/*
    libmaus2
    Copyright (C) 2016 German Tischler

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
#if ! defined(LIBMAUS2_LCS_NNPTRACECONTAINER_HPP)
#define LIBMAUS2_LCS_NNPTRACECONTAINER_HPP

#include <libmaus2/lcs/NNPTraceElement.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/lcs/AlignmentTraceContainer.hpp>

namespace libmaus2
{
	namespace lcs
	{
		struct NNPTraceContainer
		{
			typedef NNPTraceContainer this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			libmaus2::autoarray::AutoArray<NNPTraceElement> Atrace;
			int64_t traceid;
			uint64_t otrace;

			NNPTraceContainer() : traceid(-1), otrace(0)
			{}

			void reset()
			{
				traceid = -1;
				otrace = 0;
			}

			std::ostream & print(std::ostream & out) const
			{
				int64_t cur = traceid;
				while ( cur >= 0 )
				{
					out << Atrace[cur];
					cur = Atrace[cur].parent;
				}
				return out;
			}

			std::pair<uint64_t,uint64_t> getStringLengthUsed() const
			{
				uint64_t alen = 0, blen = 0;
				int64_t cur = traceid;
				while ( cur >= 0 )
				{
					alen += Atrace[cur].slide;
					blen += Atrace[cur].slide;

					switch ( Atrace[cur].step )
					{
						case libmaus2::lcs::BaseConstants::STEP_MISMATCH:
							alen += 1;
							blen += 1;
							break;
						case libmaus2::lcs::BaseConstants::STEP_DEL:
							alen += 1;
							break;
						case libmaus2::lcs::BaseConstants::STEP_INS:
							blen += 1;
							break;
						default:
							break;
					}

					cur = Atrace[cur].parent;
				}

				return std::pair<uint64_t,uint64_t>(alen,blen);
			}

			uint64_t getNumDif() const
			{
				uint64_t dif = 0;
				int64_t cur = traceid;
				while ( cur >= 0 )
				{
					switch ( Atrace[cur].step )
					{
						case libmaus2::lcs::BaseConstants::STEP_MISMATCH:
						case libmaus2::lcs::BaseConstants::STEP_DEL:
						case libmaus2::lcs::BaseConstants::STEP_INS:
							dif += 1;
							break;
						default:
							break;
					}

					cur = Atrace[cur].parent;
				}

				return dif;
			}

			void suffixPositive(
				int64_t const match_score    = PenaltyConstants::gain_match,
			        int64_t const mismatch_score = PenaltyConstants::penalty_subst,
			        int64_t const ins_score      = PenaltyConstants::penalty_ins,
			        int64_t const del_score      = PenaltyConstants::penalty_del
			)
			{
				int64_t score = 0;
				int64_t cur = traceid;
				while ( cur >= 0 )
				{
					score += match_score * Atrace[cur].slide;

					switch ( Atrace[cur].step )
					{
						case libmaus2::lcs::BaseConstants::STEP_MISMATCH:
							score -= mismatch_score;
							break;
						case libmaus2::lcs::BaseConstants::STEP_DEL:
							score -= del_score;
							break;
						case libmaus2::lcs::BaseConstants::STEP_INS:
							score -= ins_score;
							break;
						default:
							break;
					}

					if ( score < 0 )
					{
						score = 0;
						traceid = Atrace[cur].parent;
					}

					cur = Atrace[cur].parent;
				}
			}

			int64_t concat(int64_t node1, int64_t node2)
			{
				if ( node2 >= 0 )
				{
					int64_t pparent = -1;
					int64_t parent = -1;
					int64_t cur = node2;
					while ( cur >= 0 )
					{
						pparent = parent;
						parent = cur;
						cur = Atrace[cur].parent;
					}
					assert ( cur < 0 );
					assert ( parent >= 0 );
					assert ( Atrace[parent].parent < 0 );

					// remove noop trace node if possible
					if ( pparent >= 0 && parent >= 0 && node1 >= 0 && Atrace[parent].step == libmaus2::lcs::BaseConstants::STEP_RESET )
					{
						Atrace[node1].slide += Atrace[parent].slide;
						Atrace[pparent].parent = node1;
					}
					else
					{
						Atrace[parent].parent = node1;
					}

					return node2;
				}
				else
				{
					return node1;
				}
			}

			int64_t reverse(int64_t cur)
			{
				int64_t parent = -1;
				libmaus2::lcs::BaseConstants::step_type prevstep = libmaus2::lcs::BaseConstants::STEP_RESET;

				while ( cur >= 0 )
				{
					int64_t const parsave = Atrace[cur].parent;
					libmaus2::lcs::BaseConstants::step_type stepsave = Atrace[cur].step;
					Atrace[cur].parent = parent;
					Atrace[cur].step = prevstep;
					parent = cur;
					cur = parsave;
					prevstep = stepsave;
				}

				return parent;
			}

			void reverse()
			{
				traceid = reverse(traceid);
			}

			static void computeTrace(libmaus2::autoarray::AutoArray<NNPTraceElement> const & Atrace, int64_t const traceid, libmaus2::lcs::AlignmentTraceContainer & ATC)
			{
				uint64_t reserve = 0;
				for ( int64_t curtraceid = traceid ; curtraceid >= 0; curtraceid = Atrace[curtraceid].parent )
				{
					switch ( Atrace[curtraceid].step )
					{
						case libmaus2::lcs::BaseConstants::STEP_INS:
						case libmaus2::lcs::BaseConstants::STEP_DEL:
						case libmaus2::lcs::BaseConstants::STEP_MISMATCH:
							reserve += 1;
							break;
						default:
							break;
					}
					reserve += Atrace[curtraceid].slide;
				}

				if ( ATC.capacity() < reserve )
					ATC.resize(reserve);
				ATC.reset();

				for ( int64_t curtraceid = traceid ; curtraceid >= 0; curtraceid = Atrace[curtraceid].parent )
				{
					for ( int64_t i = 0; i < Atrace[curtraceid].slide; ++i )
						*(--ATC.ta) = libmaus2::lcs::BaseConstants::STEP_MATCH;

					switch ( Atrace[curtraceid].step )
					{
						case libmaus2::lcs::BaseConstants::STEP_INS:
						case libmaus2::lcs::BaseConstants::STEP_DEL:
						case libmaus2::lcs::BaseConstants::STEP_MISMATCH:
							*(--ATC.ta) = Atrace[curtraceid].step;
							break;
						default:
							break;
					}
				}

				assert ( ATC.ta >= ATC.trace.begin() );

				#if 0
				std::cerr << "here" << std::endl;
				std::cerr << ATC.getStringLengthUsed().first << std::endl;
				std::cerr << ATC.getStringLengthUsed().second << std::endl;
				#endif
			}

			void computeTrace(libmaus2::lcs::AlignmentTraceContainer & ATC) const
			{
				computeTrace(Atrace,traceid,ATC);
			}
		};

		std::ostream & operator<<(std::ostream & out, NNPTraceContainer const & T);
	}
}
#endif
