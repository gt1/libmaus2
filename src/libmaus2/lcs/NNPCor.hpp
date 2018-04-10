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
#if ! defined(LIBMAUS2_LCS_NNPCOR_HPP)
#define LIBMAUS2_LCS_NNPCOR_HPP

#include <libmaus2/lcs/NNPAlignResult.hpp>
#include <libmaus2/lcs/NNPTraceContainer.hpp>
#include <libmaus2/lcs/NNPTraceElement.hpp>
#include <libmaus2/lcs/NPLinMem.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/lcs/AlignmentTraceContainer.hpp>
#include <libmaus2/rank/popcnt.hpp>
#include <cstdlib>

namespace libmaus2
{
	namespace lcs
	{
		struct NNPCor
		{
			typedef NNPCor this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			typedef int32_t score_type;
			typedef int32_t offset_type;

			static unsigned int popcnt(uint64_t const w)
			{
				return libmaus2::rank::PopCnt8<sizeof(long)>::popcnt8(w);
			}

			struct DiagElement
			{
				score_type score;
				offset_type offset;
				int32_t id;
				uint64_t evec;

				bool operator==(DiagElement const & o) const
				{
					return score == o.score && offset == o.offset && id == o.id && evec == o.evec;
				}
			};

			struct MidDiagElement
			{
				score_type score;
				offset_type offset;
				int32_t id;
				uint64_t evec;
				std::pair<int32_t,int32_t> mid;

				bool operator==(MidDiagElement const & o) const
				{
					return score == o.score && offset == o.offset && id == o.id && evec == o.evec;
				}
			};

			// maximimum correlation error when clipping
			unsigned int const maxcerr;
			// discard traces which have more than this number of errors in the last 64 columns (local error check)
			unsigned int const maxwerr;
			// discard traces which are this far more back than the furthest
			int64_t const maxback;
			// unique terminal value used
			bool const funiquetermval;
			//
			bool const runsuffixpositive;

			libmaus2::autoarray::AutoArray<DiagElement,libmaus2::autoarray::alloc_type_c> Acontrol;
			DiagElement * control;
			libmaus2::autoarray::AutoArray<DiagElement,libmaus2::autoarray::alloc_type_c> Ancontrol;
			DiagElement * ncontrol;

			libmaus2::autoarray::AutoArray<MidDiagElement,libmaus2::autoarray::alloc_type_c> Amidcontrol;
			MidDiagElement * midcontrol;
			libmaus2::autoarray::AutoArray<MidDiagElement,libmaus2::autoarray::alloc_type_c> Anmidcontrol;
			MidDiagElement * nmidcontrol;

			libmaus2::lcs::NPLinMem npobj;

			static unsigned int getWindowSize()
			{
				return CHAR_BIT*sizeof(uint64_t);
			}

			static double getDefaultMinCorrelation()
			{
				return 0.7;
			}

			static double getDefaultMinLocalCorrelation()
			{
				return 0.53;
			}

			static bool getDefaultUniqueTermVal()
			{
				return false;
			}

			static bool getDefaultRunSuffixPositive()
			{
				return true;
			}

			static unsigned int getDefaultMaxBack()
			{
				return 75;
			}

			struct CompInfo
			{
				int64_t d;
				bool prevactive;
				bool thisactive;
				bool nextactive;

				CompInfo(
					int64_t rd = 0,
					bool rprevactive = false,
					bool rthisactive = false,
					bool rnextactive = false
				) : d(rd), prevactive(rprevactive), thisactive(rthisactive), nextactive(rnextactive)
				{

				}
			};

			libmaus2::autoarray::AutoArray<CompInfo> Acompdiag;
			libmaus2::autoarray::AutoArray<int64_t> Aactivediag;

			NNPCor(
				double const mincor = getDefaultMinCorrelation(),
				double const minlocalcor = getDefaultMinLocalCorrelation(),
				int64_t const rmaxback = getDefaultMaxBack(),
				bool const rfuniquetermval = getDefaultUniqueTermVal(),
				bool const rrunsuffixpositive = getDefaultRunSuffixPositive()
			)
			:
				maxcerr( std::floor(( 1.0 - mincor      ) * getWindowSize() + 0.5) ),
				maxwerr( std::floor(( 1.0 - minlocalcor ) * getWindowSize() + 0.5) ),
				maxback(rmaxback),
				funiquetermval(rfuniquetermval),
				runsuffixpositive(rrunsuffixpositive),
				Acontrol(1), control(Acontrol.begin()),
				Ancontrol(1), ncontrol(Ancontrol.begin()),
				Amidcontrol(1), midcontrol(Amidcontrol.begin()),
				Anmidcontrol(1), nmidcontrol(Anmidcontrol.begin())
			{
			}

			void reset()
			{
				Acontrol = libmaus2::autoarray::AutoArray<DiagElement,libmaus2::autoarray::alloc_type_c>(1);
				control = Acontrol.begin();
				Ancontrol = libmaus2::autoarray::AutoArray<DiagElement,libmaus2::autoarray::alloc_type_c>(1);
				ncontrol = Ancontrol.begin();

				Amidcontrol = libmaus2::autoarray::AutoArray<MidDiagElement,libmaus2::autoarray::alloc_type_c>(1);
				midcontrol = Amidcontrol.begin();
				Anmidcontrol = libmaus2::autoarray::AutoArray<MidDiagElement,libmaus2::autoarray::alloc_type_c>(1);
				nmidcontrol = Anmidcontrol.begin();
			}

			template<typename iterator, bool uniquetermval>
			static uint64_t slide(iterator a, iterator ae, iterator b, iterator be)
			{
				if ( uniquetermval )
				{
					iterator as = a;
					while ( *a == *b )
						++a, ++b;
					return a-as;
				}
				else
				{
					ptrdiff_t const alen = ae-a;
					ptrdiff_t const blen = be-b;

					if ( alen <= blen )
					{
						iterator as = a;
						while ( a != ae && *a == *b )
							++a, ++b;
						return a - as;
					}
					else
					{
						iterator bs = b;
						while ( b != be && *a == *b )
							++a, ++b;
						return b-bs;
					}
				}
			}

			template<typename iterator, bool uniquetermval>
			static uint64_t slider(iterator a, iterator ae, iterator b, iterator be)
			{
				if ( uniquetermval )
				{
					iterator ac = ae;

					while ( *(--ac) == *(--be) )
					{}

					return (ae-ac)-1;
				}
				else
				{
					ptrdiff_t const alen = ae-a;
					ptrdiff_t const blen = be-b;

					if ( alen <= blen )
					{
						iterator ac = ae;

						while ( ac-- != a )
							if ( *(--be) != *ac )
								break;

						return (ae-ac)-1;
					}
					else
					{
						iterator bc = be;
						while ( bc-- != b )
							if ( *(--ae) != *bc )
								break;

						return (be-bc)-1;
					}
				}
			}

			static void
				allocate(
					int64_t const mindiag, int64_t const maxdiag,
					libmaus2::autoarray::AutoArray<DiagElement,libmaus2::autoarray::alloc_type_c> & Acontrol,
					DiagElement * & control
				)
			{
				// maximum absolute diagonal index
				int64_t const maxabsdiag = std::max(std::abs(mindiag),std::abs(maxdiag));
				// required diagonals
				int64_t const reqdiag = 2*maxabsdiag + 1;

				#if 0
				std::map<int,DiagElement> M;
				for ( DiagElement * p = Acontrol.begin(); p != Acontrol.end(); ++p )
				{
					ptrdiff_t off = p - control;
					M [ off ] = *p;
				}
				#endif

				// while array is too small
				while ( reqdiag > static_cast<int64_t>(Acontrol.size()) )
				{
					// previous max absolute diagonal index
					ptrdiff_t const oldabsdiag = control - Acontrol.begin();
					// one
					ptrdiff_t const one = 1;
					// new max absolute diagonal index
					ptrdiff_t const newabsdiag = std::max(2*oldabsdiag,one);

					libmaus2::autoarray::AutoArray<DiagElement,libmaus2::autoarray::alloc_type_c> Anewcontrol(2*newabsdiag+1,false);
					std::copy(Acontrol.begin(),Acontrol.end(),Anewcontrol.begin() + (newabsdiag - oldabsdiag));

					DiagElement * newcontrol = Anewcontrol.begin() + newabsdiag;

					#if defined(LIBMAUS2_LCS_NNP_DEBUG)
					for ( int64_t i = -oldabsdiag; i <= oldabsdiag; ++i )
						assert ( newcontrol[i] == control[i] );
					#endif

					Acontrol = Anewcontrol;
					control = newcontrol;
				}

				#if 0
				for ( std::map<int,DiagElement>::const_iterator ita = M.begin(); ita != M.end(); ++ita )
					assert (
						control[ita->first] == ita->second
					);
				#endif
			}

