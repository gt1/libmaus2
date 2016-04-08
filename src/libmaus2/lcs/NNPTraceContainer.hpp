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

			void copyFrom(NNPTraceContainer const & O)
			{
				Atrace.ensureSize(O.Atrace.size());
				std::copy(O.Atrace.begin(),O.Atrace.end(),Atrace.begin());
				traceid = O.traceid;
				otrace = O.otrace;
			}

			shared_ptr_type sclone() const
			{
				shared_ptr_type sptr(new this_type);
				sptr->copyFrom(*this);
				return sptr;
			}

			unique_ptr_type uclone() const
			{
				unique_ptr_type uptr(new this_type);
				uptr->copyFrom(*this);
				return uptr;
			}

			void traceToSparse(libmaus2::lcs::BaseConstants::step_type * ta, libmaus2::lcs::BaseConstants::step_type * te)
			{
				otrace = 0;
				traceid = -1;

				int64_t parent = -1;

				if ( ta == te )
					return;

				// slide before first op
				uint64_t slide = 0;
				while ( ta != te && (*ta == libmaus2::lcs::BaseConstants::STEP_MATCH) )
					slide++, ta++;

				// if first op is a match
				if ( slide )
				{
					NNPTraceElement T;
					T.step = libmaus2::lcs::BaseConstants::STEP_RESET;
					T.slide = slide;
					T.parent = parent;
					parent = otrace;
					Atrace.push(otrace,T);
				}

				// while we have not reached the end
				while ( ta != te )
				{
					// this should not be a match
					assert ( *ta != libmaus2::lcs::BaseConstants::STEP_MATCH );

					NNPTraceElement T;
					T.step = *(ta++);

					slide = 0;
					while ( ta != te && (*ta == libmaus2::lcs::BaseConstants::STEP_MATCH) )
						slide++, ta++;
					T.slide = slide;
					T.parent = parent;
					parent = otrace;
					Atrace.push(otrace,T);
				}

				traceid = parent;
			}

			void traceToSparse(libmaus2::lcs::AlignmentTraceContainer const & ATC)
			{
				traceToSparse(ATC.ta,ATC.te);
			}

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

			struct MatMisDelResult
			{
				uint64_t mat;
				uint64_t mis;
				uint64_t del;
				uint64_t ins;

				MatMisDelResult() {}
			};

			MatMisDelResult countMatMisDel(uint64_t matmisdel, uint64_t matmisdelskip = 0) const
			{
				std::stack < int64_t > S;

				for ( int64_t curtraceid = traceid ; curtraceid >= 0; curtraceid = Atrace[curtraceid].parent )
					S.push(curtraceid);

				uint64_t mat = 0, mis = 0, del = 0, ins = 0;
				while (
					(matmisdel || matmisdelskip)
					&&
					(! S.empty())
				)
				{
					NNPTraceElement const & E = Atrace[S.top()];
					S.pop();

					if ( matmisdelskip )
						switch ( E.step )
						{
							case libmaus2::lcs::BaseConstants::STEP_MISMATCH:
							case libmaus2::lcs::BaseConstants::STEP_DEL:
								matmisdelskip -= 1;
								matmisdelskip -= 1;
								break;
							default:
								break;
						}
					else
						switch ( E.step )
						{
							case libmaus2::lcs::BaseConstants::STEP_MISMATCH:
								mis += 1;
								matmisdel -= 1;
								break;
							case libmaus2::lcs::BaseConstants::STEP_DEL:
								del += 1;
								matmisdel -= 1;
								break;
							case libmaus2::lcs::BaseConstants::STEP_INS:
								ins += 1;
								break;
							default:
								break;
						}

					for ( int64_t i = 0; i < E.slide && (matmisdel || matmisdelskip); ++i )
					{
						if ( matmisdelskip )
							matmisdelskip -= 1;
						else
						{
							mat += 1;
							matmisdel -= 1;
						}
					}
				}

				MatMisDelResult DMR;
				DMR.mat = mat;
				DMR.mis = mis;
				DMR.ins = ins;
				DMR.del = del;

				return DMR;
			}

			std::map<int64_t, uint64_t> getDiagonalHistogram() const
			{
				std::stack < int64_t > S;

				for ( int64_t curtraceid = traceid ; curtraceid >= 0; curtraceid = Atrace[curtraceid].parent )
					S.push(curtraceid);

				std::map<int64_t,uint64_t> M;

				int64_t apos = 0, bpos = 0;

				while ( ! S.empty() )
				{
					NNPTraceElement const & E = Atrace[S.top()];
					S.pop();

					switch ( E.step )
					{
						case libmaus2::lcs::BaseConstants::STEP_DEL:
							apos += 1;
							break;
						case libmaus2::lcs::BaseConstants::STEP_INS:
							bpos += 1;
							break;
						default:
							break;
					}

					int64_t const d = apos-bpos;

					M [ d ] += E.slide;
				}

				return M;
			}

			std::pair<int64_t,int64_t> getDiagonalBand(int64_t apos, int64_t bpos) const
			{
				std::stack < int64_t > S;

				for ( int64_t curtraceid = traceid ; curtraceid >= 0; curtraceid = Atrace[curtraceid].parent )
					S.push(curtraceid);

				std::map<int64_t,uint64_t> M;

				int64_t dmin = apos-bpos;
				int64_t dmax = dmin;

				while ( ! S.empty() )
				{
					NNPTraceElement const & E = Atrace[S.top()];
					S.pop();

					switch ( E.step )
					{
						case libmaus2::lcs::BaseConstants::STEP_DEL:
							apos += 1;
							break;
						case libmaus2::lcs::BaseConstants::STEP_INS:
							bpos += 1;
							break;
						default:
							break;
					}

					int64_t const d = apos-bpos;

					if ( d < dmin )
						dmin = d;
					if ( dmax < d )
						dmax = d;

					M [ d ] += E.slide;
				}

				return std::pair<int64_t,int64_t>(dmin,dmax);
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

			struct NNPTraceContainerDecoder
			{
				NNPTraceContainer const & A;
				std::stack < int64_t > SA;

				std::deque< std::pair < libmaus2::lcs::BaseConstants::step_type, uint64_t > > Q;

				libmaus2::lcs::BaseConstants::step_type peekslot;
				bool peekslotfilled;

				NNPTraceContainerDecoder(NNPTraceContainer const & rA) : A(rA), peekslotfilled(false)
				{
					for ( int64_t curtraceid = A.traceid ; curtraceid >= 0; curtraceid = A.Atrace[curtraceid].parent )
						SA.push(curtraceid);
				}

				bool peekNext(libmaus2::lcs::BaseConstants::step_type & step)
				{
					if ( ! peekslotfilled )
						peekslotfilled = getNext(peekslot);

					step = peekslot;
					return peekslotfilled;
				}

				bool getNext(libmaus2::lcs::BaseConstants::step_type & step)
				{
					if ( peekslotfilled )
					{
						step = peekslot;
						peekslotfilled = false;
						return true;
					}

					while ( ! Q.size() )
					{
						if ( SA.empty() )
							return false;

						NNPTraceElement const & E = A.Atrace[SA.top()];
						SA.pop();

						switch ( E.step )
						{
							case libmaus2::lcs::BaseConstants::STEP_INS:
							case libmaus2::lcs::BaseConstants::STEP_DEL:
							case libmaus2::lcs::BaseConstants::STEP_MISMATCH:
								Q.push_back(std::pair < libmaus2::lcs::BaseConstants::step_type, uint64_t >(E.step,1));
								break;
							default:
								break;
						}

						if ( E.slide )
							Q.push_back(std::pair < libmaus2::lcs::BaseConstants::step_type, uint64_t >(libmaus2::lcs::BaseConstants::STEP_MATCH,E.slide));
					}

					assert ( Q.size() );
					assert ( Q.front().second );

					step = Q.front().first;

					if ( ! (--(Q.front().second)) )
						Q.pop_front();

					return true;
				}
			};

			struct NNPTraceContainerDecoderReverse
			{
				NNPTraceContainer const & A;
				int64_t traceid;

				std::deque< std::pair < libmaus2::lcs::BaseConstants::step_type, uint64_t > > Q;

				libmaus2::lcs::BaseConstants::step_type peekslot;
				bool peekslotfilled;

				NNPTraceContainerDecoderReverse(NNPTraceContainer const & rA) : A(rA), traceid(A.traceid)
				{
				}

				bool peekNext(libmaus2::lcs::BaseConstants::step_type & step)
				{
					if ( ! peekslotfilled )
						peekslotfilled = getNext(peekslot);

					step = peekslot;
					return peekslotfilled;
				}

				bool getNext(libmaus2::lcs::BaseConstants::step_type & step)
				{
					if ( peekslotfilled )
					{
						step = peekslot;
						peekslotfilled = false;
						return true;
					}

					while ( ! Q.size() )
					{
						if ( traceid < 0 )
							return false;

						NNPTraceElement const & E = A.Atrace[traceid];
						traceid = E.parent;

						if ( E.slide )
							Q.push_back(std::pair < libmaus2::lcs::BaseConstants::step_type, uint64_t >(libmaus2::lcs::BaseConstants::STEP_MATCH,E.slide));

						switch ( E.step )
						{
							case libmaus2::lcs::BaseConstants::STEP_INS:
							case libmaus2::lcs::BaseConstants::STEP_DEL:
							case libmaus2::lcs::BaseConstants::STEP_MISMATCH:
								Q.push_back(std::pair < libmaus2::lcs::BaseConstants::step_type, uint64_t >(E.step,1));
								break;
							default:
								break;
						}
					}

					assert ( Q.size() );
					assert ( Q.front().second );

					step = Q.front().first;

					if ( ! (--(Q.front().second)) )
						Q.pop_front();

					return true;
				}
			};

			struct TracePointId
			{
				uint64_t apos;
				uint64_t bpos;
				uint64_t id;

				TracePointId(uint64_t const rapos = 0, uint64_t const rbpos = 0, uint64_t rid = 0) : apos(rapos), bpos(rbpos), id(rid) {}

				bool operator<(TracePointId const & O) const
				{
					if ( apos != O.apos )
						return apos < O.apos;
					else if ( bpos != O.bpos )
						return bpos < O.bpos;
					else
						return id < O.id;
				}
			};

			uint64_t getTracePoints(uint64_t apos, uint64_t bpos, uint64_t tspace, libmaus2::autoarray::AutoArray< TracePointId > & A, uint64_t o, uint64_t const id)
			{
				std::pair<uint64_t,uint64_t> const SL = getStringLengthUsed();
				apos += SL.first;
				bpos += SL.second;

				assert ( tspace );
				uint64_t tdistance = apos % tspace;

				NNPTraceContainerDecoderReverse dec(*this);

				uint64_t oa = o;

				if ( ! tdistance )
				{
					A.push(o, TracePointId(apos,bpos,id));
					tdistance = tspace;
				}

				libmaus2::lcs::BaseConstants::step_type step;
				while ( dec.getNext(step) )
				{
					switch ( step )
					{
						case ::libmaus2::lcs::BaseConstants::STEP_INS:
							bpos -= 1;
							break;
						case libmaus2::lcs::BaseConstants::STEP_DEL:
							apos -= 1;
							tdistance -= 1;
							break;
						case libmaus2::lcs::BaseConstants::STEP_MATCH:
						case libmaus2::lcs::BaseConstants::STEP_MISMATCH:
							apos -= 1;
							tdistance -= 1;
							bpos -= 1;
						default:
							break;
					}

					if ( ! tdistance )
					{
						A.push(o, TracePointId(apos,bpos,id));
						tdistance = tspace;
					}
				}

				std::reverse(A.begin()+oa,A.begin()+o);

				return o;
			}

			template<typename iterator>
			static uint64_t getCommonTracePoints(
				iterator it, iterator ite, libmaus2::autoarray::AutoArray< TracePointId > & A,
				libmaus2::autoarray::AutoArray<uint64_t> & Ashare,
				uint64_t const tspace
			)
			{
				uint64_t o = 0;
				uint64_t ashareo = 0;

				for ( uint64_t id = 0; it != ite; ++it, ++id )
				{
					o = it->second->getTracePoints(it->first.abpos,it->first.bbpos,tspace,A,o,id);
					Ashare.push(ashareo,id);
				}

				std::sort(A.begin(),A.begin()+o);

				uint64_t alow = 0;
				while ( alow < o )
				{
					uint64_t ahigh = alow+1;

					while ( ahigh < o && A[alow].apos == A[ahigh].apos )
						++ahigh;

					if ( ahigh-alow > 1 )
					{
						uint64_t blow = alow;

						while ( blow < ahigh )
						{
							uint64_t bhigh = blow + 1;

							while ( bhigh < ahigh && A[blow].bpos == A[bhigh].bpos )
								++bhigh;

							if ( bhigh-blow > 1 )
							{
								for ( uint64_t i = blow; i < bhigh; i += 2 )
								{
									uint64_t id0 = A[i].id;

									if ( i+1 < bhigh )
									{
										uint64_t id1 = A[i+1].id;
										assert ( id0 != id1 );

										if ( id0 < id1 )
											Ashare [ id1 ] = id0;
										else
											Ashare [ id0 ] = id1;
									}
								}
							}

							blow = bhigh;
						}
					}

					alow = ahigh;
				}

				return ashareo;
			}

			static bool cross(
				NNPTraceContainer const & A, int64_t aapos, int64_t abpos,
				NNPTraceContainer const & B, int64_t bapos, int64_t bbpos
			)
			{
				NNPTraceContainerDecoderReverse decA(A);
				NNPTraceContainerDecoderReverse decB(B);

				libmaus2::lcs::BaseConstants::step_type stepA;
				libmaus2::lcs::BaseConstants::step_type stepB;

				std::pair<uint64_t,uint64_t> const SLA = A.getStringLengthUsed();
				std::pair<uint64_t,uint64_t> const SLB = B.getStringLengthUsed();

				aapos += SLA.first;
				abpos += SLA.second;
				bapos += SLB.first;
				bbpos += SLB.second;

				while ( (! (aapos == bapos && abpos == bbpos)) && decA.peekNext(stepA) && decB.peekNext(stepB) )
				{
					if ( (aapos > bapos) || (aapos == bapos && abpos > bbpos) )
					{
						decA.getNext(stepA);

						switch ( stepA )
						{
							case ::libmaus2::lcs::BaseConstants::STEP_INS:
								abpos -= 1;
								break;
							case libmaus2::lcs::BaseConstants::STEP_DEL:
								aapos -= 1;
								break;
							case libmaus2::lcs::BaseConstants::STEP_MATCH:
							case libmaus2::lcs::BaseConstants::STEP_MISMATCH:
								aapos -= 1;
								abpos -= 1;
							default:
								break;
						}
					}
					else
					{
						assert ( (bapos > aapos) || (bapos == aapos && bbpos > abpos) );

						decB.getNext(stepB);

						switch ( stepB )
						{
							case libmaus2::lcs::BaseConstants::STEP_INS:
								bbpos -= 1;
								break;
							case libmaus2::lcs::BaseConstants::STEP_DEL:
								bapos -= 1;
								break;
							case libmaus2::lcs::BaseConstants::STEP_MATCH:
							case libmaus2::lcs::BaseConstants::STEP_MISMATCH:
								bapos -= 1;
								bbpos -= 1;
							default:
								break;
						}
					}
				}

				while ( (! (aapos == bapos && abpos == bbpos)) && decA.peekNext(stepA) )
				{
					decA.getNext(stepA);

					switch ( stepA )
					{
						case ::libmaus2::lcs::BaseConstants::STEP_INS:
							abpos -= 1;
							break;
						case libmaus2::lcs::BaseConstants::STEP_DEL:
							aapos -= 1;
							break;
						case libmaus2::lcs::BaseConstants::STEP_MATCH:
						case libmaus2::lcs::BaseConstants::STEP_MISMATCH:
							aapos -= 1;
							abpos -= 1;
						default:
							break;
					}
				}

				while ( (! (aapos == bapos && abpos == bbpos)) && decB.peekNext(stepB) )
				{
					decB.getNext(stepB);

					switch ( stepB )
					{
						case libmaus2::lcs::BaseConstants::STEP_INS:
							bbpos -= 1;
							break;
						case libmaus2::lcs::BaseConstants::STEP_DEL:
							bapos -= 1;
							break;
						case libmaus2::lcs::BaseConstants::STEP_MATCH:
						case libmaus2::lcs::BaseConstants::STEP_MISMATCH:
							bapos -= 1;
							bbpos -= 1;
						default:
							break;
					}

				}

				bool const cr = (aapos == bapos && abpos == bbpos);

				#if 0
				if ( cr )
					std::cerr << "cross at (" << aapos << "," << abpos << "),(" << bapos << "," << bbpos << ")" << std::endl;
				#endif

				return cr;
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
