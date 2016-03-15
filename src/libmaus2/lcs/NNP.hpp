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
#if ! defined(LIBMAUS2_LCS_NNP_HPP)
#define LIBMAUS2_LCS_NNP_HPP

#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/lcs/BaseConstants.hpp>
#include <libmaus2/lcs/AlignmentTraceContainer.hpp>
#include <libmaus2/rank/popcnt.hpp>
#include <cstdlib>

namespace libmaus2
{
	namespace lcs
	{
		struct NNP : public libmaus2::lcs::BaseConstants
		{
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
			
			struct TraceElement
			{
				libmaus2::lcs::BaseConstants::step_type step;
				int32_t slide;
				int32_t parent;
				
				TraceElement(
					libmaus2::lcs::BaseConstants::step_type rstep = libmaus2::lcs::BaseConstants::STEP_RESET,
					int32_t rslide = 0,
					int32_t rparent = -1
				) : step(rstep), slide(rslide), parent(rparent)
				{
				}
			};
			
			libmaus2::autoarray::AutoArray<DiagElement> Acontrol;
			DiagElement * control;
			libmaus2::autoarray::AutoArray<DiagElement> Ancontrol;
			DiagElement * ncontrol;
			libmaus2::autoarray::AutoArray<TraceElement> Atrace;
			int64_t traceid;

			NNP() : Acontrol(1), control(Acontrol.begin()), Ancontrol(1), ncontrol(Ancontrol.begin()), traceid(-1)
			{
			}
			
			template<typename iterator>
			static uint64_t slide(iterator a, iterator ae, iterator b, iterator be)
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
			
			static void 
				allocate(
					int64_t const mindiag, int64_t const maxdiag, libmaus2::autoarray::AutoArray<DiagElement> & Acontrol,
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
					
					libmaus2::autoarray::AutoArray<DiagElement> Anewcontrol(2*newabsdiag+1);
					std::copy(Acontrol.begin(),Acontrol.end(),Anewcontrol.begin() + (newabsdiag - oldabsdiag));
					
					DiagElement * newcontrol = Anewcontrol.begin() + newabsdiag;
					for ( int64_t i = -oldabsdiag; i <= oldabsdiag; ++i )
						assert ( newcontrol[i] == control[i] );
						
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
									
			template<typename iterator>
			void align(iterator a, iterator ae, iterator b, iterator be)
			{
				ptrdiff_t const alen = ae-a;
				ptrdiff_t const blen = be-b;
				ptrdiff_t const mlen = std::min(alen,blen);
				uint64_t otrace = 0;

				int64_t mindiag = 0;
				int64_t maxdiag = 0;
				allocate(mindiag,maxdiag,Acontrol,control);
				
				control[0].offset = slide(a,ae,b,be);
				control[0].score = control[0].offset;
				control[0].evec = 0;
				control[0].id = otrace;

				Atrace.push(otrace,TraceElement(libmaus2::lcs::BaseConstants::STEP_RESET,control[0].offset,-1));

				bool foundalignment = (alen==blen) && (control[0].offset==alen);
				bool active = true;
				
				traceid = foundalignment ? control[0].id : -1;
				unsigned int const maxwerr = 24;
				
				int64_t maxscore = -1;
				int64_t maxscoretraceid = -1;
				
				#if 0
				std::vector<int64_t> Vactivediag;
				Vactivediag.push_back(0);
				#endif

				while ( !foundalignment && active )
				{
					active = false;

					// allocate control for next round
					allocate(mindiag-1,maxdiag+1,Ancontrol,ncontrol);
					
					#if 0
					assert ( Vactivediag.size() );
					allocate(Vactivediag.front()-1,Vactivediag.back()+1,Ancontrol,ncontrol);
					
					std::vector<int64_t> Vcompdiag;
					
					for ( uint64_t i = 0; i < Vactivediag.size(); ++i )
					{
						bool const prevactive = i && (Vactivediag[i-1] == Vactivediag[i]-1);
						bool const nextactive = i+1 < Vactivediag.size() && (Vactivediag[i+1] == Vactivediag[i]+1);
						
						if ( ! prevactive )
							Vcompdiag.push_back(Vactivediag[i]-1);
						Vcompdiag.push_back(Vactivediag[i]);
						if ( ! nextactive )
							Vcompdiag.push_back(Vactivediag[i+1]);
					}
					#endif
					
					int64_t const minvalindiag = mindiag;
					int64_t const maxvalindiag = maxdiag;
					
					int64_t maxroundoffset = -1;

					// iterate over diagonals for next round
					for ( int64_t d = mindiag-1; d <= maxdiag+1; ++d )
					{
						int64_t const apre = (d >= 0) ? d : 0;
						int64_t const bpre = (d >= 0) ? 0 : -d;
						assert ( apre >= 0 );
						assert ( bpre >= 0 );
						int64_t const maxoffset = std::min(alen-apre,blen-bpre);
						bool const newdiag = (d < minvalindiag || d > maxvalindiag);
						
						/*
						 * find offset we will try to slide from
						 */
						 
						/*
						 * if we can try from top / next higher diagonal
						 */
						int64_t offtop = -1;
						if ( 
							d+1 >= minvalindiag && d+1 <= maxvalindiag && (control[d+1].offset >= 0)
						)
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
						if ( 
							d-1 >= minvalindiag && d-1 <= maxvalindiag && (control[d-1].offset >= 0)
						)
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
						if ( 
							d >= minvalindiag && d <= maxvalindiag && (control[d].offset >= 0)
						)
						{
							offdiag = control[d].offset + 1; // mismatch
							if ( offdiag > maxoffset )
								offdiag = -1;
						}

						#if 0
						if ( d >= minvalindiag && d <= maxvalindiag )
						{
							std::cerr << "diagonal " << d << " offset " << control[d].offset << " werr=" << popcnt(control[d].evec) << std::endl;
						}
						else
						{
							std::cerr << "new diagonal " << d << std::endl;
						}
						
						std::cerr << "offleft=" << offleft << " offdiag=" << offdiag << " offtop=" << offtop << std::endl;
						#endif

						// first mark as invalid
						ncontrol[d].offset = -1;

						if ( offtop >= offleft )
						{
							if ( offdiag >= offtop )
							{
								if ( 
									offdiag >= 0 && 
									(
										newdiag ||
										(offdiag > control[d].offset)
									)
								)
								{
									assert ( ! newdiag );

									int64_t const aoff = apre + offdiag;
									int64_t const boff = bpre + offdiag;
									int64_t const slidelen = slide(a+aoff,ae,b+boff,be);
									uint64_t const newevec = (slidelen < 64) ? (((control[d].evec << 1) | 1) << slidelen) : 0;
									unsigned int const numerr = popcnt(newevec);

									if ( numerr < maxwerr )
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

										Atrace.push(otrace,TraceElement(libmaus2::lcs::BaseConstants::STEP_MISMATCH,slidelen,control[d].id));
										
										active = true;
									}
								}
							}
							else // offtop maximal
							{
								assert ( offtop >= 0 );

								if ( newdiag || (offtop > control[d].offset) )
								{
									int64_t const aoff = apre + offtop;
									int64_t const boff = bpre + offtop;
																		
									int64_t const slidelen = slide(a+aoff,ae,b+boff,be);
									uint64_t const newevec = (slidelen < 64) ? (((control[d+1].evec << 1) | 1) << slidelen) : 0;
									unsigned int const numerr = popcnt(newevec);
									
									if ( numerr < maxwerr )
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

										Atrace.push(otrace,TraceElement(libmaus2::lcs::BaseConstants::STEP_INS,slidelen,control[d+1].id));
										
										active = true;
									}
								}
							}
						}
						else // offtop < offleft
						{
							if ( offdiag >= offleft )
							{
								if ( offdiag >= 0 && (newdiag || (offdiag > control[d].offset)) )
								{
									assert ( ! newdiag );
									
									int64_t const aoff = apre + offdiag;
									int64_t const boff = bpre + offdiag;
									int64_t const slidelen = slide(a+aoff,ae,b+boff,be);
									uint64_t const newevec = (slidelen < 64) ? (((control[d].evec << 1) | 1) << slidelen) : 0;
									unsigned int const numerr = popcnt(newevec);

									if ( numerr < maxwerr )
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

										Atrace.push(otrace,TraceElement(libmaus2::lcs::BaseConstants::STEP_MISMATCH,slidelen,control[d].id));
										
										active = true;
									}
								}
							}
							else // offleft maximal
							{
								if ( newdiag || (offleft > control[d].offset) )
								{
									assert ( offleft >= 0 );
									int64_t const aoff = apre + offleft;
									int64_t const boff = bpre + offleft;
									int64_t const slidelen = slide(a+aoff,ae,b+boff,be);
									uint64_t const newevec = (slidelen < 64) ? (((control[d-1].evec << 1) | 1) << slidelen) : 0;
									unsigned int const numerr = popcnt(newevec);
									
									if ( numerr < maxwerr )
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

										Atrace.push(otrace,TraceElement(libmaus2::lcs::BaseConstants::STEP_DEL,slidelen,control[d-1].id));
										
										active = true;
									}
								}
							}
						}
						
