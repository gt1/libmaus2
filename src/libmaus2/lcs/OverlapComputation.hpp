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

#if ! defined(LIBMAUS2_LCS_OVERLAPCOMPUTATION_HPP)
#define LIBMAUS2_LCS_OVERLAPCOMPUTATION_HPP

#include <iostream>
#include <cerrno>
#include <cstring>
#include <iomanip>
#include <queue>
#include <stack>

#include <libmaus2/types/types.hpp>
#include <libmaus2/lcs/CheckOverlapResult.hpp>
#include <libmaus2/lcs/SuffixPrefix.hpp>
#include <libmaus2/lcs/HashContainer.hpp>
#include <libmaus2/lcs/HashContainer2.hpp>
#include <libmaus2/fastx/FastInterval.hpp>
#include <libmaus2/fastx/FastQReader.hpp>
#include <libmaus2/fastx/FastAReader.hpp>
#if ! defined(_WIN32)
#include <libmaus2/fastx/FastClient.hpp>
#endif
#if ! defined(_WIN32)
#include <libmaus2/util/Terminal.hpp>
#endif
#include <libmaus2/aio/SynchronousOutputFile8Array.hpp>
#include <libmaus2/aio/SynchronousGenericOutput.hpp>
#include <libmaus2/aio/GenericInput.hpp>
#include <libmaus2/lcs/LCSDynamic.hpp>
#include <libmaus2/graph/TripleEdge.hpp>
#include <libmaus2/fastx/CompactReadContainer.hpp>
#include <libmaus2/lcs/OverlapComputationBlockRequest.hpp>
#include <libmaus2/lcs/OverlapOrientation.hpp>
#include <libmaus2/network/Socket.hpp>
#include <libmaus2/lcs/OrientationWeightEncoding.hpp>
#include <libmaus2/lcs/HammingOverlapDetection.hpp>

namespace libmaus2
{
	namespace lcs
	{
		struct OverlapComputation : public OrientationWeightEncoding
		{

			static unsigned long getDefaultMinTraceLength()
			{
				return 40;
			}
			static int64_t getDefaultMinScore()
			{
				return 0;
			}
			static double getDefaultMaxIndelFrac()
			{
				return 0.05;
			}
			static double getDefaultMaxSubstFrac()
			{
				return 0.05;
			}
			static uint64_t getDefaultScoreWindowSize()
			{
				return 100;
			}
			static int64_t getDefaultWindowMinScore()
			{
				return 0;
			}

			// private:
			struct SecondComponentComparator
			{
				bool operator()(edge_type const & A, edge_type const & B) const
				{
					if ( A.b != B.b )
						return A.b < B.b;
					else
						return A.a < B.a;
				} 
			};
			
			/* overlap parameters */
			/* minimum overlap */
			static unsigned long const defaultmintracelength;
			/* max allowed indels in overlap */
			static double const defaultmaxindelfrac;
			/* max allowed substitutions in overlap */
			static double const defaultmaxsubstfrac;
			/* minimum overlap */
			static int64_t const defaultminscore;
			
			public:
			static int64_t showOverlapSingle(
				std::ostream & out, 
				std::string const & a, std::string const & b,
				double const /* maxindelfrac */ = getDefaultMaxIndelFrac()
			)
			{
				#if 1
				::libmaus2::lcs::SuffixPrefixDna5 SP(a.size(),b.size());
				::libmaus2::lcs::SuffixPrefixResult SPR = SP.process(a.begin(),b.begin());
				// uint64_t const tracelength = SP.getTraceLength();
				SP.printAlignmentLines(out,a,b,SPR,::libmaus2::util::Terminal::getColumns());
				#else
				::libmaus2::lcs::HashContainer2 HC(12 /* k */, a.size());
				HC.process(a.begin());
				::libmaus2::lcs::SuffixPrefixResult const SPR = HC.match(b.begin(),b.size(),a.begin(),maxindelfrac);
				HC.printAlignmentLines(out,a.begin(),b.begin(),b.size(),SPR,
					#if ! defined(_WIN32)
					::libmaus2::util::Terminal::getColumns()
					#else
					80
					#endif
				);
				// uint64_t const tracelength = HC.trace.size();
				#endif

				out << SPR << std::endl;
			
				return SP.getTraceScore();
			}

