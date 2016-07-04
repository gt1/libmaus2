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
#if ! defined(LIBMAUS2_AUTOARRAY_AUTOARRAY2D_HPP)
#define LIBMAUS2_AUTOARRAY_AUTOARRAY2D_HPP

#include <libmaus2/autoarray/AutoArray.hpp>

namespace libmaus2
{
	namespace autoarray
	{
		template<typename _N, alloc_type _atype = alloc_type_cxx>
		struct AutoArray2d
		{
			typedef _N N;
			static alloc_type const atype = _atype;
			typedef AutoArray2d<N,atype> this_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			uint64_t n; // rows
			uint64_t m; // columns
			::libmaus2::autoarray::AutoArray<N,atype> A;

			/**
			 * compute prefix sums for row i
			 *
			 * @param i row number
			 * @return sum over row
			 **/
			uint64_t prefixSums(uint64_t const i)
			{
				N * p = &(at(i,0));
				N * const pe = p + m;

				N s = 0;
				for ( ; p != pe; ++p )
				{
					N t = *p;
					*p = s;
					s += t;
				}

				return s;
			}

			/**
			 * compute prefix sums for all rows
			 **/
			void prefixSums()
			{
				for ( uint64_t i = 0; i < n; ++i )
					prefixSums(i);
			}

			AutoArray2d() : n(0), m(0), A() {}
			AutoArray2d(uint64_t const rn, uint64_t const rm, bool const rinit = true) : n(rn), m(rm), A(n*m,rinit) {}
			AutoArray2d(AutoArray2d<N,atype> const & o) : n(o.n), m(o.m), A(o.A) {}

			void setup(uint64_t const rn, uint64_t const rm, bool const erase = true)
			{
				A.ensureSize(rn*rm);
				n = rn;
				m = rm;
				if ( erase )
					std::fill(A.begin(),A.begin()+n*m,N());
			}

			AutoArray2d<N,atype> & operator=(AutoArray2d<N,atype> const & o)
			{
				if ( this != &o )
				{
					n = o.n;
					m = o.m;
					A = o.A;
				}

				return *this;
			}

			N       & operator()(uint64_t const i, uint64_t const j)       { return A[i*m+j]; }
			N const & operator()(uint64_t const i, uint64_t const j) const { return A[i*m+j]; }

			N       & at(uint64_t const i, uint64_t const j)
			{
				if ( i < n && j < m )
					return A.at(i*m+j);
				else
				{
					libmaus2::exception::LibMausException ex;
					ex.getStream() << "index pair (" << i << "," << j << ") is out of range for AutoArray2d of size (" << n << "," << m << ")" << std::endl;
					ex.finish();
					throw ex;
				}
			}

			N const & at(uint64_t const i, uint64_t const j) const
			{
				if ( i < n && j < m )
					return A.at(i*m+j);
				else
				{
					libmaus2::exception::LibMausException ex;
					ex.getStream() << "index pair (" << i << "," << j << ") is out of range for AutoArray2d of size (" << n << "," << m << ")" << std::endl;
					ex.finish();
					throw ex;
				}
			}

			N const * operator[](uint64_t const i) const
			{
				return &(at(i,0));
			}
			N       * operator[](uint64_t const i)
			{
				return &(at(i,0));
			}
		};
	}
}
#endif
