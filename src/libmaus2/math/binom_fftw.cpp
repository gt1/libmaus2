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
#include <libmaus2/math/binom.hpp>
#include <complex>
#include <cassert>
#include <libmaus2/math/ipow.hpp>

#if defined(LIBMAUS2_HAVE_FFTW)
#include <fftw3.h>
#endif

#if defined(LIBMAUS2_HAVE_FFTW)
struct FFTWMemBlock
{
	fftw_complex * A;

	static fftw_complex * allocate(size_t const fftn)
	{
		fftw_complex * A = reinterpret_cast<fftw_complex *>(fftw_malloc(fftn * sizeof(fftw_complex)));
		if ( A )
			return A;
		else
			throw std::bad_alloc();
	}

	FFTWMemBlock(size_t const fftn)
	: A(allocate(fftn))
	{
	}

	~FFTWMemBlock()
	{
		fftw_free(A);
	}
};
#endif

std::vector < libmaus2::math::GmpFloat > libmaus2::math::Binom::multiDimBinomialFFT(
	double const
		#if defined(LIBMAUS2_HAVE_FFTW)
		avg
		#endif
		,
	double const
		#if defined(LIBMAUS2_HAVE_FFTW)
		coverage
		#endif
		,
	uint64_t const
		#if defined(LIBMAUS2_HAVE_FFTW)
		d
		#endif
)
{
	#if defined(LIBMAUS2_HAVE_FFTW)
	std::vector < libmaus2::math::GmpFloat > const BV = libmaus2::math::Binom::binomVector(avg,coverage,512);

	uint64_t const fftn = coverage*d+1;

	FFTWMemBlock CCin(fftn);
	FFTWMemBlock CCout(fftn);
	FFTWMemBlock CCtmp(fftn);

	fftw_plan fftwforwplan = fftw_plan_dft_1d(fftn,CCin.A,CCout.A,FFTW_FORWARD,0);
	fftw_plan fftwbackplan = fftw_plan_dft_1d(fftn,CCtmp.A,CCin.A,FFTW_BACKWARD,0);

	for ( uint64_t i = 0; i < fftn; ++i )
		CCin.A[i][0] = CCin.A[i][1] = 0.0;

	assert ( fftn >= BV.size() );
	for ( uint64_t i = 0; i < BV.size(); ++i )
		CCin.A[i][0] = BV[i];

	fftw_execute(fftwforwplan);

	for ( uint64_t i = 0; i < fftn; ++i )
	{
		std::complex<double> C(CCout.A[i][0],CCout.A[i][1]);
		C = libmaus2::math::gpow(C,d);
		CCtmp.A[i][0] = C.real();
		CCtmp.A[i][1] = C.imag();
	}

	fftw_execute(fftwbackplan);

	fftw_destroy_plan(fftwforwplan);
	fftw_destroy_plan(fftwbackplan);

	std::vector < libmaus2::math::GmpFloat > R(fftn);
	for ( uint64_t i = 0; i < fftn; ++i )
	{
		R[i] = CCin.A[i][0]/fftn;
		if ( static_cast<double>(R[i]) < 1e-7 )
			R[i] = 0.0;
	}

	return R;
	#else
	libmaus2::exception::LibMausException lme;
	lme.getStream() << "[E] libmaus2::math::Binom::multiDimBinomialFFT: libmaus2 is built without fftw support" << std::endl;
	lme.finish();
	throw lme;
	#endif
}