			static void showOverlap(
				std::ostream & out, 
				std::string const & a, std::string const & b,
				double const maxindelfrac = getDefaultMaxIndelFrac()
			)
			{
				std::ostringstream sstr, rstr;
				int64_t const scores = showOverlapSingle(sstr,a,b,maxindelfrac);
				int64_t const scorer = showOverlapSingle(rstr,::libmaus2::fastx::reverseComplementUnmapped(b),::libmaus2::fastx::reverseComplementUnmapped(a),maxindelfrac);
				
				if ( scores >= scorer )
					out << sstr.str();
				else
					out << rstr.str();			
			}

			static int64_t showOverlapCoverSingle(
				std::ostream & out, 
				std::string const & a, std::string const & b,
				double const /* maxindelfrac */ = getDefaultMaxIndelFrac()
			)
			{
				#if 1
				::libmaus2::lcs::SuffixPrefixDna5 SP(a.size(),b.size());
				::libmaus2::lcs::SuffixPrefixResult SPR = SP.process(a.begin(),b.begin());
				// uint64_t const tracelength = SP.getTraceLength();
				SP.printInfixAlignment(a.begin(),b.begin(),SPR,out);
				#else
				::libmaus2::lcs::HashContainer2 HC(12 /* k */, a.size());
				HC.process(a.begin());
				::libmaus2::lcs::SuffixPrefixResult const SPR = HC.match(b.begin(),b.size(),a.begin(),maxindelfrac);
				HC.printInfixAlignment(a.begin(),b.begin(),SPR,out);
				// uint64_t const tracelength = HC.trace.size();
				#endif
				
				out << SPR << std::endl;
				
				if ( 
					SP.infixtrace.ta != SP.infixtrace.te && 
					(
						(*(SP.infixtrace.ta  ) == SP.STEP_DEL)
						||
						(*(SP.infixtrace.te-1) == SP.STEP_DEL)
					)
				)
					return std::numeric_limits<int64_t>::min();
				else
					return SPR.infix_score;
			}

			static void showOverlapCover(
				std::ostream & out, 
				std::string const & a, std::string const & b,
				double const maxindelfrac = getDefaultMaxIndelFrac()
			)
			{
				std::ostringstream sstr, rstr;
				int64_t const scores = showOverlapCoverSingle(sstr,a,b,maxindelfrac);
				int64_t const scorer = showOverlapCoverSingle(rstr,::libmaus2::fastx::reverseComplementUnmapped(b),::libmaus2::fastx::reverseComplementUnmapped(a),maxindelfrac);

				#if 0				
				out << "scores=" << scores << " scorer=" << scorer << std::endl;

				out << sstr.str();
				out << rstr.str();
				#endif
				
				#if 1
				if ( scores >= scorer )
					out << sstr.str();
				else
					out << rstr.str();
				#endif
			}

			static uint64_t printQuadAlignment(std::string const & a, std::string const & b)
			{
				::libmaus2::lcs::SuffixPrefixDna5 SP(a.size(),b.size());
				::libmaus2::lcs::SuffixPrefixResult const SPR = SP.process(a.begin(),b.begin());
				std::cerr << "quad alignment: " << std::endl;
				SP.printAlignmentLines(std::cerr,a,b,SPR,
					#if ! defined(_WIN32)
					::libmaus2::util::Terminal::getColumns()
					#else
					80
					#endif
				);
				return std::max(0, SP.getTraceScore() ); // ::libmaus2::lcs::TraceContainer::getTraceScore(SP.trace.begin(),SP.trace.end()));
			}
			
			enum single_check_type {
				single_check_both,
				single_check_infix,
				single_check_suffix
			};

