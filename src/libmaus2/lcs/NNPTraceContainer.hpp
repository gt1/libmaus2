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
#include <stack>

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

			void swap(NNPTraceContainer & O)
			{
				if ( this != &O )
				{
					Atrace.swap(O.Atrace);
					std::swap(traceid,O.traceid);
					std::swap(otrace,O.otrace);
				}
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

			template<typename iterator>
			bool checkTrace(iterator a, iterator b) const
			{
				std::pair<uint64_t,uint64_t> const P = getStringLengthUsed();
				iterator ac = a + P.first;
				iterator bc = b + P.second;

				int64_t cur = traceid;
				bool ok = true;
				while ( cur >= 0 )
				{
					for ( uint64_t i = 0; i < Atrace[cur].slide; ++i )
						ok = ok && ((*(--ac)) == (*(--bc)));

					switch ( Atrace[cur].step )
					{
						case libmaus2::lcs::BaseConstants::STEP_MISMATCH:
							ok = ok && ((*(--ac)) != (*(--bc)));
							break;
						case libmaus2::lcs::BaseConstants::STEP_DEL:
							--ac;
							break;
						case libmaus2::lcs::BaseConstants::STEP_INS:
							--bc;
							break;
						default:
							break;
					}

					cur = Atrace[cur].parent;
				}

				ok = ok && (ac == a);
				ok = ok && (bc == b);

				return ok;
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

			template<typename value_type>
			static value_type defaultRemapFunction(value_type c)
			{
				return c;
			}

			template<typename iterator>
			void printTraceLines(
				std::ostream & out,
				iterator a, iterator b,
				uint64_t const linelength = 80,
				std::string const & indent = std::string(),
				std::string const & linesep = std::string(),
				typename ::std::iterator_traits<iterator>::value_type (*remapFunction)(typename ::std::iterator_traits<iterator>::value_type) = defaultRemapFunction
			) const
			{
				std::stack < int64_t > S;
				bool firstline = true;

				for ( int64_t curtraceid = traceid ; curtraceid >= 0; curtraceid = Atrace[curtraceid].parent )
					S.push(curtraceid);

				std::string aline(linelength,' ');
				std::string bline(linelength,' ');
				std::string opline(linelength,' ');
				uint64_t o = 0;

				while ( ! S.empty() )
				{
					NNPTraceElement const & E = Atrace[S.top()];
					S.pop();

					switch ( E.step )
					{
						case libmaus2::lcs::BaseConstants::STEP_INS:
							aline[o] = '-';
							bline[o] = remapFunction(*(b++));
							opline[o] = 'I';
							o += 1;
							break;
						case libmaus2::lcs::BaseConstants::STEP_DEL:
							aline[o] = remapFunction(*(a++));
							bline[o] = '-';
							opline[o] = 'D';
							o += 1;
							break;
						case libmaus2::lcs::BaseConstants::STEP_MISMATCH:
							aline[o] = remapFunction(*(a++));
							bline[o] = remapFunction(*(b++));
							opline[o] = '-';
							o += 1;
							break;
						default:
							break;
					}

					if ( o == linelength )
					{
						if ( !firstline )
							out << linesep;
						firstline = false;

						out << indent << aline << "\n";
						out << indent << bline << "\n";
						out << indent << opline << "\n";
						o = 0;
					}

					for ( uint64_t i = 0; i < E.slide; ++i )
					{
						aline[o] = remapFunction(*(a++));
						bline[o] = remapFunction(*(b++));
						opline[o] = '+';
						o += 1;

						if ( o == linelength )
						{
							if ( !firstline )
								out << linesep;
							firstline = false;

							out << indent << aline << "\n";
							out << indent << bline << "\n";
							out << indent << opline << "\n";
							o = 0;
						}
					}
				}

				if ( o )
				{
					if ( !firstline )
						out << linesep;
					firstline = false;

					out << indent << aline << "\n";
					out << indent << bline << "\n";
					out << indent << opline << "\n";
					o = 0;
				}
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

		struct NNPTraceContainerAllocator
		{
			NNPTraceContainerAllocator() {}

			NNPTraceContainer::shared_ptr_type operator()()
			{
				NNPTraceContainer::shared_ptr_type tptr(new NNPTraceContainer);
				return tptr;
			}
		};

		struct NNPTraceContainerTypeInfo
		{
			typedef NNPTraceContainerTypeInfo this_type;

			typedef libmaus2::lcs::NNPTraceContainer::shared_ptr_type pointer_type;

			static pointer_type getNullPointer()
			{
				pointer_type p;
				return p;
			}

			static pointer_type deallocate(pointer_type /* p */)
			{
				return getNullPointer();
			}
		};
	}
}
#endif