			static void
				allocate(
					int64_t const mindiag, int64_t const maxdiag,
					libmaus2::autoarray::AutoArray<MidDiagElement,libmaus2::autoarray::alloc_type_c> & Acontrol,
					MidDiagElement * & control
				)
			{
				// maximum absolute diagonal index
				int64_t const maxabsdiag = std::max(std::abs(mindiag),std::abs(maxdiag));
				// required diagonals
				int64_t const reqdiag = 2*maxabsdiag + 1;

				#if 0
				std::map<int,MidDiagElement> M;
				for ( MidDiagElement * p = Acontrol.begin(); p != Acontrol.end(); ++p )
				{
					ptrdiff_t off = p - control;
					M [ off ] = *p;
				}
				#endif

				// while array is too small
				while ( reqdiag > static_cast<int64_t>(Acontrol.size()) )
				{
					// previous max absolute diagonal index
					ptrdiff_t const oldabsdiag = control - Acontrol.begin();
					// one
					ptrdiff_t const one = 1;
					// new max absolute diagonal index
					ptrdiff_t const newabsdiag = std::max(2*oldabsdiag,one);

					libmaus2::autoarray::AutoArray<MidDiagElement,libmaus2::autoarray::alloc_type_c> Anewcontrol(2*newabsdiag+1,false);
					std::copy(Acontrol.begin(),Acontrol.end(),Anewcontrol.begin() + (newabsdiag - oldabsdiag));

					MidDiagElement * newcontrol = Anewcontrol.begin() + newabsdiag;

					#if defined(LIBMAUS2_LCS_NNP_DEBUG)
					for ( int64_t i = -oldabsdiag; i <= oldabsdiag; ++i )
						assert ( newcontrol[i] == control[i] );
					#endif

					Acontrol = Anewcontrol;
					control = newcontrol;
				}

				#if 0
				for ( std::map<int,MidDiagElement>::const_iterator ita = M.begin(); ita != M.end(); ++ita )
					assert (
						control[ita->first] == ita->second
					);
				#endif
			}

			static int64_t getDefaultMinDiag()
			{
				return std::numeric_limits<int64_t>::min();
			}

			static int64_t getDefaultMaxDiag()
			{
				return std::numeric_limits<int64_t>::max();
			}


			template<typename iterator>
			std::pair<uint64_t,uint64_t> alignForward(
				iterator ab,
				iterator ae,
				iterator bb,
				iterator be,
				NNPTraceContainer & tracecontainer,
				bool const self = false,
				bool const uniquetermval = false,
				int64_t const minband = getDefaultMinDiag(),
				int64_t const maxband = getDefaultMaxDiag()
			)
			{
				tracecontainer.reset();

				if ( self )
				{
					static bool const t_self = true;

					if ( uniquetermval )
					{
						static bool const t_uniquetermval = true;

						alignTemplate<iterator,true /* forward */,t_self,t_uniquetermval>(
							ab,ae,bb,be,tracecontainer,
							minband,maxband
						);
					}
					else
					{
						static bool const t_uniquetermval = false;

						alignTemplate<iterator,true /* forward */,t_self,t_uniquetermval>(
							ab,ae,bb,be,tracecontainer,
							minband,maxband
						);
					}
				}
				else
				{
					static bool const t_self = false;

					if ( uniquetermval )
					{
						static bool const t_uniquetermval = true;

						alignTemplate<iterator,true /* forward */,t_self,t_uniquetermval>(
							ab,ae,bb,be,tracecontainer,
							minband,maxband
						);
					}
					else
					{
						static bool const t_uniquetermval = false;

						alignTemplate<iterator,true /* forward */,t_self,t_uniquetermval>(
							ab,ae,bb,be,tracecontainer,
							minband,maxband
						);
					}
				}

				if ( runsuffixpositive )
					tracecontainer.suffixPositive(1,1,1,1);

				std::pair<uint64_t,uint64_t> const SL = tracecontainer.getStringLengthUsed();

				return SL;
			}

			template<typename iterator>
			std::pair<uint64_t,uint64_t> alignForward(
				iterator ab,
				iterator ae,
				iterator bb,
				iterator be,
				NNPTraceContainer & tracecontainer,
				libmaus2::lcs::AlignmentTraceContainer & ATC,
				bool const self = false,
				bool const uniquetermval = false,
				int64_t const minband = getDefaultMinDiag(),
				int64_t const maxband = getDefaultMaxDiag()
			)
			{
				std::pair<uint64_t,uint64_t> const SL = alignForward(ab,ae,bb,be,tracecontainer,self,uniquetermval,minband,maxband);
				tracecontainer.computeTrace(ATC);
				return SL;
			}

