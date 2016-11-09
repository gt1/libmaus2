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


				bool trueOverlap(libmaus2::dazzler::align::Overlap const & OVL, libmaus2::bambam::BamAlignment const & A, libmaus2::bambam::BamAlignment const & B)
				{
					uint64_t const aread = OVL.aread;
					uint64_t const bread = OVL.bread;

					// get common bases on reference
					uint64_t const o = libmaus2::bambam::BamAlignment::getRefCommon(A,B,V);

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

									if ( apos >= V[0].first && apos <= V[o-1].first )
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
							// std::cerr << "dropping " << OVL << std::endl;

							std::string const sta = A.getRead();
							std::string const stb = B.getRead();

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