						if ( d == (alen-blen) && ncontrol[d].offset == mlen )
						{
							std::cerr << "yes " << d << ",offset=" 
								<< ncontrol[d].offset << ",score=" << ncontrol[d].score 
								<< ",alen=" << alen << ",blen=" << blen << ",mlen=" << mlen
								<< std::endl;
							foundalignment = true;
							traceid = otrace-1;
						}
						
						if ( ncontrol[d].offset > maxroundoffset )
							maxroundoffset = ncontrol[d].offset;
							
					}
					
					Acontrol.swap(Ancontrol);
					std::swap(control,ncontrol);
					
					mindiag -= 1;
					maxdiag += 1;

					#if 0
					for ( int64_t d = mindiag; d <= maxdiag; ++d )
					{
						std::cerr << d << " " << control[d].offset << " " << maxroundoffset << std::endl;
					}
					#endif
				}
				
				if ( ! foundalignment )
				{
					std::cerr << "no full alignment found, cutting back to best score" << std::endl;
					traceid = maxscoretraceid;
				}
				
				std::cerr << "maxscore=" << maxscore << std::endl;
				std::cerr << "maxscoretraceid=" << maxscoretraceid << std::endl;
				std::cerr << "traceid=" << traceid << std::endl;
				
				#if 0
				for ( int64_t curtraceid = traceid ; curtraceid >= 0; curtraceid = Atrace[curtraceid].parent )
				{
					std::cerr << "traceid=" << curtraceid << " op " << Atrace[curtraceid].step << " slide " << Atrace[curtraceid].slide << std::endl;					
				}
				#endif
			}

			void computeTrace(libmaus2::lcs::AlignmentTraceContainer & ATC) const
			{
				uint64_t reserve = 0;
				for ( int64_t curtraceid = traceid ; curtraceid >= 0; curtraceid = Atrace[curtraceid].parent )
					reserve += Atrace[curtraceid].slide + 1;
				
				if ( ATC.capacity() < reserve )
					ATC.resize(reserve);
				ATC.reset();
				
				// std::cerr << "reserve " << reserve << std::endl;
				
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
		};
	}
}
#endif
