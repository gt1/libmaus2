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

#if !defined(LIBMAUS2_LCS_DALIGNERLOCALALIGNMENT_HPP)
#define LIBMAUS2_LCS_DALIGNERLOCALALIGNMENT_HPP

#include <libmaus2/lcs/LocalEditDistanceResult.hpp>
#include <libmaus2/lcs/LocalEditDistanceTraceContainer.hpp>
#include <libmaus2/dazzler/align/Overlap.hpp>
#include <libmaus2/lcs/DalignerNP.hpp>

#if defined(LIBMAUS2_HAVE_DALIGNER)
#include <align.h>
#endif

namespace libmaus2
{
	namespace lcs
	{
		struct DalignerLocalAlignment : public EditDistanceTraceContainer
		{
			typedef DalignerLocalAlignment this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef LocalEditDistanceResult result_type;

			#if defined(LIBMAUS2_HAVE_DALIGNER)
			Align_Spec * spec;
			Work_Data * workdata;
			Alignment align;
			Overlap OVL;
			DalignerNP NP;
			
			libmaus2::autoarray::AutoArray<char> A;
			libmaus2::autoarray::AutoArray<char> B;
			#endif

			void cleanup()
			{
				#if defined(LIBMAUS2_HAVE_DALIGNER)
				if ( workdata )
				{
					Free_Work_Data(workdata);
					workdata = 0;
				}
				if ( spec )
				{
					Free_Align_Spec(spec);
					spec = 0;
				}
				#endif
			}
		
			public:
			DalignerLocalAlignment(
				double const correlation = 0.70,
				int64_t const tspace = 100,
				float const afreq = 0.25,
				float const cfreq = 0.25,
				float const gfreq = 0.25,
				float const tfreq = 0.25
			) : spec(0), workdata(0)
			{
				#if defined(LIBMAUS2_HAVE_DALIGNER)
			        float freq[] = { afreq, cfreq, gfreq, tfreq };
			        spec = ::New_Align_Spec(correlation,tspace,&freq[0]);
			        
			        if ( ! spec )
			        {
			        	cleanup();
			        	libmaus2::exception::LibMausException lme;
			        	lme.getStream() << "DalignerLocalAlignment: failed to allocate spec object via New_Align_Spec" << std::endl;
			        	lme.finish();
			        	throw lme;
			        }

			        workdata = ::New_Work_Data();
			        if ( ! workdata )
			        {
			        	cleanup();
			        	libmaus2::exception::LibMausException lme;
			        	lme.getStream() << "DalignerLocalAlignment: failed to allocate work data object via New_Work_Data" << std::endl;
			        	lme.finish();
			        	throw lme;
			        }
			        #else
		        	libmaus2::exception::LibMausException lme;
		        	lme.getStream() << "DalignerLocalAlignment: libmaus2 is compiled without DALIGNER support" << std::endl;
		        	lme.finish();
		        	throw lme;			        
			        #endif
			}
			
			LocalEditDistanceResult process(uint8_t const * a, uint64_t const n, uint8_t const * b, uint64_t const m)
			{
				#if defined(LIBMAUS2_HAVE_DALIGNER)
				assert ( spec );
				assert ( workdata );
				
				if ( (n+2) > A.size() )
					A.resize(n+2);
				if ( (m+2) > B.size() )
					B.resize(m+2);
				
				// text	
				std::copy(a,a+n,A.begin()+1);
				// terminators in front and back
				A[0] = 4;
				A[n+1] = 4;
				
				// text
				std::copy(b,b+m,B.begin()+1);
				// terminators in front and back
				B[0] = 4;
				A[m+1] = 4;
				
				::std::memset(&align,0,sizeof(align));
				::std::memset(&OVL,0,sizeof(OVL));
					
				align.bseq = A.begin()+1;
				align.aseq = B.begin()+1;
				align.blen = n;
				align.alen = m;				
				align.path = &(OVL.path);

				// compute the trace points
				Path * path = Local_Alignment(&align,workdata,spec,0,0,0,-1,-1);
				
				// compute dense alignment	
				Compute_Trace_PTS(&align,workdata,Trace_Spacing(spec));

				// check for output size
				if ( capacity() < n + m )
					resize(n + m);
					
				ta = trace.begin();
				te = trace.begin();

				// uint64_t const nused = path->aepos - path->abpos;
				uint64_t const mused = path->bepos - path->bbpos;

				// extract edit operations
				int const * trace = reinterpret_cast<int const *>(align.path->trace);
				int i = 1; // on B, MAT,MIS,INS
				int j = 1; // on A, MAT,MIS,DEL
				char const * tp = B.begin() + path->abpos;
				char const * qp = A.begin() + path->bbpos;
				uint64_t nummat = 0, nummis = 0, numins = 0, numdel = 0;
				for ( int k = 0; k < align.path->tlen; ++k )
				{
					if ( trace[k] < 0 )
					{
						int p = -trace[k];
						while ( i < p )
						{
							char const tc = tp[i++];
							char const qc = qp[j++];
							bool const eq = tc == qc;
							
							if ( eq )
							{
								*(te++)	= libmaus2::lcs::BaseConstants::STEP_MATCH;		
								nummat += 1;
							}
							else
							{
								*(te++)	= libmaus2::lcs::BaseConstants::STEP_MISMATCH;						
								nummis += 1;
							}
						}

						#if 0						
						*(te++)	= libmaus2::lcs::BaseConstants::STEP_INS;
						numins += 1;
						#else
						*(te++)	= libmaus2::lcs::BaseConstants::STEP_DEL;
						numdel += 1;						
						#endif
						++j;
					}
					else
					{
						int p = trace[k];
						while ( j < p )
						{
							char const tc = tp[i++];
							char const qc = qp[j++];
							bool const eq = tc == qc;
							
							if ( eq )
							{
								*(te++)	= libmaus2::lcs::BaseConstants::STEP_MATCH;		
								nummat += 1;
							}
							else
							{
								*(te++)	= libmaus2::lcs::BaseConstants::STEP_MISMATCH;						
								nummis += 1;
							}
						}
						#if 0
						*(te++)	= libmaus2::lcs::BaseConstants::STEP_DEL;
						numdel += 1;
						#else
						*(te++)	= libmaus2::lcs::BaseConstants::STEP_INS;
						numins += 1;						
						#endif
						++i;
					}
				}
				while ( i <= static_cast<int>(mused) )
				{
					char const tc = tp[i++];
					char const qc = qp[j++];
					bool const eq = tc == qc;

					if ( eq )
					{
						*(te++)	= libmaus2::lcs::BaseConstants::STEP_MATCH;
						nummat += 1;
					}
					else
					{
						*(te++)	= libmaus2::lcs::BaseConstants::STEP_MISMATCH;
						nummis += 1;
					}
				}
				
				std::pair<uint64_t,uint64_t> PP = prefixPositive(1,1,1,1);
				std::pair<uint64_t,uint64_t> PS = suffixPositive(1,1,1,1);
				
				AlignmentStatistics const AS = getAlignmentStatistics();
				
				// return counts
				return LocalEditDistanceResult(
					AS.insertions,AS.deletions,AS.matches,AS.mismatches,
					path->abpos + PP.first,
					n-path->aepos + PS.first,
					path->bbpos + PP.second,
					m-path->bepos + PS.second
				);
				#else
				libmaus2::exception::LibMausException lme;
				lme.getStream() << "DalignerLocalAlignment: libmaus2 is compiled without DALIGNER support" << std::endl;
				lme.finish();
				throw lme;			        
				#endif
			}	
		};
	}
}
#endif