			static CheckOverlapResult checkOverlapSingleDirection(
				uint64_t const ia, uint64_t const ib,
				std::string const & a, std::string const & b,
				unsigned int const mintracelength,
				int64_t const minscore,
				uint64_t const scorewindowsize,
				int64_t const windowminscore,
				double const maxindelfrac,
				double const maxsubstfrac,
				overlap_orientation const dovetail_orientation,
				overlap_orientation const containment_orientation,
				single_check_type const singlechecktype = single_check_both
			)
			{
				#if 1
				::libmaus2::lcs::SuffixPrefixDna5 HC(a.size(),b.size());
				::libmaus2::lcs::SuffixPrefixResult SPR = HC.process(a.begin(),b.begin());
				uint64_t const tracelength = HC.getTraceLength();
				#else
				::libmaus2::lcs::HashContainer2 HC(12 /* k */, a.size());
				HC.process(a.begin());
				::libmaus2::lcs::SuffixPrefixResult const SPR = HC.match(b.begin(),b.size(),a.begin(),maxindelfrac);
				uint64_t const tracelength = HC.trace.size();
				#endif

				int64_t const score = HC.getTraceScore();
				int64_t const windowscore = HC.getWindowMinScore(scorewindowsize);
				// int64_t const infixscore = HC.infixtrace.getTraceScore();
				int64_t const infixwindowscore = HC.infixtrace.getWindowMinScore(scorewindowsize);
				
				#if 0
				if ( score >= minscore )
				{
					std::cerr << "windowscore=" << windowscore << " windowminscore=" << windowminscore << std::endl;
				}
				#endif
				
				// test for containment
				if ( 
					infixwindowscore >= windowminscore
					&&
					(SPR.infix_numins + SPR.infix_numdel) <= (maxindelfrac * b.size())
					&&
					(SPR.infix_nummis)                    <= (maxsubstfrac * b.size())
					&&
					(HC.infixtrace.ta != HC.infixtrace.te)
					&&
					( *(HC.infixtrace.ta  ) != HC.STEP_DEL)
					&&
					( *(HC.infixtrace.te-1) != HC.STEP_DEL)
					&&
					(
						singlechecktype == single_check_both
						||
						singlechecktype == single_check_infix
					)
				)
				{

					CheckOverlapResult const COR (
						true,
						ia,
						ib,
						SPR.infix_score,
						infixwindowscore,
						SPR.infix_aclip_left, // overhang left
						SPR.infix_aclip_right, // overhang right
						SPR.infix_nummat,
						SPR.infix_nummis,
						SPR.infix_numins,
						SPR.infix_numdel,
						containment_orientation,
						HC.infixtrace.traceToString()
						);
					
					#if 0
					std::cerr << "Found containment " << COR << std::endl;
					HC.printInfixAlignment(a.begin(),b.begin(),SPR,std::cerr);
					// std::cerr << COR << std::endl;
					#endif

					return COR;
				}
				else if ( 
					windowscore >= windowminscore
					&&
					score >= minscore
					&&
					tracelength >= mintracelength
					&&
					((SPR.numdel + SPR.numins) <= maxindelfrac * tracelength)
					&&
					(SPR.nummis <= maxsubstfrac * tracelength)
					&&
					(
						singlechecktype == single_check_both
						||
						singlechecktype == single_check_suffix
					)
				)
				{
					// #define OCSHOW

					#if defined(OCSHOW)
						std::cerr <<
							"tracelength=" << tracelength << " >= mintracelength=" << mintracelength << " " <<
							"score=" << score << " >= minscore=" << minscore << " " <<
							"SPR.nummat=" << SPR.nummat << " " <<
							"SPR.nummis=" << SPR.nummis << " " <<
							"SPR.numins=" << SPR.numins << " " <<
							"SPR.numdel=" << SPR.numdel << " " <<
							"maxindelfrac=" << maxindelfrac << " " <<
							"maxsubstfrac=" << maxsubstfrac << " " <<
							"orientation=" << dovetail_orientation << " " <<
							std::endl;
						std::cerr << "trace=" << HC.traceToString() << std::endl;
					#endif

					assert ( SPR.nummis + SPR.nummat + SPR.numins <= a.size() );
					assert ( SPR.nummis + SPR.nummat + SPR.numdel <= b.size() );
					assert ( SPR.nummis + SPR.nummat + SPR.numins + SPR.aclip_left  == a.size() );
					assert ( SPR.nummis + SPR.nummat + SPR.numdel + SPR.bclip_right == b.size() );
				
					CheckOverlapResult const COR (
						true,
						ia,
						ib,
						score,
						windowscore,
						SPR.aclip_left, // overhang left
						SPR.bclip_right, // overhang right
						SPR.nummat,
						SPR.nummis,
						SPR.numins,
						SPR.numdel,
						dovetail_orientation,
						HC.traceToString()
						);

					assert ( COR.nummis + COR.nummat + COR.numins <= a.size() );
					assert ( COR.nummis + COR.nummat + COR.numdel <= b.size() );
					assert ( COR.nummis + COR.nummat + COR.numins + COR.clipa == a.size() );
					assert ( COR.nummis + COR.nummat + COR.numdel + COR.clipb == b.size() );					
						
					return COR;
				}
				else
				{
					return CheckOverlapResult(
						false,ia,ib,
						std::numeric_limits<int64_t>::min() /* score */,
						std::numeric_limits<int64_t>::min() /* window min score */,
						0,0,0,0,0,0,dovetail_orientation,"");
				}
			}
			