			template<typename iterator, bool forward, bool self, bool uniquetermval>
			void alignTemplate(
				iterator ab,
				iterator ae,
				iterator bb,
				iterator be,
				NNPTraceContainer & tracecontainer,
				int64_t const minband = getDefaultMinDiag(),
				int64_t const maxband = getDefaultMaxDiag()
			)
			{
				uint64_t oactivediag = 0;

				ptrdiff_t const alen = ae-ab;
				ptrdiff_t const blen = be-bb;
				ptrdiff_t const mlen = std::min(alen,blen);
				bool const startdiagonalvalid = (minband <= 0 && maxband >= 0);

				int64_t maxscore = -1;
				int64_t maxscoretraceid = -1;

				uint64_t otrace = tracecontainer.otrace;

				tracecontainer.traceid = -1;

				libmaus2::autoarray::AutoArray<NNPTraceElement,libmaus2::autoarray::alloc_type_c> & Atrace = tracecontainer.Atrace;

				bool active = false;
				bool foundalignment = false;

				if ( startdiagonalvalid )
				{
					active = true;

					allocate(0,0,Acontrol,control);
					control[0].offset = forward ? slide<iterator,uniquetermval>(ab,ae,bb,be) : slider<iterator,uniquetermval>(ab,ae,bb,be);
					control[0].score = control[0].offset;
					control[0].evec = 0;
					control[0].id = otrace;
					Atrace.push(otrace,NNPTraceElement(libmaus2::lcs::BaseConstants::STEP_RESET,control[0].offset,-1));

					foundalignment = (alen==blen) && (control[0].offset==alen);

					if ( foundalignment )
					{
						tracecontainer.traceid = control[0].id;
					}
					else
					{
						Aactivediag.ensureSize(1);
						Aactivediag[oactivediag++] = 0;
					}
				}

				while ( !foundalignment && active )
				{
					active = false;

					// allocate control for next round
					// assert ( oactivediag );
					allocate(
						Aactivediag[0]-1,
						Aactivediag[oactivediag-1]+1,Ancontrol,ncontrol);

					#if 0
					std::set<int64_t> Sactive(
						Aactivediag.begin(),
						Aactivediag.begin()+oactivediag
					);

					for ( uint64_t i = 1; i < oactivediag; ++i )
						assert ( Aactivediag[i-1] < Aactivediag[i] );
					#endif

					uint64_t compdiago = 0;
					Acompdiag.ensureSize(oactivediag*3);

					for ( uint64_t i = 0; i < oactivediag; ++i )
					{
						int64_t const d = Aactivediag[i];
						int64_t const prevd = d-1;
						int64_t const nextd = d+1;

						bool const prevactive = (i   > 0          ) && (Aactivediag[i-1] == prevd);
						bool const nextactive = (i+1 < oactivediag) && (Aactivediag[i+1] == nextd);

						// std::cerr << "d=" << d << " prevd=" << prevd << " nextd=" << nextd << std::endl;

						#if 0
						assert ( Sactive.find(d) != Sactive.end() );
						if ( prevactive )
							assert ( Sactive.find(prevd) != Sactive.end() );
						else
							assert ( Sactive.find(prevd) == Sactive.end() );
						if ( nextactive )
							assert ( Sactive.find(nextd) != Sactive.end() );
						else
							assert ( Sactive.find(nextd) == Sactive.end() );
						#endif

						if (
							! prevactive
							&&
							(!compdiago || Acompdiag[compdiago-1].d != Aactivediag[i]-1)
						)
						{
							bool const prevprevactive = i && (Aactivediag[i-1] == Aactivediag[i]-2);
							int64_t canddiag = Aactivediag[i]-1;
							if ( canddiag >= minband && canddiag <= maxband )
								Acompdiag[compdiago++] = CompInfo(canddiag,prevprevactive,false /* this active */,true);
						}

						Acompdiag[compdiago++] = CompInfo(Aactivediag[i],prevactive,true /* this active */,nextactive);

						if ( ! nextactive )
						{
							bool const nextnextactive = i+1 < oactivediag && (Aactivediag[i+1] == Aactivediag[i]+2);
							int64_t const canddiag = Aactivediag[i]+1;
							if ( canddiag >= minband && canddiag <= maxband )
								Acompdiag[compdiago++] = CompInfo(canddiag,true,false /* this active */,nextnextactive);
						}
					}

					int64_t maxroundoffset = -1;

					// make space
					Atrace.ensureSize(otrace + compdiago);

					// iterate over diagonals for next round
					for ( uint64_t di = 0; di < compdiago; ++di )
					{
						int64_t const d = Acompdiag[di].d;

						// assert ( d >= minband && d <= maxband );

						bool const prevactive = Acompdiag[di].prevactive;
						bool const nextactive = Acompdiag[di].nextactive;
						bool const thisactive = Acompdiag[di].thisactive;

						#if 0
						if ( prevactive )
							assert ( Sactive.find(d-1) != Sactive.end() );
						else
							assert ( Sactive.find(d-1) == Sactive.end() );
						if ( nextactive )
							assert ( Sactive.find(d+1) != Sactive.end() );
						else
							assert ( Sactive.find(d+1) == Sactive.end() );
						if ( thisactive )
							assert ( Sactive.find(d) != Sactive.end() );
						else
							assert ( Sactive.find(d) == Sactive.end() );
						#endif

						// offsets on a and b due to diagonal
						int64_t const apre = (d >= 0) ? d : 0;
						int64_t const bpre = (d >= 0) ? 0 : -d;
						//assert ( apre >= 0 );
						//assert ( bpre >= 0 );
						// maximum offset on this diagonal
						int64_t const maxoffset = std::min(alen-apre,blen-bpre);

						if ( self )
						{
							iterator acheck = forward ? (ab + apre) : (ae-apre);
							iterator bcheck = forward ? (bb + bpre) : (be-bpre);

							if ( acheck == bcheck )
							{
								std::cerr << "diag=" << d << " forw=" << forward << std::endl;
								assert ( acheck != bcheck );
							}
						}

						/*
						 * find offset we will try to slide from
						 */

						/*
						 * if we can try from top / next higher diagonal
						 */
						int64_t offtop = -1;
						if ( nextactive && (control[d+1].offset >= 0) )
						{
							if ( d >= 0 )
							{
								offtop = control[d+1].offset + 1;
							}
							else
							{
								offtop = control[d+1].offset;
							}
							if ( offtop > maxoffset )
								offtop = -1;
						}
						/*
						 * if we can try from left / next lower diagonal
						 */
						int64_t offleft = -1;
						if ( prevactive && (control[d-1].offset >= 0) )
						{
							if ( d > 0 )
							{
								offleft = control[d-1].offset;
							}
							else
							{
								offleft = control[d-1].offset+1;
							}
							if ( offleft > maxoffset )
								offleft = -1;
						}
						/*
						 * if we can start from this diagonal
						 */
						int64_t offdiag = -1;
						if ( thisactive && (control[d].offset >= 0) )
						{
							offdiag = control[d].offset + 1; // mismatch
							if ( offdiag > maxoffset )
								offdiag = -1;
						}

						#if 0
						if ( thisactive )
						{
							std::cerr << "diagonal " << d << " offset " << control[d].offset << " werr=" << popcnt(control[d].evec) << std::endl;
						}
						else
						{
							std::cerr << "new diagonal " << d << std::endl;
						}

						std::cerr << "offleft=" << offleft << " offdiag=" << offdiag << " offtop=" << offtop << std::endl;
						#endif

						// mark as invalid
						ncontrol[d].offset = -1;

						if ( offtop >= offleft )
						{
							if ( offdiag >= offtop )
							{
								if ( offdiag >= 0 && (offdiag > control[d].offset) )
								{
									int64_t const aoff = apre + offdiag;
									int64_t const boff = bpre + offdiag;
									int64_t const slidelen = forward ? slide<iterator,uniquetermval>(ab+aoff,ae,bb+boff,be) : slider<iterator,uniquetermval>(ab,ae-aoff,bb,be-boff);
									uint64_t const newevec = (slidelen < 64) ? (((control[d].evec << 1) | 1) << slidelen) : 0;
									unsigned int const numerr = popcnt(newevec);

									if ( numerr <= maxwerr )
									{
										ncontrol[d].offset = offdiag + slidelen;
										ncontrol[d].score = control[d].score - 1 + slidelen;
										ncontrol[d].id = otrace;
										ncontrol[d].evec = newevec;

										if ( ncontrol[d].score > maxscore )
										{
											maxscore = ncontrol[d].score;
											maxscoretraceid = ncontrol[d].id;
										}

										//assert ( otrace < Atrace.size() );
										Atrace[otrace++] = NNPTraceElement(libmaus2::lcs::BaseConstants::STEP_MISMATCH,slidelen,control[d].id);
									}
								}
							}
							else // offtop maximal
							{
								//assert ( offtop >= 0 );

								if ( (!thisactive) || (offtop > control[d].offset) )
								{
									int64_t const aoff = apre + offtop;
									int64_t const boff = bpre + offtop;

									int64_t const slidelen = forward ? slide<iterator,uniquetermval>(ab+aoff,ae,bb+boff,be) : slider<iterator,uniquetermval>(ab,ae-aoff,bb,be-boff);
									uint64_t const newevec = (slidelen < 64) ? (((control[d+1].evec << 1) | 1) << slidelen) : 0;
									unsigned int const numerr = popcnt(newevec);

									if ( numerr <= maxwerr )
									{
										ncontrol[d].offset = offtop + slidelen;
										ncontrol[d].id = otrace;
										ncontrol[d].score = control[d+1].score - 1 + slidelen;
										ncontrol[d].evec = newevec;

										if ( ncontrol[d].score > maxscore )
										{
											maxscore = ncontrol[d].score;
											maxscoretraceid = ncontrol[d].id;
										}

										//assert ( otrace < Atrace.size() );
										Atrace[otrace++] = NNPTraceElement(libmaus2::lcs::BaseConstants::STEP_INS,slidelen,control[d+1].id);
									}
								}
							}
						}
						else // offtop < offleft
						{
							if ( offdiag >= offleft )
							{
								if ( offdiag >= 0 && (offdiag > control[d].offset) )
								{
									int64_t const aoff = apre + offdiag;
									int64_t const boff = bpre + offdiag;
									int64_t const slidelen = forward ? slide<iterator,uniquetermval>(ab+aoff,ae,bb+boff,be) : slider<iterator,uniquetermval>(ab,ae-aoff,bb,be-boff);
									uint64_t const newevec = (slidelen < 64) ? (((control[d].evec << 1) | 1) << slidelen) : 0;
									unsigned int const numerr = popcnt(newevec);

									if ( numerr <= maxwerr )
									{
										ncontrol[d].offset = offdiag + slidelen;
										ncontrol[d].id = otrace;
										ncontrol[d].score = control[d].score - 1 + slidelen;
										ncontrol[d].evec = newevec;

										if ( ncontrol[d].score > maxscore )
										{
											maxscore = ncontrol[d].score;
											maxscoretraceid = ncontrol[d].id;
										}

										//assert ( otrace < Atrace.size() );
										Atrace[otrace++] = NNPTraceElement(libmaus2::lcs::BaseConstants::STEP_MISMATCH,slidelen,control[d].id);
									}
								}
							}
							else // offleft maximal
							{
								if ( (!thisactive) || (offleft > control[d].offset) )
								{
									//assert ( offleft >= 0 );
									int64_t const aoff = apre + offleft;
									int64_t const boff = bpre + offleft;
									int64_t const slidelen = forward ? slide<iterator,uniquetermval>(ab+aoff,ae,bb+boff,be) : slider<iterator,uniquetermval>(ab,ae-aoff,bb,be-boff);
									uint64_t const newevec = (slidelen < 64) ? (((control[d-1].evec << 1) | 1) << slidelen) : 0;
									unsigned int const numerr = popcnt(newevec);

									if ( numerr <= maxwerr )
									{
										ncontrol[d].offset = offleft + slidelen;
										ncontrol[d].id = otrace;
										ncontrol[d].score = control[d-1].score - 1 + slidelen;
										ncontrol[d].evec = newevec;

										if ( ncontrol[d].score > maxscore )
										{
											maxscore = ncontrol[d].score;
											maxscoretraceid = ncontrol[d].id;
										}

										//assert ( otrace < Atrace.size() );
										Atrace[otrace++] = NNPTraceElement(libmaus2::lcs::BaseConstants::STEP_DEL,slidelen,control[d-1].id);
									}
								}
							}
						}

						// have we found a full alignment of the two strings?
						if ( d == (alen-blen) && ncontrol[d].offset == mlen )
						{
							#if 0
							std::cerr << "yes " << d << ",offset="
								<< ncontrol[d].offset << ",score=" << ncontrol[d].score
								<< ",alen=" << alen << ",blen=" << blen << ",mlen=" << mlen
								<< std::endl;
							#endif

							foundalignment = true;
							tracecontainer.traceid = otrace-1;
						}

						if ( ncontrol[d].offset > maxroundoffset )
							maxroundoffset = ncontrol[d].offset;

					}

					Acontrol.swap(Ancontrol);
					std::swap(control,ncontrol);

					// filter active diagonals
					Aactivediag.ensureSize(compdiago);
					oactivediag = 0;
					for ( uint64_t di = 0; di < compdiago; ++di )
					{
						int64_t const d = Acompdiag[di].d;
						if ( control[d].offset >= 0 && control[d].offset >= maxroundoffset-maxback )
						{
							Aactivediag[oactivediag++] = d;
							active = true;
						}
					}
				}

				if ( ! foundalignment )
				{
					// std::cerr << "no full alignment found, cutting back to best score" << std::endl;
					tracecontainer.traceid = maxscoretraceid;
				}

				#if 0
				std::cerr << "maxscore=" << maxscore << std::endl;
				std::cerr << "maxscoretraceid=" << maxscoretraceid << std::endl;
				std::cerr << "traceid=" << traceid << std::endl;
				#endif

				#if 0
				for ( int64_t curtraceid = traceid ; curtraceid >= 0; curtraceid = Atrace[curtraceid].parent )
				{
					std::cerr << "traceid=" << curtraceid << " op " << Atrace[curtraceid].step << " slide " << Atrace[curtraceid].slide << std::endl;
				}
				#endif

				tracecontainer.otrace = otrace;

				#if 0
				if ( runsuffixpositive )
					tracecontainer.suffixPositive(1,1,1,1);
				#endif

				if ( ! forward )
					tracecontainer.reverse();
			}

