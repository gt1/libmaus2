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
#include <libmaus2/lcs/DalignerLocalAlignment.hpp>
#include <libmaus2/lcs/LocalEditDistanceTraceContainer.hpp>
#include <libmaus2/dazzler/align/Overlap.hpp>
#include <libmaus2/lcs/DalignerNP.hpp>

#if defined(LIBMAUS2_HAVE_DALIGNER)
#include <align.h>
#endif

struct DalignerData
{
	#if defined(LIBMAUS2_HAVE_DALIGNER)
	Align_Spec * spec;
	Work_Data * workdata;
	::Alignment align;
	::Overlap OVL;
	::Path path;
	libmaus2::lcs::DalignerNP NP;
	libmaus2::autoarray::AutoArray<int16_t> Apath;
	#endif

	DalignerData(
		#if defined(LIBMAUS2_HAVE_DALIGNER)
		double const correlation,
		int64_t const tspace,
		float const afreq,
		float const cfreq,
		float const gfreq,
		float const tfreq
		#else
		double const,
		int64_t const,
		float const,
		float const,
		float const,
		float const
		#endif
	)
	#if defined(LIBMAUS2_HAVE_DALIGNER)
	: spec(0), workdata(0)
	#endif
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

	~DalignerData()
	{
		cleanup();
	}
};

libmaus2::lcs::DalignerLocalAlignment::DalignerLocalAlignment(
	#if defined(LIBMAUS2_HAVE_DALIGNER)
	double const correlation,
	int64_t const tspace,
	float const afreq,
	float const cfreq,
	float const gfreq,
	float const tfreq
	#else
	double const,
	int64_t const,
	float const,
	float const,
	float const,
	float const
	#endif
)
{
	#if defined(LIBMAUS2_HAVE_DALIGNER)
	data = new DalignerData(correlation,tspace,afreq,cfreq,gfreq,tfreq);
	#else
	libmaus2::exception::LibMausException lme;
	lme.getStream() << "DalignerLocalAlignment: libmaus2 is compiled without DALIGNER support" << std::endl;
	lme.finish();
	throw lme;
	#endif
}

libmaus2::lcs::DalignerLocalAlignment::~DalignerLocalAlignment()
{
	#if defined(LIBMAUS2_HAVE_DALIGNER)
	delete reinterpret_cast<DalignerData *>(data);
	#endif
}

libmaus2::lcs::LocalEditDistanceResult libmaus2::lcs::DalignerLocalAlignment::process(
	#if defined(LIBMAUS2_HAVE_DALIGNER)
	uint8_t const * a, uint64_t const n, uint64_t const seedposa, uint8_t const * b, uint64_t const m, uint64_t const seedposb
	#else
	uint8_t const *, uint64_t const, uint64_t const, uint8_t const *, uint64_t const, uint64_t const
	#endif
)
{
	#if defined(LIBMAUS2_HAVE_DALIGNER)
	DalignerData * dataobject = reinterpret_cast<DalignerData *>(data);
	assert ( dataobject->spec );
	assert ( dataobject->spec );

	bool const selfie = (a==b) && (n==m);

	libmaus2::autoarray::AutoArray<char> & A = CA;
	libmaus2::autoarray::AutoArray<char> & B = selfie ? CA : CB;

	if ( (n+2) > A.size() )
		A.resize(n+2);
	if ( (m+2) > B.size() )
		B.resize(m+2);

	// text
	std::copy(a,a+n,A.begin()+1);
	// terminators in front and back
	A[0] = 4;
	A[n+1] = 4;

	if ( ! selfie )
	{
		// text
		std::copy(b,b+m,B.begin()+1);
		// terminators in front and back
		B[0] = 4;
		B[m+1] = 4;
	}

	::std::memset(&(dataobject->align),0,sizeof(dataobject->align));
	::std::memset(&(dataobject->OVL),0,sizeof(dataobject->OVL));

	dataobject->align.bseq = A.begin()+1;
	dataobject->align.aseq = B.begin()+1;
	dataobject->align.blen = n;
	dataobject->align.alen = m;
	dataobject->align.path = &(dataobject->OVL.path);

	// compute the trace points
	Local_Alignment(
		&(dataobject->align),
		dataobject->workdata,
		dataobject->spec,
		static_cast<int64_t>(seedposb)-static_cast<int64_t>(seedposa),
		static_cast<int64_t>(seedposb)-static_cast<int64_t>(seedposa),
		seedposb+seedposa /* anti diagonal */,-1,-1);

	// compute dense dataobject->alignment
	Compute_Trace_PTS(&(dataobject->align),dataobject->workdata,Trace_Spacing(dataobject->spec),GREEDIEST);

	// check for output size
	if ( EditDistanceTraceContainer::capacity() < n + m )
		resize(n + m);

	ta = trace.begin();
	te = trace.begin();

	// extract edit operations
	Path *npath = dataobject->align.path;
	int const tlen = dataobject->align.path->tlen;
	int const * trace = reinterpret_cast<int const *>(npath->trace);
	int i = npath->abpos + 1;
	int j = npath->bbpos + 1;
	uint8_t const * tp = b - 1;
	uint8_t const * qp = a - 1;
	uint64_t nummat = 0, nummis = 0, numins = 0, numdel = 0;

	for ( int k = 0; k < tlen; ++k )
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

			*(te++)	= libmaus2::lcs::BaseConstants::STEP_DEL;
			numdel += 1;
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
			*(te++)	= libmaus2::lcs::BaseConstants::STEP_INS;
			numins += 1;
			++i;
		}
	}

	while ( i <= static_cast<int>(npath->aepos) )
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

	AlignmentStatistics const AS = getAlignmentStatistics();

	// return counts
	return LocalEditDistanceResult(
		AS.insertions,AS.deletions,AS.matches,AS.mismatches,
		// front clipping on a
		npath->bbpos,
		// back clipping on a
		n-npath->bepos,
		// front clipping on b
		npath->abpos,
		// back clipping on b
		m-npath->aepos
	);
	#else
	libmaus2::exception::LibMausException lme;
	lme.getStream() << "DalignerLocalAlignment: libmaus2 is compiled without DALIGNER support" << std::endl;
	lme.finish();
	throw lme;
	#endif
}