			static CheckOverlapResult checkOverlap(
				uint64_t const ia, uint64_t const ib,
				std::string const & a, std::string const & b,
				unsigned int const mintracelength,
				int64_t const minscore,
				uint64_t const scorewindowsize,
				int64_t const windowminscore,
				double const maxindelfrac,
				double const maxsubstfrac,
				overlap_orientation const dovetail_orientation,
				overlap_orientation const containment_orientation,
				single_check_type const singlechecktype = single_check_both
				)
			{
				/* straight */
				CheckOverlapResult CORs = checkOverlapSingleDirection(
					ia,ib,
					a,b,
					mintracelength,minscore,
					scorewindowsize,windowminscore,
					maxindelfrac,maxsubstfrac,
					dovetail_orientation,containment_orientation,
					singlechecktype
				);
				
				if ( CORs.valid && CORs.orientation == dovetail_orientation )
				{
					assert ( CORs.nummis + CORs.nummat + CORs.numins <= a.size() );
					assert ( CORs.nummis + CORs.nummat + CORs.numdel <= b.size() );
					assert ( CORs.nummis + CORs.nummat + CORs.numins + CORs.clipa == a.size() );
					assert ( CORs.nummis + CORs.nummat + CORs.numdel + CORs.clipb == b.size() );					
				}
	
				return CORs;
			}