			template<typename iterator, bool forward, bool self, bool uniquetermval>
			std::pair<int32_t,int32_t> alignTemplateMid(
				iterator ab,
				iterator ae,
				iterator bb,
				iterator be,
				int64_t const minband = getDefaultMinDiag(),
				int64_t const maxband = getDefaultMaxDiag()
			)
			{
				uint64_t oactivediag = 0;

				ptrdiff_t const alen = ae-ab;
				ptrdiff_t const blen = be-bb;
				ptrdiff_t const mlen = std::min(alen,blen);
				ptrdiff_t const mlen2 = (mlen+1)/2;
				bool const startdiagonalvalid = (minband <= 0 && maxband >= 0);

				int64_t maxscore       = std::numeric_limits<int64_t>::min();
				int64_t maxscorediag   = std::numeric_limits<int64_t>::min();
				//int64_t maxscoreoffset = std::numeric_limits<int64_t>::min();

				bool active = false;
				bool foundalignment = false;

				std::pair<int32_t,int32_t> const Punset(
					std::numeric_limits<int32_t>::min(),
					std::numeric_limits<int32_t>::min()
				);

				if ( startdiagonalvalid )
				{
					active = true;

					allocate(0,0,Amidcontrol,midcontrol);
					midcontrol[0].offset = forward ? slide<iterator,uniquetermval>(ab,ae,bb,be) : slider<iterator,uniquetermval>(ab,ae,bb,be);
					midcontrol[0].score = midcontrol[0].offset;
					midcontrol[0].evec = 0;

					if ( midcontrol[0].offset >= mlen2 )
						midcontrol[0].mid = std::pair<int32_t,int32_t>(0,midcontrol[0].offset);
					else
						midcontrol[0].mid = Punset;

					foundalignment = (alen==blen) && (midcontrol[0].offset==alen);

					if ( foundalignment )
					{
						maxscore = midcontrol[0].score;
						maxscorediag = 0;
						//maxscoreoffset = midcontrol[0].offset;
					}
					else
					{
						Aactivediag.ensureSize(1);
						Aactivediag[oactivediag++] = 0;
					}
				}

				while ( !foundalignment && active )
				{
					active = false;

					// allocate midcontrol for next round
					// assert ( oactivediag );
					allocate(
						Aactivediag[0]-1,
						Aactivediag[oactivediag-1]+1,Anmidcontrol,nmidcontrol);

					#if 0
					std::set<int64_t> Sactive(
						Aactivediag.begin(),
						Aactivediag.begin()+oactivediag
					);

					for ( uint64_t i = 1; i < oactivediag; ++i )
						assert ( Aactivediag[i-1] < Aactivediag[i] );
					#endif

					uint64_t compdiago = 0;
					Acompdiag.ensureSize(oactivediag*3);

					for ( uint64_t i = 0; i < oactivediag; ++i )
					{
						int64_t const d = Aactivediag[i];
						int64_t const prevd = d-1;
						int64_t const nextd = d+1;

						bool const prevactive = (i   > 0          ) && (Aactivediag[i-1] == prevd);
						bool const nextactive = (i+1 < oactivediag) && (Aactivediag[i+1] == nextd);

						// std::cerr << "d=" << d << " prevd=" << prevd << " nextd=" << nextd << std::endl;

						#if 0
						assert ( Sactive.find(d) != Sactive.end() );
						if ( prevactive )
							assert ( Sactive.find(prevd) != Sactive.end() );
						else
							assert ( Sactive.find(prevd) == Sactive.end() );
						if ( nextactive )
							assert ( Sactive.find(nextd) != Sactive.end() );
						else
							assert ( Sactive.find(nextd) == Sactive.end() );
						#endif

						if (
							! prevactive
							&&
							(!compdiago || Acompdiag[compdiago-1].d != Aactivediag[i]-1)
						)
						{
							bool const prevprevactive = i && (Aactivediag[i-1] == Aactivediag[i]-2);
							int64_t canddiag = Aactivediag[i]-1;
							if ( canddiag >= minband && canddiag <= maxband )
								Acompdiag[compdiago++] = CompInfo(canddiag,prevprevactive,false /* this active */,true);
						}

						Acompdiag[compdiago++] = CompInfo(Aactivediag[i],prevactive,true /* this active */,nextactive);

						if ( ! nextactive )
						{
							bool const nextnextactive = i+1 < oactivediag && (Aactivediag[i+1] == Aactivediag[i]+2);
							int64_t const canddiag = Aactivediag[i]+1;
							if ( canddiag >= minband && canddiag <= maxband )
								Acompdiag[compdiago++] = CompInfo(canddiag,true,false /* this active */,nextnextactive);
						}
					}

					int64_t maxroundoffset = -1;

					// iterate over diagonals for next round
					for ( uint64_t di = 0; di < compdiago; ++di )
					{
						int64_t const d = Acompdiag[di].d;

						// assert ( d >= minband && d <= maxband );

						bool const prevactive = Acompdiag[di].prevactive;
						bool const nextactive = Acompdiag[di].nextactive;
						bool const thisactive = Acompdiag[di].thisactive;

						#if 0
						if ( prevactive )
							assert ( Sactive.find(d-1) != Sactive.end() );
						else
							assert ( Sactive.find(d-1) == Sactive.end() );
						if ( nextactive )
							assert ( Sactive.find(d+1) != Sactive.end() );
						else
							assert ( Sactive.find(d+1) == Sactive.end() );
						if ( thisactive )
							assert ( Sactive.find(d) != Sactive.end() );
						else
							assert ( Sactive.find(d) == Sactive.end() );
						#endif

						// offsets on a and b due to diagonal
						int64_t const apre = (d >= 0) ? d : 0;
						int64_t const bpre = (d >= 0) ? 0 : -d;
						//assert ( apre >= 0 );
						//assert ( bpre >= 0 );
						// maximum offset on this diagonal
						int64_t const maxoffset = std::min(alen-apre,blen-bpre);

						if ( self )
						{
							iterator acheck = forward ? (ab + apre) : (ae-apre);
							iterator bcheck = forward ? (bb + bpre) : (be-bpre);

							if ( acheck == bcheck )
							{
								std::cerr << "diag=" << d << " forw=" << forward << std::endl;
								assert ( acheck != bcheck );
							}
						}

						/*
						 * find offset we will try to slide from
						 */

						/*
						 * if we can try from top / next higher diagonal
						 */
						int64_t offtop = -1;
						if ( nextactive && (midcontrol[d+1].offset >= 0) )
						{
							if ( d >= 0 )
							{
								offtop = midcontrol[d+1].offset + 1;
							}
							else
							{
								offtop = midcontrol[d+1].offset;
							}
							if ( offtop > maxoffset )
								offtop = -1;
						}
						/*
						 * if we can try from left / next lower diagonal
						 */
						int64_t offleft = -1;
						if ( prevactive && (midcontrol[d-1].offset >= 0) )
						{
							if ( d > 0 )
							{
								offleft = midcontrol[d-1].offset;
							}
							else
							{
								offleft = midcontrol[d-1].offset+1;
							}
							if ( offleft > maxoffset )
								offleft = -1;
						}
						/*
						 * if we can start from this diagonal
						 */
						int64_t offdiag = -1;
						if ( thisactive && (midcontrol[d].offset >= 0) )
						{
							offdiag = midcontrol[d].offset + 1; // mismatch
							if ( offdiag > maxoffset )
								offdiag = -1;
						}

						#if 0
						if ( thisactive )
						{
							std::cerr << "diagonal " << d << " offset " << midcontrol[d].offset << " werr=" << popcnt(midcontrol[d].evec) << std::endl;
						}
						else
						{
							std::cerr << "new diagonal " << d << std::endl;
						}

						std::cerr << "offleft=" << offleft << " offdiag=" << offdiag << " offtop=" << offtop << std::endl;
						#endif

						// mark as invalid
						nmidcontrol[d].offset = -1;

						if ( offtop >= offleft )
						{
							if ( offdiag >= offtop )
							{
								if ( offdiag >= 0 && (offdiag > midcontrol[d].offset) )
								{
									int64_t const aoff = apre + offdiag;
									int64_t const boff = bpre + offdiag;
									int64_t const slidelen = forward ? slide<iterator,uniquetermval>(ab+aoff,ae,bb+boff,be) : slider<iterator,uniquetermval>(ab,ae-aoff,bb,be-boff);
									uint64_t const newevec = (slidelen < 64) ? (((midcontrol[d].evec << 1) | 1) << slidelen) : 0;
									unsigned int const numerr = popcnt(newevec);

									if ( numerr <= maxwerr )
									{
										nmidcontrol[d].offset = offdiag + slidelen;
										nmidcontrol[d].score = midcontrol[d].score - 1 + slidelen;
										nmidcontrol[d].evec = newevec;

										if ( midcontrol[d].mid != Punset )
											nmidcontrol[d].mid = midcontrol[d].mid;
										else if ( nmidcontrol[d].offset >= mlen2 )
											nmidcontrol[d].mid = std::pair<int32_t,int32_t>(d,nmidcontrol[d].offset);
										else
											nmidcontrol[d].mid = Punset;

										if ( nmidcontrol[d].score > maxscore )
										{
											maxscore = nmidcontrol[d].score;
											maxscorediag = d;
											//maxscoreoffset = nmidcontrol[d].offset;
										}
									}
								}
							}
							else // offtop maximal
							{
								//assert ( offtop >= 0 );

								if ( (!thisactive) || (offtop > midcontrol[d].offset) )
								{
									int64_t const aoff = apre + offtop;
									int64_t const boff = bpre + offtop;

									int64_t const slidelen = forward ? slide<iterator,uniquetermval>(ab+aoff,ae,bb+boff,be) : slider<iterator,uniquetermval>(ab,ae-aoff,bb,be-boff);
									uint64_t const newevec = (slidelen < 64) ? (((midcontrol[d+1].evec << 1) | 1) << slidelen) : 0;
									unsigned int const numerr = popcnt(newevec);

									if ( numerr <= maxwerr )
									{
										nmidcontrol[d].offset = offtop + slidelen;
										nmidcontrol[d].score = midcontrol[d+1].score - 1 + slidelen;
										nmidcontrol[d].evec = newevec;

										if ( midcontrol[d+1].mid != Punset )
											nmidcontrol[d].mid = midcontrol[d+1].mid;
										else if ( nmidcontrol[d].offset >= mlen2 )
											nmidcontrol[d].mid = std::pair<int32_t,int32_t>(d,nmidcontrol[d].offset);
										else
											nmidcontrol[d].mid = Punset;

										if ( nmidcontrol[d].score > maxscore )
										{
											maxscore = nmidcontrol[d].score;
											maxscorediag = d;
											//maxscoreoffset = nmidcontrol[d].offset;
										}
									}
								}
							}
						}
						else // offtop < offleft
						{
							if ( offdiag >= offleft )
							{
								if ( offdiag >= 0 && (offdiag > midcontrol[d].offset) )
								{
									int64_t const aoff = apre + offdiag;
									int64_t const boff = bpre + offdiag;
									int64_t const slidelen = forward ? slide<iterator,uniquetermval>(ab+aoff,ae,bb+boff,be) : slider<iterator,uniquetermval>(ab,ae-aoff,bb,be-boff);
									uint64_t const newevec = (slidelen < 64) ? (((midcontrol[d].evec << 1) | 1) << slidelen) : 0;
									unsigned int const numerr = popcnt(newevec);

									if ( numerr <= maxwerr )
									{
										nmidcontrol[d].offset = offdiag + slidelen;
										nmidcontrol[d].score = midcontrol[d].score - 1 + slidelen;
										nmidcontrol[d].evec = newevec;

										if ( midcontrol[d].mid != Punset )
											nmidcontrol[d].mid = midcontrol[d].mid;
										else if ( nmidcontrol[d].offset >= mlen2 )
											nmidcontrol[d].mid = std::pair<int32_t,int32_t>(d,nmidcontrol[d].offset);
										else
											nmidcontrol[d].mid = Punset;

										if ( nmidcontrol[d].score > maxscore )
										{
											maxscore = nmidcontrol[d].score;
											maxscorediag = d;
											//maxscoreoffset = nmidcontrol[d].offset;
										}
									}
								}
							}
							else // offleft maximal
							{
								if ( (!thisactive) || (offleft > midcontrol[d].offset) )
								{
									//assert ( offleft >= 0 );
									int64_t const aoff = apre + offleft;
									int64_t const boff = bpre + offleft;
									int64_t const slidelen = forward ? slide<iterator,uniquetermval>(ab+aoff,ae,bb+boff,be) : slider<iterator,uniquetermval>(ab,ae-aoff,bb,be-boff);
									uint64_t const newevec = (slidelen < 64) ? (((midcontrol[d-1].evec << 1) | 1) << slidelen) : 0;
									unsigned int const numerr = popcnt(newevec);

									if ( numerr <= maxwerr )
									{
										nmidcontrol[d].offset = offleft + slidelen;
										nmidcontrol[d].score = midcontrol[d-1].score - 1 + slidelen;
										nmidcontrol[d].evec = newevec;

										if ( midcontrol[d-1].mid != Punset )
											nmidcontrol[d].mid = midcontrol[d-1].mid;
										else if ( nmidcontrol[d].offset >= mlen2 )
											nmidcontrol[d].mid = std::pair<int32_t,int32_t>(d,nmidcontrol[d].offset);
										else
											nmidcontrol[d].mid = Punset;


										if ( nmidcontrol[d].score > maxscore )
										{
											maxscore = nmidcontrol[d].score;
											maxscorediag = d;
											//maxscoreoffset = nmidcontrol[d].offset;
										}
									}
								}
							}
						}

						// have we found a full alignment of the two strings?
						if ( d == (alen-blen) && nmidcontrol[d].offset == mlen )
						{
							#if 0
							std::cerr << "yes " << d << ",offset="
								<< nmidcontrol[d].offset << ",score=" << nmidcontrol[d].score
								<< ",alen=" << alen << ",blen=" << blen << ",mlen=" << mlen
								<< std::endl;
							#endif

							foundalignment = true;

							maxscore = nmidcontrol[d].score;
							maxscorediag = d;
							//maxscoreoffset = nmidcontrol[d].offset;
						}

						if ( nmidcontrol[d].offset > maxroundoffset )
							maxroundoffset = nmidcontrol[d].offset;

					}

					Amidcontrol.swap(Anmidcontrol);
					std::swap(midcontrol,nmidcontrol);

					// filter active diagonals
					Aactivediag.ensureSize(compdiago);
					oactivediag = 0;
					for ( uint64_t di = 0; di < compdiago; ++di )
					{
						int64_t const d = Acompdiag[di].d;
						if ( midcontrol[d].offset >= 0 && midcontrol[d].offset >= maxroundoffset-maxback )
						{
							Aactivediag[oactivediag++] = d;
							active = true;
						}
					}
				}

				if ( foundalignment )
				{
					std::pair<int32_t,int32_t> const P = midcontrol[alen-blen].mid;

					if ( P == Punset )
					{
						return std::pair<int32_t,int32_t>(0,0);
					}
					else
					{
						int64_t const d = P.first;
						int64_t const off = P.second;
						int64_t const apre = (d >= 0) ? d : 0;
						int64_t const bpre = (d >= 0) ? 0 : -d;
						return std::pair<int32_t,int32_t>(apre + off,bpre+off);
					}
				}
				else
				{
					if ( maxscore != std::numeric_limits<int64_t>::min() )
					{
						std::pair<int32_t,int32_t> const P = midcontrol[maxscorediag].mid;

						if ( P == Punset )
							return std::pair<int32_t,int32_t>(0,0);
						else
						{
							int64_t const d = P.first;
							int64_t const off = P.second;
							int64_t const apre = (d >= 0) ? d : 0;
							int64_t const bpre = (d >= 0) ? 0 : -d;
							return std::pair<int32_t,int32_t>(apre + off,bpre+off);
						}
					}
					else
					{
						return std::pair<int32_t,int32_t>(0,0);
					}
				}
			}

