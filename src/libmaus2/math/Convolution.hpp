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
#if ! defined(LIBMAUS2_MATH_CONVOLUTION_HPP)
#define LIBMAUS2_MATH_CONVOLUTION_HPP

#include <vector>
#include <cassert>
#include <numeric>
#include <libmaus2/types/types.hpp>

namespace libmaus2
{
	namespace math
	{
		struct Convolution
		{
			template<typename type>
			static std::vector<type> convolution(
				std::vector<type> const & A,
				std::vector<type> const & B
			)
			{
				// max value A.size()-1 + B.size() - 1 = A.size()+B.size()-2
				std::vector<type> R((A.size() * B.size()) ? (A.size()+B.size()-1) : 0);

				for ( uint64_t i = 0; i < R.size(); ++i )
				{
					type s = 0.0;

					// j >= 0
					// i-j < B.size() -> i-j+1 <= B.size() -> i-B.size()+1 <= j -> j >= i-B.size()+1

					uint64_t const start = (i >= B.size()-1) ? (i-B.size()+1) : 0;
					uint64_t const end = std::min(i+1,A.size());

					for ( uint64_t j = start; j < end; ++j )
					{
						assert ( j < A.size() );
						assert ( i-j < B.size() );
						s += A[j] * B[i-j];
					}

					R[i] = s;
				}

				return R;
			}

			static std::vector<double> convolutionFFTW(
				std::vector<double> const & RA,
				std::vector<double> const & RB
			);

			template<typename type_1, typename type_2>
			static std::vector<double> convolutionFFT(
				std::vector<type_1> const & RA,
				std::vector<type_2> const & RB
			)
			{
				std::vector<double> const A(RA.begin(),RA.end());
				std::vector<double> const B(RB.begin(),RB.end());

				#if defined(LIBMAUS2_HAVE_FFTW)
				return convolutionFFTW(A,B);
				#else
				return convolution(A,B);
				#endif
			}
		};
	}
}
#endif
