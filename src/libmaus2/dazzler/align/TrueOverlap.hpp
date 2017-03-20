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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_TRUEOVERLAP_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_TRUEOVERLAP_HPP

#include <libmaus2/dazzler/align/TrueOverlapStats.hpp>
#include <libmaus2/lcs/NP.hpp>
#include <libmaus2/dazzler/db/DatabaseFile.hpp>
#include <libmaus2/dazzler/align/Overlap.hpp>
#include <libmaus2/lcs/AlignmentPrint.hpp>
#include <libmaus2/bambam/BamAlignment.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct TrueOverlap
			{
				typedef TrueOverlap this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				TrueOverlapStats & stats;
				libmaus2::dazzler::db::DatabaseFile const & DB;
				int64_t const tspace;
				libmaus2::autoarray::AutoArray< std::pair<uint32_t,uint32_t> > V;
				libmaus2::lcs::AlignmentTraceContainer ATC;
				libmaus2::lcs::NP aligner;

				libmaus2::bambam::BamAlignment TA;
				libmaus2::bambam::BamAlignment TB;

				TrueOverlap(TrueOverlapStats & rstats, libmaus2::dazzler::db::DatabaseFile const & rDB, int64_t const rtspace) : stats(rstats), DB(rDB), tspace(rtspace)
				{

				}

				static void copyAlignment(uint8_t const * a, uint8_t const * e, libmaus2::bambam::BamAlignment & A)
				{
					A.D.ensureSize(e-a);
					std::copy(a,e,A.D.begin());
					A.blocksize = e-a;
					A.checkAlignment();
				}

				static bool trueOverlap(
					libmaus2::bambam::BamAlignment const & abam,
					libmaus2::bambam::BamAlignment const & bbam,
					// trace a -> b
					libmaus2::lcs::AlignmentTraceContainer const & OATC,
					int64_t const yabpos,
					int64_t const ybbpos,
					bool const yisinv,
					uint64_t const blen
				)
				{
					if (
						abam.getRefID() == bbam.getRefID()
					)
					{
						libmaus2::math::IntegerInterval<int64_t> const Aref = abam.getReferenceInterval();
						libmaus2::math::IntegerInterval<int64_t> const Bref = bbam.getReferenceInterval();
						libmaus2::math::IntegerInterval<int64_t> const Cref = Aref.intersection(Bref);

						if ( ! Cref.isEmpty() )
						{
							// A -> ref
							libmaus2::autoarray::AutoArray<libmaus2::bambam::cigar_operation> Acig;
							uint32_t const ancigar = abam.getCigarOperations(Acig);
							libmaus2::lcs::AlignmentTraceContainer abamATC;
							libmaus2::bambam::CigarStringParser::cigarToTrace(Acig.begin(),Acig.begin()+ancigar,abamATC);
							int64_t abampos = abam.getPos();
							int64_t areadpos = abam.getFrontSoftClipping();
							for ( int64_t i = 0; i < abamATC.te-abamATC.ta; ++i )
								if ( abamATC.ta[i] == libmaus2::lcs::AlignmentTraceContainer::STEP_DEL )
									--abampos;
								else
									break;

							std::vector < std::pair<int64_t,int64_t> > VMA;

							std::pair<int64_t,int64_t> SLA = abamATC.getStringLengthUsed();
							SLA.first += abampos;
							SLA.second += areadpos;

							for  ( libmaus2::lcs::AlignmentTraceContainer::step_type const * tc = abamATC.ta; tc != abamATC.te; ++tc )
							{
								libmaus2::lcs::AlignmentTraceContainer::step_type step = *tc;

								switch ( step )
								{
									case libmaus2::lcs::AlignmentTraceContainer::STEP_MATCH:
										VMA.push_back(std::pair<int64_t,int64_t>(abampos,areadpos));
										abampos++;
										areadpos++;
										break;
									case libmaus2::lcs::AlignmentTraceContainer::STEP_MISMATCH:
										abampos++;
										areadpos++;
										break;
									case libmaus2::lcs::AlignmentTraceContainer::STEP_DEL:
										abampos++;
										break;
									case libmaus2::lcs::AlignmentTraceContainer::STEP_INS:
										areadpos++;
										break;
									default:
										break;
								}
							}
							assert ( abampos == SLA.first );
							assert ( areadpos == SLA.second );

							if ( abam.isReverse() )
								for ( uint64_t i = 0; i < VMA.size(); ++i )
									VMA[i].second = abam.getLseq() - VMA[i].second - 1;

							// B -> ref
							libmaus2::autoarray::AutoArray<libmaus2::bambam::cigar_operation> Bcig;
							uint32_t const bncigar = bbam.getCigarOperations(Bcig);
							libmaus2::lcs::AlignmentTraceContainer bbamATC;
							libmaus2::bambam::CigarStringParser::cigarToTrace(Bcig.begin(),Bcig.begin()+bncigar,bbamATC);
							int64_t bbampos = bbam.getPos();
							int64_t breadpos = bbam.getFrontSoftClipping();
							for ( int64_t i = 0; i < bbamATC.te-bbamATC.ta; ++i )
								if ( bbamATC.ta[i] == libmaus2::lcs::AlignmentTraceContainer::STEP_DEL )
									--bbampos;
								else
									break;

							std::vector < std::pair<int64_t,int64_t> > VMB;

							std::pair<int64_t,int64_t> SLB = bbamATC.getStringLengthUsed();
							SLB.first += bbampos;
							SLB.second += breadpos;

							while  ( bbamATC.ta != bbamATC.te )
							{
								libmaus2::lcs::AlignmentTraceContainer::step_type step = *(bbamATC.ta++);

								switch ( step )
								{
									case libmaus2::lcs::AlignmentTraceContainer::STEP_MATCH:
										VMB.push_back(std::pair<int64_t,int64_t>(bbampos,breadpos));
										bbampos++;
										breadpos++;
										break;
									case libmaus2::lcs::AlignmentTraceContainer::STEP_MISMATCH:
										bbampos++;
										breadpos++;
										break;
									case libmaus2::lcs::AlignmentTraceContainer::STEP_DEL:
										bbampos++;
										break;
									case libmaus2::lcs::AlignmentTraceContainer::STEP_INS:
										breadpos++;
										break;
									default:
										break;
								}
							}
							assert ( bbampos == SLB.first );
							assert ( breadpos == SLB.second );

							if ( bbam.isReverse() )
								for ( uint64_t i = 0; i < VMB.size(); ++i )
									VMB[i].second = bbam.getLseq() - VMB[i].second - 1;

							uint64_t vmai = 0, vmbi = 0;

							std::vector < std::pair<int64_t,int64_t> > VMC;

							while ( vmai < VMA.size() && vmbi < VMB.size() )
							{
								if ( VMA[vmai].first < VMB[vmbi].first )
									++vmai;
								else if ( VMB[vmbi].first < VMA[vmai].first )
									++vmbi;
								else
								{
									VMC.push_back(std::pair<int64_t,int64_t>(VMA[vmai].second,VMB[vmbi].second));
									++vmai;
									++vmbi;
								}
							}

							std::sort(VMC.begin(),VMC.end());

							//libmaus2::lcs::AlignmentTraceContainer const & OATC = *(Mtraces.find(y)->second);
							int64_t oapos = yabpos; // ita[y].path.abpos;
							int64_t obpos = ybbpos; // ita[y].path.bbpos;

							libmaus2::lcs::AlignmentTraceContainer::step_type const * ota = OATC.ta;
							libmaus2::lcs::AlignmentTraceContainer::step_type const * ote = OATC.te;

							std::vector < std::pair<int64_t,int64_t> > VMO;

							while ( ota != ote )
							{
								libmaus2::lcs::AlignmentTraceContainer::step_type const step = *(ota++);

								switch ( step )
								{
									case libmaus2::lcs::AlignmentTraceContainer::STEP_MATCH:
										VMO.push_back(std::pair<int64_t,int64_t>(oapos,obpos));
										oapos++;
										obpos++;
										break;
									case libmaus2::lcs::AlignmentTraceContainer::STEP_MISMATCH:
										oapos++;
										obpos++;
										break;
									case libmaus2::lcs::AlignmentTraceContainer::STEP_DEL:
										oapos++;
										break;
									case libmaus2::lcs::AlignmentTraceContainer::STEP_INS:
										obpos++;
										break;
									default:
										break;
								}
							}

							// if ( ita[y].isInverse() )
							if ( yisinv )
							{
								uint64_t const l = blen; // RC2.getReadLength(ita[y].bread);
								for ( uint64_t i = 0; i < VMO.size(); ++i )
									VMO[i].second = l - VMO[i].second - 1;
							}

							uint64_t ma = 0, mb = 0;
							bool found = false;

							while ( (!found) && ma < VMC.size() && mb < VMO.size() )
							{
								if ( VMC[ma] < VMO[mb] )
									++ma;
								else if ( VMO[mb] < VMC[ma] )
									++mb;
								else
								{
									assert ( VMC[ma] == VMO[mb] );
									found = true;
								}
							}

							if ( found )
							{
								return true;
							}
							else
							{
								//err << "not found" << std::endl;
							}
						}
					}

					return false;
				}

				bool trueOverlap(libmaus2::dazzler::align::Overlap const & OVL, libmaus2::bambam::BamAlignment const & A, libmaus2::bambam::BamAlignment const & B)
				{
					uint64_t const aread = OVL.aread;
					uint64_t const bread = OVL.bread;

					// get common bases on reference
					uint64_t const o = libmaus2::bambam::BamAlignment::getRefCommon(A,B,V);
					int const arev = A.isReverse();
					int const brev = B.isReverse();
					int rev = (arev+brev)%2;

					if ( A.isReverse() )
					{
						uint64_t const alen = A.getLseq();

						for ( uint64_t i = 0; i < o; ++i )
							V[i].first = alen - V[i].first - 1;
					}
					if ( B.isReverse() )
					{
						uint64_t const blen = B.getLseq();

						for ( uint64_t i = 0; i < o; ++i )
							V[i].second = blen - V[i].second - 1;
					}

					if ( o )
					{
						stats.ncom++;

						std::string const astr = DB[aread];
						std::string const prebstr = DB[bread];
						std::string const bstr = OVL.isInverse() ? libmaus2::fastx::reverseComplementUnmapped(prebstr) : prebstr;

						OVL.computeTrace(
							reinterpret_cast<uint8_t const *>(astr.c_str()),
							reinterpret_cast<uint8_t const *>(bstr.c_str()),
							tspace,
							ATC,
							aligner
						);

						if ( OVL.isInverse() )
						{
							rev += 1;
							rev %= 2;

							uint64_t const blen = B.getLseq();

							for ( uint64_t i = 0; i < o; ++i )
								V[i].second = blen - V[i].second - 1;
						}

						std::sort(V.begin(),V.begin()+o);

						#if 0
						libmaus2::lcs::AlignmentPrint::printAlignmentLines(
							std::cerr,
							astr.begin() + OVL.path.abpos,
							OVL.path.aepos - OVL.path.abpos,
							bstr.begin() + OVL.path.bbpos,
							OVL.path.bepos - OVL.path.bbpos,
							80,
							ATC.ta,
							ATC.te
						);
						#endif

						int64_t apos = OVL.path.abpos;
						int64_t bpos = OVL.path.bbpos;

						libmaus2::lcs::AlignmentTraceContainer::step_type const * ta = ATC.ta;
						libmaus2::lcs::AlignmentTraceContainer::step_type const * te = ATC.te;
						bool found = false;

						for ( ; (!found) && (ta != te); ++ta )
						{
							// std::cerr << "apos=" << apos << " bpos=" << bpos << std::endl;

							switch ( *ta )
							{
								case libmaus2::lcs::BaseConstants::STEP_MATCH:
								{
									assert ( astr[apos] == bstr[bpos] );

									// if ( apos >= V[0].first && apos <= V[o-1].first )
									{
										std::pair <
											std::pair<uint32_t,uint32_t> const *,
											std::pair<uint32_t,uint32_t> const *
										> const P =  std::equal_range(V.begin(),V.begin()+o,std::pair<uint32_t,uint32_t>(apos,bpos));

										if ( P.second != P.first )
										{
											// std::cerr << "got it apos=" << apos << " bpos=" << bpos << " refid " << A.getRefID() << " aread " << aread << std::endl;
											found = true;
										}
										else
										{
											#if 0
											std::cerr << "apos=" << apos << " bpos=" << bpos << " " << A.isReverse() << " " << B.isReverse() << std::endl;

											std::pair<uint32_t,uint32_t> const * L = std::lower_bound(V.begin(),V.begin()+o,std::pair<uint32_t,uint32_t>(apos,bpos));
											if ( L != V.begin() )
											{
												std::cerr << "prev " << L[-1].first << "," << L[-1].second << std::endl;
											}
											if ( L != V.begin()+o )
											{
												std::cerr << "next " << L->first << "," << L->second << std::endl;
											}
											#endif
										}
									}

									apos++;
									bpos++;
									break;
								}
								case libmaus2::lcs::BaseConstants::STEP_MISMATCH:
									apos++;
									bpos++;
									break;
								case libmaus2::lcs::BaseConstants::STEP_INS:
									bpos++;
									break;
								case libmaus2::lcs::BaseConstants::STEP_DEL:
									apos++;
									break;
								default:
									break;
							}
						}

						#if 0
						for ( uint64_t i = 0; i < o; ++i )
							std::cerr << V[i].first << "," << V[i].second << std::endl;
						std::cerr << std::endl;
						#endif

						if ( found )
						{
							stats.nkept++;
							return true;
						}
						else
						{
							#if 0
							std::cerr << "dropping " << OVL << std::endl;
							#endif

							std::string const sta = A.isReverse() ? A.getReadRC() : A.getRead();
							std::string const stb = (B.isReverse() ^ OVL.isInverse()) ? B.getReadRC() : B.getRead();

							if ( rev )
								for ( uint64_t i = 0; i < o; ++i )
									assert ( sta[V[i].first] == libmaus2::fastx::invertUnmapped(stb[V[i].second]) );
							else
								for ( uint64_t i = 0; i < o; ++i )
									assert ( sta[V[i].first] == stb[V[i].second] );

							#if 0
							for ( uint64_t i = 0; i < o; ++i )
								std::cerr
									<< V[i].first << "," << V[i].second
									<< " " << sta[V[i].first]
									<< " " << stb[V[i].second]
									<< std::endl;
							std::cerr << std::endl;
							#endif


							stats.ndisc++;
							return false;
						}
					}
					else
					{
						stats.ndisc++;
						stats.nuncom++;
						return false;
					}
				}

				bool trueOverlap(
					libmaus2::dazzler::align::Overlap const & OVL,
					uint8_t const * aa,
					uint8_t const * ae,
					uint8_t const * ba,
					uint8_t const * be
				)
				{
					copyAlignment(aa,ae,TA);
					copyAlignment(ba,be,TB);
					return trueOverlap(OVL,TA,TB);
				}
			};
		}
	}
}
#endif