			template<typename iterator, bool forward, bool self, bool uniquetermval>
			std::pair<uint64_t,uint64_t> alignTemplateNoTrace(
				iterator ab,
				iterator ae,
				iterator bb,
				iterator be,
				int64_t const minband = getDefaultMinDiag(),
				int64_t const maxband = getDefaultMaxDiag()
			)
			{
				uint64_t oactivediag = 0;

				ptrdiff_t const alen = ae-ab;
				ptrdiff_t const blen = be-bb;
				ptrdiff_t const mlen = std::min(alen,blen);
				bool const startdiagonalvalid = (minband <= 0 && maxband >= 0);

				int64_t maxscore = std::numeric_limits<int64_t>::min();
				int64_t maxscorediagonal = std::numeric_limits<int64_t>::min();
				int64_t maxscoreoffset = std::numeric_limits<int64_t>::min();

				bool active = false;
				bool foundalignment = false;

				if ( startdiagonalvalid )
				{
					active = true;

					allocate(0,0,Acontrol,control);
					control[0].offset = forward ? slide<iterator,uniquetermval>(ab,ae,bb,be) : slider<iterator,uniquetermval>(ab,ae,bb,be);
					control[0].score = control[0].offset;
					control[0].evec = 0;

					foundalignment = (alen==blen) && (control[0].offset==alen);

					if ( foundalignment )
					{

					}
					else
					{
						Aactivediag.ensureSize(1);
						Aactivediag[oactivediag++] = 0;
					}
				}

				while ( !foundalignment && active )
				{
					active = false;

					// allocate control for next round
					// assert ( oactivediag );
					allocate(
						Aactivediag[0]-1,
						Aactivediag[oactivediag-1]+1,Ancontrol,ncontrol);

					#if 0
					std::set<int64_t> Sactive(
						Aactivediag.begin(),
						Aactivediag.begin()+oactivediag
					);

					for ( uint64_t i = 1; i < oactivediag; ++i )
						assert ( Aactivediag[i-1] < Aactivediag[i] );
					#endif

					uint64_t compdiago = 0;
					Acompdiag.ensureSize(oactivediag*3);

					for ( uint64_t i = 0; i < oactivediag; ++i )
					{
						int64_t const d = Aactivediag[i];
						int64_t const prevd = d-1;
						int64_t const nextd = d+1;

						bool const prevactive = (i   > 0          ) && (Aactivediag[i-1] == prevd);
						bool const nextactive = (i+1 < oactivediag) && (Aactivediag[i+1] == nextd);

						// std::cerr << "d=" << d << " prevd=" << prevd << " nextd=" << nextd << std::endl;

						#if 0
						assert ( Sactive.find(d) != Sactive.end() );
						if ( prevactive )
							assert ( Sactive.find(prevd) != Sactive.end() );
						else
							assert ( Sactive.find(prevd) == Sactive.end() );
						if ( nextactive )
							assert ( Sactive.find(nextd) != Sactive.end() );
						else
							assert ( Sactive.find(nextd) == Sactive.end() );
						#endif

						if (
							! prevactive
							&&
							(!compdiago || Acompdiag[compdiago-1].d != Aactivediag[i]-1)
						)
						{
							bool const prevprevactive = i && (Aactivediag[i-1] == Aactivediag[i]-2);
							int64_t canddiag = Aactivediag[i]-1;
							if ( canddiag >= minband && canddiag <= maxband )
								Acompdiag[compdiago++] = CompInfo(canddiag,prevprevactive,false /* this active */,true);
						}

						Acompdiag[compdiago++] = CompInfo(Aactivediag[i],prevactive,true /* this active */,nextactive);

						if ( ! nextactive )
						{
							bool const nextnextactive = i+1 < oactivediag && (Aactivediag[i+1] == Aactivediag[i]+2);
							int64_t const canddiag = Aactivediag[i]+1;
							if ( canddiag >= minband && canddiag <= maxband )
								Acompdiag[compdiago++] = CompInfo(canddiag,true,false /* this active */,nextnextactive);
						}
					}

					int64_t maxroundoffset = -1;

					// iterate over diagonals for next round
					for ( uint64_t di = 0; di < compdiago; ++di )
					{
						int64_t const d = Acompdiag[di].d;

						// assert ( d >= minband && d <= maxband );

						bool const prevactive = Acompdiag[di].prevactive;
						bool const nextactive = Acompdiag[di].nextactive;
						bool const thisactive = Acompdiag[di].thisactive;

						#if 0
						if ( prevactive )
							assert ( Sactive.find(d-1) != Sactive.end() );
						else
							assert ( Sactive.find(d-1) == Sactive.end() );
						if ( nextactive )
							assert ( Sactive.find(d+1) != Sactive.end() );
						else
							assert ( Sactive.find(d+1) == Sactive.end() );
						if ( thisactive )
							assert ( Sactive.find(d) != Sactive.end() );
						else
							assert ( Sactive.find(d) == Sactive.end() );
						#endif

						// offsets on a and b due to diagonal
						int64_t const apre = (d >= 0) ? d : 0;
						int64_t const bpre = (d >= 0) ? 0 : -d;
						//assert ( apre >= 0 );
						//assert ( bpre >= 0 );
						// maximum offset on this diagonal
						int64_t const maxoffset = std::min(alen-apre,blen-bpre);

						if ( self )
						{
							iterator acheck = forward ? (ab + apre) : (ae-apre);
							iterator bcheck = forward ? (bb + bpre) : (be-bpre);

							if ( acheck == bcheck )
							{
								std::cerr << "diag=" << d << " forw=" << forward << std::endl;
								assert ( acheck != bcheck );
							}
						}

						/*
						 * find offset we will try to slide from
						 */

						/*
						 * if we can try from top / next higher diagonal
						 */
						int64_t offtop = -1;
						if ( nextactive && (control[d+1].offset >= 0) )
						{
							if ( d >= 0 )
							{
								offtop = control[d+1].offset + 1;
							}
							else
							{
								offtop = control[d+1].offset;
							}
							if ( offtop > maxoffset )
								offtop = -1;
						}
						/*
						 * if we can try from left / next lower diagonal
						 */
						int64_t offleft = -1;
						if ( prevactive && (control[d-1].offset >= 0) )
						{
							if ( d > 0 )
							{
								offleft = control[d-1].offset;
							}
							else
							{
								offleft = control[d-1].offset+1;
							}
							if ( offleft > maxoffset )
								offleft = -1;
						}
						/*
						 * if we can start from this diagonal
						 */
						int64_t offdiag = -1;
						if ( thisactive && (control[d].offset >= 0) )
						{
							offdiag = control[d].offset + 1; // mismatch
							if ( offdiag > maxoffset )
								offdiag = -1;
						}

						#if 0
						if ( thisactive )
						{
							std::cerr << "diagonal " << d << " offset " << control[d].offset << " werr=" << popcnt(control[d].evec) << std::endl;
						}
						else
						{
							std::cerr << "new diagonal " << d << std::endl;
						}

						std::cerr << "offleft=" << offleft << " offdiag=" << offdiag << " offtop=" << offtop << std::endl;
						#endif

						// mark as invalid
						ncontrol[d].offset = -1;

						if ( offtop >= offleft )
						{
							if ( offdiag >= offtop )
							{
								if ( offdiag >= 0 && (offdiag > control[d].offset) )
								{
									int64_t const aoff = apre + offdiag;
									int64_t const boff = bpre + offdiag;
									int64_t const slidelen = forward ? slide<iterator,uniquetermval>(ab+aoff,ae,bb+boff,be) : slider<iterator,uniquetermval>(ab,ae-aoff,bb,be-boff);
									uint64_t const newevec = (slidelen < 64) ? (((control[d].evec << 1) | 1) << slidelen) : 0;
									unsigned int const numerr = popcnt(newevec);

									if ( numerr <= maxwerr )
									{
										ncontrol[d].offset = offdiag + slidelen;
										ncontrol[d].score = control[d].score - 1 + slidelen;
										ncontrol[d].evec = newevec;

										if ( ncontrol[d].score > maxscore )
										{
											maxscore = ncontrol[d].score;
											maxscorediagonal = d;
											maxscoreoffset = ncontrol[d].offset;
										}
									}
								}
							}
							else // offtop maximal
							{
								//assert ( offtop >= 0 );

								if ( (!thisactive) || (offtop > control[d].offset) )
								{
									int64_t const aoff = apre + offtop;
									int64_t const boff = bpre + offtop;

									int64_t const slidelen = forward ? slide<iterator,uniquetermval>(ab+aoff,ae,bb+boff,be) : slider<iterator,uniquetermval>(ab,ae-aoff,bb,be-boff);
									uint64_t const newevec = (slidelen < 64) ? (((control[d+1].evec << 1) | 1) << slidelen) : 0;
									unsigned int const numerr = popcnt(newevec);

									if ( numerr <= maxwerr )
									{
										ncontrol[d].offset = offtop + slidelen;
										ncontrol[d].score = control[d+1].score - 1 + slidelen;
										ncontrol[d].evec = newevec;

										if ( ncontrol[d].score > maxscore )
										{
											maxscore = ncontrol[d].score;
											maxscorediagonal = d;
											maxscoreoffset = ncontrol[d].offset;
										}
									}
								}
							}
						}
						else // offtop < offleft
						{
							if ( offdiag >= offleft )
							{
								if ( offdiag >= 0 && (offdiag > control[d].offset) )
								{
									int64_t const aoff = apre + offdiag;
									int64_t const boff = bpre + offdiag;
									int64_t const slidelen = forward ? slide<iterator,uniquetermval>(ab+aoff,ae,bb+boff,be) : slider<iterator,uniquetermval>(ab,ae-aoff,bb,be-boff);
									uint64_t const newevec = (slidelen < 64) ? (((control[d].evec << 1) | 1) << slidelen) : 0;
									unsigned int const numerr = popcnt(newevec);

									if ( numerr <= maxwerr )
									{
										ncontrol[d].offset = offdiag + slidelen;
										ncontrol[d].score = control[d].score - 1 + slidelen;
										ncontrol[d].evec = newevec;

										if ( ncontrol[d].score > maxscore )
										{
											maxscore = ncontrol[d].score;
											maxscorediagonal = d;
											maxscoreoffset = ncontrol[d].offset;
										}
									}
								}
							}
							else // offleft maximal
							{
								if ( (!thisactive) || (offleft > control[d].offset) )
								{
									//assert ( offleft >= 0 );
									int64_t const aoff = apre + offleft;
									int64_t const boff = bpre + offleft;
									int64_t const slidelen = forward ? slide<iterator,uniquetermval>(ab+aoff,ae,bb+boff,be) : slider<iterator,uniquetermval>(ab,ae-aoff,bb,be-boff);
									uint64_t const newevec = (slidelen < 64) ? (((control[d-1].evec << 1) | 1) << slidelen) : 0;
									unsigned int const numerr = popcnt(newevec);

									if ( numerr <= maxwerr )
									{
										ncontrol[d].offset = offleft + slidelen;
										ncontrol[d].score = control[d-1].score - 1 + slidelen;
										ncontrol[d].evec = newevec;

										if ( ncontrol[d].score > maxscore )
										{
											maxscore = ncontrol[d].score;
											maxscorediagonal = d;
											maxscoreoffset = ncontrol[d].offset;
										}
									}
								}
							}
						}

						// have we found a full alignment of the two strings?
						if ( d == (alen-blen) && ncontrol[d].offset == mlen )
						{
							#if 0
							std::cerr << "yes " << d << ",offset="
								<< ncontrol[d].offset << ",score=" << ncontrol[d].score
								<< ",alen=" << alen << ",blen=" << blen << ",mlen=" << mlen
								<< std::endl;
							#endif

							foundalignment = true;
						}

						if ( ncontrol[d].offset > maxroundoffset )
							maxroundoffset = ncontrol[d].offset;

					}

					Acontrol.swap(Ancontrol);
					std::swap(control,ncontrol);

					// filter active diagonals
					Aactivediag.ensureSize(compdiago);
					oactivediag = 0;
					for ( uint64_t di = 0; di < compdiago; ++di )
					{
						int64_t const d = Acompdiag[di].d;
						if ( control[d].offset >= 0 && control[d].offset >= maxroundoffset-maxback )
						{
							Aactivediag[oactivediag++] = d;
							active = true;
						}
					}
				}

				if ( foundalignment )
				{
					return std::pair<uint64_t,uint64_t>(ae-ab,be-bb);
				}
				else
				{
					if ( maxscore != std::numeric_limits<int64_t>::min() )
					{
						int64_t const d = maxscorediagonal;
						int64_t const off = maxscoreoffset;
						int64_t const apre = (d >= 0) ? d : 0;
						int64_t const bpre = (d >= 0) ? 0 : -d;
						int64_t const aoff = apre + off;
						int64_t const boff = bpre + off;
						assert ( aoff >= 0 );
						assert ( boff >= 0 );

						return std::pair<uint64_t,uint64_t>(aoff,boff);
					}
					else
					{
						return std::pair<uint64_t,uint64_t>(0,0);
					}
				}
			}