			public:
			static CheckOverlapResult bestOverlap(
				uint64_t const ia, // edgeit->a
				uint64_t const ib, // edgeit->b
				std::string const & a,
				std::string const & ar,
				std::string const & b,
				unsigned int const mintracelength,
				int64_t const minscore,
				uint64_t const scorewindowsize,
				int64_t const windowminscore,
				double const maxindelfrac,
				double const maxsubstfrac,
				CheckOverlapResult ** const CORVa
				)
			{	
				#if 0
				std::cerr << "scorewindowsize=" << scorewindowsize << std::endl;
				std::cerr << "windowminscore=" << windowminscore << std::endl;
				#endif
				
				CheckOverlapResult CORab = 
					checkOverlap(ia,ib,a,b,mintracelength,minscore,
						scorewindowsize, windowminscore,						
						maxindelfrac,maxsubstfrac,
						overlap_a_back_dovetail_b_front,overlap_a_covers_b);
				CheckOverlapResult CORba = 
					checkOverlap(ib,ia,b,a,mintracelength,minscore,
						scorewindowsize, windowminscore,
						maxindelfrac,maxsubstfrac,
						overlap_a_front_dovetail_b_back,overlap_b_covers_a).invert();
						
				if ( 
					CORab.valid && CORba.valid &&
					CORab.orientation == overlap_a_covers_b &&
					CORba.orientation == overlap_b_covers_a )
				{
					CORab.orientation = CORba.orientation = overlap_cover_complete;
				}

				CheckOverlapResult CORarb = 
					checkOverlap(ia,ib,ar,b,mintracelength,minscore,
						scorewindowsize, windowminscore,
						maxindelfrac,maxsubstfrac,
						overlap_a_front_dovetail_b_front,overlap_ar_covers_b);
				CheckOverlapResult CORbar = 
					checkOverlap(ib,ia,b,ar,mintracelength,minscore,
						scorewindowsize, windowminscore,
						maxindelfrac,maxsubstfrac,
						overlap_a_back_dovetail_b_back,overlap_b_covers_ar).invert();

				if ( 
					CORarb.valid && CORbar.valid &&
					CORarb.orientation == overlap_ar_covers_b &&
					CORbar.orientation == overlap_b_covers_ar )
				{
					CORarb.orientation = CORbar.orientation = overlap_cover_complete;
				}


				CheckOverlapResult ** CORVc = CORVa;

				if ( CORab.valid ) *(CORVc++) = & CORab;
				if ( CORba.valid ) *(CORVc++) = & CORba;
				if ( CORarb.valid ) *(CORVc++) = & CORarb;
				if ( CORbar.valid ) *(CORVc++) = & CORbar;
				*(CORVc++) = 0;
				
				CheckOverlapResult * pbestoverlap = 0;
				for ( CheckOverlapResult ** CORVi = CORVa; *CORVi; ++CORVi )
				{
					assert ( (*CORVi)->valid );
					assert ( (*CORVi)->ia == ia );
					assert ( (*CORVi)->ib == ib );
				
					if ( (!pbestoverlap) || (*CORVi)->score > pbestoverlap->score )
						pbestoverlap = *CORVi;
				}

				if ( pbestoverlap != 0 )
				{
					CheckOverlapResult & bestoverlap = *pbestoverlap;
					assert ( bestoverlap.valid );

					if ( 
						isLeftEdge(bestoverlap.orientation)
						||
						isRightEdge(bestoverlap.orientation)
					)
					{
						assert ( bestoverlap.nummis + bestoverlap.nummat + bestoverlap.numins <= a.size() );
						assert ( bestoverlap.nummis + bestoverlap.nummat + bestoverlap.numdel <= b.size() );
						assert ( bestoverlap.nummis + bestoverlap.nummat + bestoverlap.numins + bestoverlap.clipa == a.size() );
						assert ( bestoverlap.nummis + bestoverlap.nummat + bestoverlap.numdel + bestoverlap.clipb == b.size() );					
					}

					#if defined(OCSHOW)
					std::cerr << "---\n";
					switch ( bestoverlap.orientation )
					{
						case overlap_a_back_dovetail_b_front:
							showOverlap(std::cerr,a,b);
							break;
						case overlap_a_front_dovetail_b_back:
							showOverlap(std::cerr,b,a);
							break;
						case overlap_a_front_dovetail_b_front:
							showOverlap(std::cerr,ar,b);
							break;
						case overlap_a_back_dovetail_b_back:
							showOverlap(std::cerr,b,ar);
							break;
						default:
							break;
					}	
					std::cerr << "Orientation " << bestoverlap.orientation << std::endl;
					std::cerr << "---\n\n";
					#endif
	
					return bestoverlap;
				}
				else
				{
					assert ( ! CORab.valid );
					return CORab;
				}
			}

