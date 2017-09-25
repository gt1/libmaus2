/*
    libmaus2
    Copyright (C) 2017 German Tischler

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
#if ! defined(LIBMAUS2_MATH_MATRIX_HPP)
#define LIBMAUS2_MATH_MATRIX_HPP

#include <libmaus2/autoarray/AutoArray.hpp>

namespace libmaus2
{
	namespace math
	{
		template<typename _N> struct Matrix;
		template<typename N> std::ostream & operator<<(std::ostream & out, Matrix<N> const & M);

		template<typename _N> struct Matrix
		{
			typedef _N N;
			typedef Matrix<N> this_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			uint64_t r; // rows
			uint64_t c; // columns
			libmaus2::autoarray::AutoArray<N> A; // data

			N const & operator()(uint64_t const i, uint64_t const j) const
			{
				return A [ i * c + j ];
			}

			N & operator()(uint64_t const i, uint64_t const j)
			{
				return A [ i * c + j ];
			}

			Matrix()
			{
			}

			Matrix(uint64_t const rr, uint64_t const rc)
			: r(rr), c(rc), A(r*c)
			{
			}

			Matrix(Matrix<N> const & O)
			: r(O.r), c(O.c), A(r*c)
			{
				std::copy(O.A.begin(),O.A.end(),A.begin());
			}

			void swapRows(uint64_t const i, uint64_t const j)
			{
				for ( uint64_t k = 0; k < c; ++k )
					std::swap(
						(*this)(i,k),
						(*this)(j,k)
					);
			}

			void addRows(uint64_t const i, uint64_t const j, N const s)
			{
				for ( uint64_t k = 0; k < c; ++k )
					(*this)(i,k) += s * (*this)(j,k);
			}

			void multRow(uint64_t const i, N const s)
			{
				for ( uint64_t k = 0; k < c; ++k )
					(*this)(i,k) *= s;
			}

			// inverse matrix by Gauss elimination
			Matrix<N> inverse(N const t = N())
			{
				assert ( r == c );

				Matrix<N> U = *this;
				Matrix<N> I(r,c);

				for ( uint64_t i = 0; i < r; ++i )
					I(i,i) = N(1);

				for ( uint64_t i = 0; i < c; ++i )
				{
					N m = U(i,i);
					uint64_t mj = i;

					for ( uint64_t j = i; j < r; ++j )
						if ( abs(m) < abs(U(j,i)) )
						{
							m = U(j,i);
							mj = j;
						}

					if ( abs(m) < t )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "Matrix::inverse: Matrix is not invertible" << std::endl;
						lme.finish();
						throw lme;
					}

					U.swapRows(i,mj);
					I.swapRows(i,mj);

					for ( uint64_t j = i+1; j < r; ++j )
					{
						N const s = -U(j,i)/U(i,i);

						U.addRows(j,i,s);
						I.addRows(j,i,s);
					}
				}

				for ( uint64_t ii = 0; ii < c; ++ii )
				{
					uint64_t const i = c-ii-1;

					for ( uint64_t jj = ii+1; jj < c; ++jj )
					{
						uint64_t const j = c-jj-1;

						N const s = -U(j,i) / U(i,i);

						U.addRows(j,i,s);
						I.addRows(j,i,s);
					}
				}

				for ( uint64_t i = 0; i < c; ++i )
				{
					N const s = N(1) / U(i,i);

					U.multRow(i,s);
					I.multRow(i,s);
				}

				for ( uint64_t i = 0; i < r; ++i )
					for ( uint64_t j = 0; j < c; ++j )
						if ( abs(I(i,j)) < t )
							I(i,j) = N();

				return I;
			}

			static N abs(N s)
			{
				if ( s < N() )
					return -s;
				else
					return s;
			}

			Matrix<N> & operator+=(Matrix<N> const & O)
			{
				assert ( r == O.r );
				assert ( c == O.c );

				for ( uint64_t i = 0; i < A.size(); ++i )
					A[i] += O.A[i];

				return *this;
			}

			Matrix<N> & operator-=(Matrix<N> const & O)
			{
				assert ( r == O.r );
				assert ( c == O.c );

				for ( uint64_t i = 0; i < A.size(); ++i )
					A[i] -= O.A[i];

				return *this;
			}
		};

		template<typename N> Matrix<N> operator*(Matrix<N> const & A, Matrix<N> const & B)
		{
			assert ( A.c == B.r );

			Matrix<N> R(A.r,B.c);

			for ( uint64_t i = 0; i < R.r; ++i )
				for ( uint64_t j = 0; j < R.c; ++j )
					for ( uint64_t k = 0; k < A.c; ++k )
						R(i,j) += A(i,k) * B(k,j);

			return R;
		}

		template<typename N> std::ostream & operator<<(std::ostream & out, Matrix<N> const & A)
		{
			for ( uint64_t i = 0; i < A.r; ++i )
			{
				for ( uint64_t j = 0; j < A.c; ++j )
				{
					out << A(i,j);

					if ( j+1 < A.c )
						out.put('\t');
				}

				out.put('\n');
			}

			return out;
		}
	}
}
#endif