			template<typename iterator>
			void align(
				iterator ab,
				iterator ae,
				iterator bb,
				iterator be,
				NNPTraceContainer & tracecontainer,
				int64_t const minband = getDefaultMinDiag(),
				int64_t const maxband = getDefaultMaxDiag(),
				bool const forward = true,
				bool const self = false,
				bool const uniquetermval = false
			)
			{
				if ( uniquetermval )
				{
					if ( forward )
						if ( self )
							alignTemplate<iterator,true,true,true>(ab,ae,bb,be,tracecontainer,minband,maxband);
						else
							alignTemplate<iterator,true,false,true>(ab,ae,bb,be,tracecontainer,minband,maxband);
					else
						if ( self )
							alignTemplate<iterator,false,true,true>(ab,ae,bb,be,tracecontainer,minband,maxband);
						else
							alignTemplate<iterator,false,false,true>(ab,ae,bb,be,tracecontainer,minband,maxband);
				}
				else
				{
					if ( forward )
						if ( self )
							alignTemplate<iterator,true,true,false>(ab,ae,bb,be,tracecontainer,minband,maxband);
						else
							alignTemplate<iterator,true,false,false>(ab,ae,bb,be,tracecontainer,minband,maxband);
					else
						if ( self )
							alignTemplate<iterator,false,true,false>(ab,ae,bb,be,tracecontainer,minband,maxband);
						else
							alignTemplate<iterator,false,false,false>(ab,ae,bb,be,tracecontainer,minband,maxband);
				}
			}