			template<typename reader_type, typename srcreads_container_type, typename edges_array_type>
			static uint64_t processReads(
				typename reader_type::unique_ptr_type & blockreader,
				OverlapComputationBlockRequest const & OCBR,
				edges_array_type & edges,
				srcreads_container_type & srcreads,
				::libmaus2::network::SocketBase * sock,
				::libmaus2::parallel::OMPLock & lock
				)
			{
				// typedef typename reader_type::unique_ptr_type reader_ptr_type;
				typedef typename reader_type::pattern_type pattern_type;
				
				unsigned int const mintracelength = OCBR.mintracelength;
				int64_t const minscore = OCBR.minscore;
				double const maxindelfrac = OCBR.maxindelfrac;
				double const maxsubstfrac = OCBR.maxsubstfrac;
				uint64_t const scorewindowsize = OCBR.scorewindowsize;
				int64_t const windowminscore = OCBR.windowminscore;

				std::cerr << "scorewindowsize=" << scorewindowsize << std::endl;
				std::cerr << "windowminscore=" << windowminscore << std::endl;
								
				pattern_type pattern;
				
				edge_type const * edgeit = edges.begin();
				edge_type * edgeoutit = edges.begin();
				
				uint64_t edgeok = 0;
				uint64_t edgedel = 0;
				uint64_t patproc = 0;
				uint64_t edgeperc = 0;
				while ( blockreader->getNextPatternUnlocked(pattern) )
				{
					while ( edgeit != edges.end() && edgeit->b < pattern.patid )
						++edgeit;
					if ( edgeit == edges.end() )
						break;

					std::string const & b = pattern.spattern;
					std::string const br = ::libmaus2::fastx::reverseComplementUnmapped(b);
					
					CheckOverlapResult * ACORV[5];
					CheckOverlapResult ** const CORVa = &ACORV[0];

					for ( ; edgeit != edges.end() && edgeit->b == pattern.patid; edgeit++ )
					{
						// uint64_t const local = (edgeit->a - OCBR.FI.low);
						// std::string const & b = srcreads[local];
						std::string const a = srcreads[edgeit->a];
						std::string const ar = ::libmaus2::fastx::reverseComplementUnmapped(a);

						edge_type edge;
						CheckOverlapResult const COR = 
							(edgeit->a <= edgeit->b)
							?
								bestOverlap(
									edgeit->a,edgeit->b,a,ar,b,
									mintracelength,minscore,
									scorewindowsize,
									windowminscore,
									maxindelfrac,maxsubstfrac,
									CORVa)
								:
								bestOverlap(
									edgeit->b,edgeit->a,b,br,a,
									mintracelength,minscore,
									scorewindowsize,
									windowminscore,
									maxindelfrac,maxsubstfrac,
									CORVa).invertIncludingOrientation()
							;

						if ( COR.valid )
						{
							edgeok ++;							
							*(edgeoutit++) = COR.toEdge();
							
							if ( sock )
							{
								sock->writeString(
									COR.serialise()
								);
							}
							// std::cerr << "Overlap " << edgeit->a << " -> " << edgeit->b << " " << COR.orientation << std::endl;
						}
						else
						{
							// std::cerr << "No overlap " << edgeit->a << " -> " << edgeit->b << std::endl;
							edgedel ++;
						}
					}
					
					if ( (100*(edgeit-edges.begin()))/edges.size() != edgeperc )
					{
						lock.lock();
						edgeperc = (100*(edgeit-edges.begin()))/edges.size();
						double const bf = (OCBR.blockid+1.0)/static_cast<double>(OCBR.numblocks);
						std::cerr << bf << "," << OCBR.subblockid << "/" << OCBR.numsubblocks << ":" << "Processed " << edgeperc 
							<< "(" << (1.0-(edges.size() ? ((edges.end()-edgeit)/static_cast<double>(edges.size())) : 0.0)) << ")" << std::endl;
						lock.unlock();
					
					}
					
					patproc++;
				}	

				lock.lock();
				double const bf = (OCBR.blockid+1.0)/static_cast<double>(OCBR.numblocks);
				std::cerr << bf << "," << OCBR.subblockid << "/" << OCBR.numsubblocks << ":" << "Processed " << 100 << "(" << 1.0 << ")" << std::endl;
				lock.unlock();
				
				return edgeok;
			}