libmaus2::lcs::LocalEditDistanceResult libmaus2::lcs::DalignerLocalAlignment::computeDenseTrace(
	#if defined(LIBMAUS2_HAVE_DALIGNER)
	uint8_t const * a, uint64_t const n, uint8_t const * b, uint64_t const m, int64_t const tspace, std::pair<uint16_t,uint16_t> const * intrace, uint64_t const tracelen,
	int32_t const diffs,
	int32_t const abpos,
	int32_t const bbpos,
	int32_t const aepos,
	int32_t const bepos
	#else
	uint8_t const *, uint64_t const, uint8_t const *, uint64_t const, int64_t const, std::pair<uint16_t,uint16_t> const *, uint64_t const,
	int32_t const,
	int32_t const,
	int32_t const,
	int32_t const,
	int32_t const
	#endif
)
{
	#if defined(LIBMAUS2_HAVE_DALIGNER)
	DalignerData * dataobject = reinterpret_cast<DalignerData *>(data);
	assert ( dataobject->spec );

	bool const selfie = (a==b) && (n==m);

	libmaus2::autoarray::AutoArray<char> & A = CA;
	libmaus2::autoarray::AutoArray<char> & B = selfie ? CA : CB;

	if ( (n+2) > A.size() )
		A.resize(n+2);
	if ( (m+2) > B.size() )
		B.resize(m+2);

	// text
	std::copy(a,a+n,A.begin()+1);
	// terminators in front and back
	A[0] = 4;
	A[n+1] = 4;

	if ( ! selfie )
	{
		// text
		std::copy(b,b+m,B.begin()+1);
		// terminators in front and back
		B[0] = 4;
		B[m+1] = 4;
	}

	// erase align object
	::std::memset(&(dataobject->align),0,sizeof(dataobject->align));

	// copy trace data
	if ( dataobject->Apath.size() < 2*tracelen )
		dataobject->Apath.resize(2*tracelen);
	for ( uint64_t i = 0; i < tracelen; ++i )
	{
		dataobject->Apath[2*i+0] = intrace[i].first;
		dataobject->Apath[2*i+1] = intrace[i].second;
	}

	dataobject->align.aseq = A.begin()+1;
	dataobject->align.bseq = B.begin()+1;
	dataobject->align.alen = n;
	dataobject->align.blen = m;
	dataobject->align.path = &(dataobject->path);
	dataobject->path.diffs = diffs;
	dataobject->path.abpos = abpos;
	dataobject->path.bbpos = bbpos;
	dataobject->path.aepos = aepos;
	dataobject->path.bepos = bepos;
	dataobject->path.trace = dataobject->Apath.begin();
	dataobject->path.tlen  = 2*tracelen;

	// compute dense alignment trace
	Compute_Trace_PTS(&(dataobject->align),dataobject->workdata,tspace,GREEDIEST);

	// check for output size
	if ( EditDistanceTraceContainer::capacity() < n + m )
		resize(n + m);

	ta = trace.begin();
	te = trace.begin();

	// extract edit operations
	Path *npath    = &(dataobject->path);
	int const tlen = npath->tlen;
	int const * trace = reinterpret_cast<int const *>(npath->trace);
	int i = npath->abpos + 1;
	int j = npath->bbpos + 1;
	uint8_t const * tp = a - 1;
	uint8_t const * qp = b - 1;
	uint64_t nummat = 0, nummis = 0, numins = 0, numdel = 0;

	for ( int k = 0; k < tlen; ++k )
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

			*(te++)	= libmaus2::lcs::BaseConstants::STEP_INS;
			numdel += 1;
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
			*(te++)	= libmaus2::lcs::BaseConstants::STEP_DEL;
			numins += 1;
			++i;
		}
	}

	while ( i <= static_cast<int>(npath->aepos) )
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

	AlignmentStatistics const AS = getAlignmentStatistics();

	// return counts
	return LocalEditDistanceResult(
		AS.insertions,AS.deletions,AS.matches,AS.mismatches,
		// front clipping on a
		npath->abpos,
		// back clipping on a
		n-npath->aepos,
		// front clipping on b
		npath->bbpos,
		// back clipping on b
		m-npath->bepos
	);
	#else
	libmaus2::exception::LibMausException lme;
	lme.getStream() << "DalignerLocalAlignment: libmaus2 is compiled without DALIGNER support" << std::endl;
	lme.finish();
	throw lme;
	#endif

}