			template<typename iterator>
			std::pair<int32_t,int32_t> alignNoTrace(
				iterator ab,
				iterator ae,
				iterator bb,
				iterator be,
				int64_t const minband = getDefaultMinDiag(),
				int64_t const maxband = getDefaultMaxDiag(),
				bool const forward = true,
				bool const self = false,
				bool const uniquetermval = false
			)
			{
				if ( uniquetermval )
				{
					if ( forward )
						if ( self )
							return alignTemplateNoTrace<iterator,true,true,true>(ab,ae,bb,be,minband,maxband);
						else
							return alignTemplateNoTrace<iterator,true,false,true>(ab,ae,bb,be,minband,maxband);
					else
						if ( self )
							return alignTemplateNoTrace<iterator,false,true,true>(ab,ae,bb,be,minband,maxband);
						else
							return alignTemplateNoTrace<iterator,false,false,true>(ab,ae,bb,be,minband,maxband);
				}
				else
				{
					if ( forward )
						if ( self )
							return alignTemplateNoTrace<iterator,true,true,false>(ab,ae,bb,be,minband,maxband);
						else
							return alignTemplateNoTrace<iterator,true,false,false>(ab,ae,bb,be,minband,maxband);
					else
						if ( self )
							return alignTemplateNoTrace<iterator,false,true,false>(ab,ae,bb,be,minband,maxband);
						else
							return alignTemplateNoTrace<iterator,false,false,false>(ab,ae,bb,be,minband,maxband);
				}
			}

