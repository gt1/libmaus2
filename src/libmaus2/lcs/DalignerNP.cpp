/*
    libmaus2
    Copyright (C) 2015 German Tischler

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
#include <libmaus2/lcs/DalignerNP.hpp>
#include <libmaus2/LibMausConfig.hpp>
#include <libmaus2/fastx/acgtnMap.hpp>

#if defined(LIBMAUS2_HAVE_DALIGNER)
#include <align.h>
#endif

libmaus2::lcs::DalignerNP::DalignerNP()
#if defined(LIBMAUS2_HAVE_DALIGNER)
: data(0)
#endif
{
	#if defined(LIBMAUS2_HAVE_DALIGNER)
	data = New_Work_Data();

	if ( ! data )
	{
		::libmaus2::exception::LibMausException se;
		se.getStream() << "libmaus2::lcs::DalignerNP::DalignerNP(): New_Work_Data() failed." << std::endl;
		se.finish();
		throw se;
	}
	#else
	libmaus2::exception::LibMausException lme;
	lme.getStream() << "libmaus2::lcs::DalignerNP::DalignerNP: support for daligner is not present" << std::endl;
	lme.finish();
	throw lme;
	#endif
}
libmaus2::lcs::DalignerNP::~DalignerNP()
{
	#if defined(LIBMAUS2_HAVE_DALIGNER)
	Free_Work_Data(reinterpret_cast<Work_Data *>(data));
	#endif

}
void libmaus2::lcs::DalignerNP::align(
	#if defined(LIBMAUS2_HAVE_DALIGNER)
	uint8_t const * a,
	size_t const l_a,
	uint8_t const * b,
	size_t const l_b
	#else
	uint8_t const *,
	size_t const,
	uint8_t const *,
	size_t const
	#endif
)
{
	#if defined(LIBMAUS2_HAVE_DALIGNER)
	if ( l_a > A.size() )
		A.resize(l_a);
	if ( l_b > B.size() )
		B.resize(l_b);

	for ( size_t i = 0; i < l_a; ++i )
		A[i] = libmaus2::fastx::mapChar(a[i]);
	for ( size_t i = 0; i < l_b; ++i )
		B[i] = libmaus2::fastx::mapChar(b[i]);

	Path path;
	path.trace = NULL;
	path.tlen = 0;
	path.diffs = l_a+l_b;
	path.abpos = 0;
	path.bbpos = 0;
	path.aepos = l_a;
	path.bepos = l_b;

	Alignment algn;
	algn.path = &path;
	algn.flags = 0;
	algn.aseq = A.begin();
	algn.bseq = B.begin();
	algn.alen = l_a;
	algn.blen = l_b;

	Compute_Trace_ALL(&algn,reinterpret_cast<Work_Data *>(data));

	if ( capacity() < l_a+l_b )
		resize(l_a+l_b);

	ta = trace.begin();
	te = trace.begin();

	int const * trace = reinterpret_cast<int const *>(algn.path->trace);
	int i = 1;
	int j = 1;
	char const * tp = A.begin()-1;
	char const * qp = B.begin()-1;
	for ( int k = 0; k < algn.path->tlen; ++k )
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
				}
				else
				{
					*(te++)	= libmaus2::lcs::BaseConstants::STEP_MISMATCH;
				}
			}

			*(te++)	= libmaus2::lcs::BaseConstants::STEP_INS;
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
				}
				else
				{
					*(te++)	= libmaus2::lcs::BaseConstants::STEP_MISMATCH;
				}
			}
			*(te++)	= libmaus2::lcs::BaseConstants::STEP_DEL;
			++i;
		}
	}
	while ( i <= static_cast<int>(l_a) )
	{
		char const tc = tp[i++];
		char const qc = qp[j++];
		bool const eq = tc == qc;

		if ( eq )
		{
			*(te++)	= libmaus2::lcs::BaseConstants::STEP_MATCH;
		}
		else
		{
			*(te++)	= libmaus2::lcs::BaseConstants::STEP_MISMATCH;
		}
	}
	#endif
}
libmaus2::lcs::AlignmentTraceContainer const & libmaus2::lcs::DalignerNP::getTraceContainer() const
{
	return *this;
}