			template<typename reader_type, typename srcreads_container_type, typename edges_array_type>
			static uint64_t processReadsHamming(
				typename reader_type::unique_ptr_type & blockreader,
				OverlapComputationBlockRequest const & OCBR,
				edges_array_type & edges,
				srcreads_container_type & srcreads,
				::libmaus2::network::SocketBase * sock,
				::libmaus2::parallel::OMPLock & lock
				)
			{
				// typedef typename reader_type::unique_ptr_type reader_ptr_type;
				typedef typename reader_type::pattern_type pattern_type;
				
				libmaus2::lcs::HammingOverlapDetection HOD;
				
				unsigned int const mintracelength = OCBR.mintracelength;
				int64_t const minscore = OCBR.minscore;
				// double const maxindelfrac = OCBR.maxindelfrac;
				double const maxsubstfrac = OCBR.maxsubstfrac;
				uint64_t const scorewindowsize = OCBR.scorewindowsize;
				int64_t const windowminscore = OCBR.windowminscore;
				
				uint64_t const maxsubstperc = std::ceil(maxsubstfrac * 100.0);

				std::cerr << "scorewindowsize=" << scorewindowsize << std::endl;
				std::cerr << "windowminscore=" << windowminscore << std::endl;
								
				pattern_type pattern;
				
				edge_type const * edgeit = edges.begin();
				edge_type * edgeoutit = edges.begin();
				
				uint64_t edgeok = 0;
				uint64_t edgedel = 0;
				uint64_t patproc = 0;
				uint64_t edgeperc = 0;
				while ( blockreader->getNextPatternUnlocked(pattern) )
				{
					while ( edgeit != edges.end() && edgeit->b < pattern.patid )
						++edgeit;
					if ( edgeit == edges.end() )
						break;

					std::string const & b = pattern.spattern;

					for ( ; edgeit != edges.end() && edgeit->b == pattern.patid; edgeit++ )
					{
						std::string const a = srcreads[edgeit->a];

						libmaus2::lcs::OverlapOrientation::overlap_orientation orientation;
						uint64_t overhang;
						int64_t maxscore;
						
						if ( 
							HOD.detect(a,b,maxsubstperc,orientation,overhang,maxscore) 
							&&
							// check length of trace
							((b.size() - overhang) >= mintracelength)
							&&
							// check score
							(maxscore >= minscore)
						)
						{
							edgeok ++;							
							*(edgeoutit++) = edge_type(edgeit->a,edgeit->b,addOrientation(overhang,orientation));

							if ( sock )
							{
							}
							// std::cerr << "Overlap " << edgeit->a << " -> " << edgeit->b << " " << COR.orientation << std::endl;
						}
						else
						{
							// std::cerr << "No overlap " << edgeit->a << " -> " << edgeit->b << std::endl;
							edgedel ++;
						}
					}
					
					if ( (100*(edgeit-edges.begin()))/edges.size() != edgeperc )
					{
						lock.lock();
						edgeperc = (100*(edgeit-edges.begin()))/edges.size();
						double const bf = (OCBR.blockid+1.0)/static_cast<double>(OCBR.numblocks);
						std::cerr << bf << "," << OCBR.subblockid << "/" << OCBR.numsubblocks << ":" << "Processed " << edgeperc 
							<< "(" << (1.0-(edges.size() ? ((edges.end()-edgeit)/static_cast<double>(edges.size())) : 0.0)) << ")" << std::endl;
						lock.unlock();
					
					}
					
					patproc++;
				}	

				lock.lock();
				double const bf = (OCBR.blockid+1.0)/static_cast<double>(OCBR.numblocks);
				std::cerr << bf << "," << OCBR.subblockid << "/" << OCBR.numsubblocks << ":" << "Processed " << 100 << "(" << 1.0 << ")" << std::endl;
				lock.unlock();
				
				return edgeok;
			}
			