			template<typename iterator>
			std::pair<int32_t,int32_t> alignMid(
				iterator ab,
				iterator ae,
				iterator bb,
				iterator be,
				int64_t const minband = getDefaultMinDiag(),
				int64_t const maxband = getDefaultMaxDiag(),
				bool const forward = true,
				bool const self = false,
				bool const uniquetermval = false
			)
			{
				if ( uniquetermval )
				{
					if ( forward )
						if ( self )
							return alignTemplateMid<iterator,true,true,true>(ab,ae,bb,be,minband,maxband);
						else
							return alignTemplateMid<iterator,true,false,true>(ab,ae,bb,be,minband,maxband);
					else
						if ( self )
							return alignTemplateMid<iterator,false,true,true>(ab,ae,bb,be,minband,maxband);
						else
							return alignTemplateMid<iterator,false,false,true>(ab,ae,bb,be,minband,maxband);
				}
				else
				{
					if ( forward )
						if ( self )
							return alignTemplateMid<iterator,true,true,false>(ab,ae,bb,be,minband,maxband);
						else
							return alignTemplateMid<iterator,true,false,false>(ab,ae,bb,be,minband,maxband);
					else
						if ( self )
							return alignTemplateMid<iterator,false,true,false>(ab,ae,bb,be,minband,maxband);
						else
							return alignTemplateMid<iterator,false,false,false>(ab,ae,bb,be,minband,maxband);
				}
			}

			#if 0
			template<typename iterator, bool forward, bool self, bool uniquetermval>
			std::pair<uint64_t,uint64_t> alignTemplateNoTrace(
				iterator ab,
				iterator ae,
				iterator bb,
				iterator be,
				int64_t const minband = getDefaultMinDiag(),
				int64_t const maxband = getDefaultMaxDiag()
			)
			#endif

			template<typename iterator>
			NNPAlignResult alignLinMem(
				iterator ab, iterator ae, uint64_t seedposa, iterator bb, iterator be, uint64_t seedposb,
				libmaus2::lcs::AlignmentTraceContainer & ATC,
				uint64_t const d = 128,
				bool const corclip = false
			)
			{
				ATC.reset();

				bool const self = false;

				std::pair<int32_t,int32_t> const SLA = alignNoTrace<iterator>(ab,ab+seedposa,bb,bb+seedposb,getDefaultMinDiag(),getDefaultMaxDiag(),false/*forward*/,self,funiquetermval);
				std::pair<int32_t,int32_t> const SLB = alignNoTrace<iterator>(ab+seedposa,ae,bb+seedposb,be,getDefaultMinDiag(),getDefaultMaxDiag(),true /*forward*/,self,funiquetermval);

				uint64_t abpos = seedposa - SLA.first;
				uint64_t bbpos = seedposb - SLA.second;
				uint64_t aepos = abpos + SLA.first + SLB.first;
				uint64_t bepos = bbpos + SLA.second + SLB.second;

				struct Entry
				{
					uint64_t abpos;
					uint64_t aepos;
					uint64_t bbpos;
					uint64_t bepos;

					Entry()
					{}

					Entry(
						uint64_t const rabpos,
						uint64_t const raepos,
						uint64_t const rbbpos,
						uint64_t const rbepos
					) : abpos(rabpos), aepos(raepos), bbpos(rbbpos), bepos(rbepos)
					{

					}

					std::string toString() const
					{
						std::ostringstream ostr;

						ostr << "(" << abpos << "," << aepos << "," << bbpos << "," << bepos << ")";

						return ostr.str();
					}
				};
				std::stack<Entry> Q;
				Q.push(Entry(abpos,aepos,bbpos,bepos));
				uint64_t ed = 0;

				while ( !Q.empty() )
				{
					Entry E = Q.top();
					Q.pop();

					bool split = false;

					if (
						std::max(
							E.aepos-E.abpos,
							E.bepos-E.bbpos
						) >= d
					)
					{
						std::pair<int32_t,int32_t> const P = alignMid(
							ab + E.abpos,
							ab + E.aepos,
							bb + E.bbpos,
							bb + E.bepos,
							getDefaultMinDiag(),getDefaultMaxDiag(),
							true /* forward */,
							self,
							funiquetermval
						);

						bool const aempty = (P.first==0);
						bool const bempty = (P.second==0);
						bool const empty = aempty && bempty;
						bool const afull = P.first == static_cast<int64_t>(E.aepos-E.abpos);
						bool const bfull = P.second == static_cast<int64_t>(E.bepos-E.bbpos);
						bool const full = afull && bfull;
						bool const trivial = empty || full;

						if ( !trivial )
						{
							Q.push(Entry(E.abpos+P.first,E.aepos,E.bbpos+P.second,E.bepos));
							Q.push(Entry(E.abpos,E.abpos+P.first,E.bbpos,E.bbpos+P.second));
							split = true;
						}
					}

					if ( ! split )
					{
						int64_t const led = npobj.np(
							ab+E.abpos,
							ab+E.aepos,
							bb+E.bbpos,
							bb+E.bepos,
							self
						);
						ATC.push(npobj);
						ed += led;
					}
				}

				if ( corclip )
				{
					std::pair<uint64_t,uint64_t> clipBack = ATC.lastGoodWindowBack(getWindowSize(), static_cast<double>(maxcerr) / getWindowSize());
					aepos -= clipBack.first;
					bepos -= clipBack.second;

					std::reverse(ATC.ta,ATC.te);
					std::pair<uint64_t,uint64_t> clipFront = ATC.lastGoodWindowBack(getWindowSize(), static_cast<double>(maxcerr) / getWindowSize());
					std::reverse(ATC.ta,ATC.te);

					abpos += clipFront.first;
					bbpos += clipFront.second;
				}

				// count number of errors
				return NNPAlignResult(abpos,aepos,bbpos,bepos,ATC.getAlignmentStatistics().getEditDistance());
			}

			template<typename iterator>
			NNPAlignResult align(
				iterator ab, iterator ae, uint64_t seedposa, iterator bb, iterator be, uint64_t seedposb,
				NNPTraceContainer & tracecontainer,
				// set valid diagonal band to avoid self matches when a and b are the same string
				bool const self = false,
				int64_t minfdiag = getDefaultMinDiag(),
				int64_t maxfdiag = getDefaultMaxDiag()
			)
			{
				tracecontainer.reset();

				// run backward alignment from seedpos
				if ( self )
				{
					if ( seedposa+ab >= seedposb+bb )
						minfdiag = std::max(static_cast<int64_t>(((seedposb+bb) - (seedposa+ab)) + 1),minfdiag);
					if ( seedposb+bb >= seedposa+ab )
						maxfdiag = std::min(static_cast<int64_t>(((seedposb+bb) - (seedposa+ab)) - 1),maxfdiag);

					//std::cerr << "minfdiag=" << minfdiag << " maxfdiag=" << maxfdiag << std::endl;
				}

				int64_t minrdiag = getDefaultMinDiag();
				int64_t maxrdiag = getDefaultMaxDiag();

				if ( minfdiag != getDefaultMinDiag() )
					maxrdiag = -minfdiag;
				if ( maxfdiag != getDefaultMaxDiag() )
					minrdiag = -maxfdiag;

				align(ab,ab+seedposa,bb,bb+seedposb,tracecontainer,minrdiag,maxrdiag,false /* forward */,self,funiquetermval);

				int64_t const revroot = tracecontainer.traceid;
				std::pair<uint64_t,uint64_t> const SLF = tracecontainer.getStringLengthUsed();

				// run forward alignment from seedpos
				align(ab+seedposa,ae,bb+seedposb,be,tracecontainer,minfdiag,maxfdiag,true /* forward */,self,funiquetermval);
				int64_t const forroot = tracecontainer.traceid;
				std::pair<uint64_t,uint64_t> const SLR = tracecontainer.getStringLengthUsed();

				// concatenate traces
				tracecontainer.traceid = tracecontainer.concat(revroot,forroot);

				std::pair<uint64_t,uint64_t> Dback = tracecontainer.clipEndWindowError(maxcerr);

				if ( runsuffixpositive )
				{
					std::pair<uint64_t,uint64_t> Sback = tracecontainer.suffixPositive(1,1,1,1);
					Dback.first += Sback.first;
					Dback.second += Sback.second;
				}

				tracecontainer.reverse();
				std::pair<uint64_t,uint64_t> Dfront = tracecontainer.clipEndWindowError(maxcerr);
				if ( runsuffixpositive )
				{
					std::pair<uint64_t,uint64_t> Sfront = tracecontainer.suffixPositive(1,1,1,1);
					Dfront.first += Sfront.first;
					Dfront.second += Sfront.second;
				}
				tracecontainer.reverse();

				// count number of errors
				uint64_t const dif = tracecontainer.getNumDif();
				uint64_t const abpos = seedposa - SLF.first + Dfront.first;
				uint64_t const bbpos = seedposb - SLF.second + Dfront.second;
				uint64_t const aepos = seedposa + SLR.first - Dback.first;
				uint64_t const bepos = seedposb + SLR.second - Dback.second;
				return NNPAlignResult(abpos,aepos,bbpos,bepos,dif);
			}
		};
	}
}
#endif
