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
#include <libmaus2/math/Convolution.hpp>

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
	uint64_t const fftn;
	fftw_complex * A;

	static fftw_complex * allocate(size_t const fftn)
	{
		fftw_complex * A = reinterpret_cast<fftw_complex *>(fftw_malloc(fftn * sizeof(fftw_complex)));
		if ( A )
			return A;
		else
			throw std::bad_alloc();
	}

	FFTWMemBlock(size_t const rfftn, bool const rerase = true)
	: fftn(rfftn), A(allocate(fftn))
	{
		if ( rerase )
			erase();
	}

	void erase()
	{
		for ( uint64_t i = 0; i < fftn; ++i )
			A[i][0] = A[i][1] = 0.0;
	}

	~FFTWMemBlock()
	{
		fftw_free(A);
	}
};
#endif

std::vector<double> libmaus2::math::Convolution::convolutionFFTW(
	std::vector<double> const &
		#if defined(LIBMAUS2_HAVE_FFTW)
		RA
		#endif
		,
	std::vector<double> const &
		#if defined(LIBMAUS2_HAVE_FFTW)
		RB
		#endif
)
{
	#if defined(LIBMAUS2_HAVE_FFTW)
	uint64_t const ra = RA.size();
	uint64_t const rb = RB.size();

	if ( ra*rb == 0 )
		return std::vector<double>(0);

	uint64_t const fftn = (ra+rb-1);

	FFTWMemBlock CCin_A(fftn);
	FFTWMemBlock CCin_B(fftn);
	FFTWMemBlock CCtmp_A(fftn);
	FFTWMemBlock CCtmp_B(fftn);
	FFTWMemBlock CCtmp_C(fftn);
	FFTWMemBlock CCout(fftn);

	fftw_plan fftwforwAplan = fftw_plan_dft_1d(fftn,CCin_A.A,CCtmp_A.A,FFTW_FORWARD,0);
	fftw_plan fftwforwBplan = fftw_plan_dft_1d(fftn,CCin_B.A,CCtmp_B.A,FFTW_FORWARD,0);
	fftw_plan fftwbackplan  = fftw_plan_dft_1d(fftn,CCtmp_C.A,CCout.A,FFTW_BACKWARD,0);

	for ( uint64_t i = 0; i < RA.size(); ++i )
		CCin_A.A[i][0] = RA[i];
	for ( uint64_t i = 0; i < RB.size(); ++i )
		CCin_B.A[i][0] = RB[i];

	fftw_execute(fftwforwAplan);
	fftw_execute(fftwforwBplan);

	for ( uint64_t i = 0; i < fftn; ++i )
	{
		std::complex<double> CA(CCtmp_A.A[i][0],CCtmp_A.A[i][1]);
		std::complex<double> CB(CCtmp_B.A[i][0],CCtmp_B.A[i][1]);
		std::complex<double> CC = CA * CB;
		CCtmp_C.A[i][0] = CC.real();
		CCtmp_C.A[i][1] = CC.imag();
	}

	fftw_execute(fftwbackplan);

	fftw_destroy_plan(fftwforwAplan);
	fftw_destroy_plan(fftwforwBplan);
	fftw_destroy_plan(fftwbackplan);

	std::vector<double> R(fftn);
	for ( uint64_t i = 0; i < fftn; ++i )
		R[i] = CCout.A[i][0] / fftn;

	return R;
	#else
	libmaus2::exception::LibMausException lme;
	lme.getStream() << "[E] libmaus2::math::Convolution::convolutionFFTW: libmaus2 is built without fftw support" << std::endl;
	lme.finish();
	throw lme;
	#endif
}