			#if ! defined(_WIN32)
			template<typename edges_array_type>
			static uint64_t handleBlock(
				OverlapComputationBlockRequest const & OCBR,
				::libmaus2::fastx::CompactReadContainer & srcreads,
				edges_array_type & edges,
				::libmaus2::network::SocketBase * sock,
				::libmaus2::parallel::OMPLock & lock
			)
			{
				#if 1
				lock.lock();
				std::cerr << OCBR.blockid << "," << OCBR.subblockid << ":" << "Handle block " << OCBR.FI << std::endl;
				lock.unlock();
				#endif

				// src,target

				#if 1
				lock.lock();
				std::cerr << OCBR.blockid << "," << OCBR.subblockid << ":" << "Sorting edge vector for " << OCBR.FI << "," << OCBR.SUBFI << std::endl;
				lock.unlock();
				#endif
				std::sort ( edges.begin(), edges.end(), SecondComponentComparator());
				#if 1
				lock.lock();
				std::cerr << OCBR.blockid << "," << OCBR.subblockid << ":" << "Sorted edge vector for " << OCBR.FI << "," << OCBR.SUBFI << std::endl;
				lock.unlock();
				#endif
				
				uint64_t edgeok;
				if ( OCBR.serverport == 0 )
				{
					#if defined(UNCOMPRESSED)
					if ( OCBR.isfastq )
					{
						typedef ::libmaus2::fastx::FastQReader reader_type;
						typedef reader_type::unique_ptr_type reader_ptr_type;
						reader_ptr_type blockreader ( new reader_type (OCBR.inputfiles,OCBR.SUBFI) );
						// edgeok = processReads<reader_type>(blockreader,OCBR,edges,srcreads,sock,lock);
						edgeok = processReadsHamming<reader_type>(blockreader,OCBR,edges,srcreads,sock,lock);
					}
					else
					{
						typedef ::libmaus2::fastx::FastAReader reader_type;
						typedef reader_type::unique_ptr_type reader_ptr_type;
						reader_ptr_type blockreader ( new reader_type (OCBR.inputfiles,OCBR.SUBFI) );
						// edgeok = processReads<reader_type>(blockreader,OCBR,edges,srcreads,sock,lock);					
						edgeok = processReadsHamming<reader_type>(blockreader,OCBR,edges,srcreads,sock,lock);					
					}
					#else
					typedef ::libmaus2::fastx::CompactFastConcatDecoder reader_type;
					typedef reader_type::unique_ptr_type reader_ptr_type;
					reader_ptr_type blockreader ( new reader_type (OCBR.inputfiles,OCBR.SUBFI) );
					// edgeok = processReads<reader_type>(blockreader,OCBR,edges,srcreads,sock,lock);					
					edgeok = processReadsHamming<reader_type>(blockreader,OCBR,edges,srcreads,sock,lock);					
					#endif
				}
				else
				{
					#if defined(UNCOMPRESSED)
					if ( OCBR.isfastq )
					{
						typedef ::libmaus2::fastx::FastQClient reader_type;
						typedef reader_type::unique_ptr_type reader_ptr_type;
						std::cerr << "Using host " << OCBR.serverhostname << " port " << OCBR.serverport << std::endl;
						reader_ptr_type blockreader ( new reader_type (
							OCBR.serverport,
							OCBR.serverhostname,
							OCBR.sublo,
							OCBR.subblockid) );
						// edgeok = processReads<reader_type>(blockreader,OCBR,edges,srcreads,sock,lock);
						edgeok = processReadsHamming<reader_type>(blockreader,OCBR,edges,srcreads,sock,lock);
					}
					else
					{
						typedef ::libmaus2::fastx::FastAClient reader_type;
						typedef reader_type::unique_ptr_type reader_ptr_type;
						std::cerr << "Using host " << OCBR.serverhostname << " port " << OCBR.serverport << std::endl;
						reader_ptr_type blockreader ( new reader_type (
							OCBR.serverport,
							OCBR.serverhostname,
							OCBR.sublo,
							OCBR.subblockid) );
						// edgeok = processReads<reader_type>(blockreader,OCBR,edges,srcreads,sock,lock);
						edgeok = processReadsHamming<reader_type>(blockreader,OCBR,edges,srcreads,sock,lock);
					}
					#else
					typedef ::libmaus2::fastx::FastCClient reader_type;
					typedef reader_type::unique_ptr_type reader_ptr_type;
					std::cerr << "Using host " << OCBR.serverhostname << " port " << OCBR.serverport << std::endl;
					reader_ptr_type blockreader ( new reader_type (
						OCBR.serverport,
						OCBR.serverhostname,
						OCBR.sublo,
						OCBR.subblockid) );
					// edgeok = processReads<reader_type>(blockreader,OCBR,edges,srcreads,sock,lock);
					edgeok = processReadsHamming<reader_type>(blockreader,OCBR,edges,srcreads,sock,lock);
					#endif
				}
				
				#if 1
				lock.lock();
				std::cerr << OCBR.blockid << "," << OCBR.subblockid << ":" << "Resorting edge vector for " << OCBR.FI << "," << OCBR.SUBFI << std::endl;
				lock.unlock();
				#endif
				std::sort (edges.begin(),edges.begin() + edgeok);
				#if 1
				lock.lock();
				std::cerr << OCBR.blockid << "," << OCBR.subblockid << ":" << "Resorted edge vector for " << OCBR.FI << "," << OCBR.SUBFI << std::endl;
				lock.unlock();
				#endif
				
				return edgeok;
			}
			#endif
		};
	}
}
#endif
