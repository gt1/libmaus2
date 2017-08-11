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
#include <libmaus2/LibMausConfig.hpp>
#include <libmaus2/math/ConstantSumCombinatorics.hpp>

int main(int argc, char * argv[])
{
	try
	{
		for ( uint64_t i = 0; i < 20; ++i )
		{
			std::vector < libmaus2::math::Rational<libmaus2::math::GmpInteger> > V = libmaus2::math::Faulhaber<libmaus2::math::GmpInteger>::polynomial(i);
			for ( uint64_t j = 0; j < V.size(); ++j )
				std::cerr << V[j] << ";";
			std::cerr << std::endl;
		}

		uint64_t c = 0;
		uint64_t const n = 20;
		for ( uint64_t i0 = 0; i0 <= n; ++i0 )
		for ( uint64_t i1 = 0; i1 <= n; ++i1 )
		for ( uint64_t i2 = 0; i2 <= n; ++i2 )
		for ( uint64_t i3 = 0; i3 <= n; ++i3 )
		for ( uint64_t i4 = 0; i4 <= n; ++i4 )
		if ( i0 + i1 + i2 + i3 + i4 == n )
		{
			++c;
		}
		std::cerr << "c=" << c << std::endl;

		std::vector < libmaus2::math::Rational<libmaus2::math::GmpInteger> > cur = libmaus2::math::ConstantSumCombinatorics<libmaus2::math::GmpInteger>::first();
		libmaus2::math::BernoulliNumberCache<libmaus2::math::GmpInteger> BNC;
		for ( uint64_t i = 0; i <= 20; ++i )
		{
			std::cerr << i << "\t";

			// std::vector < libmaus2::math::Rational<libmaus2::math::GmpInteger> > cur = libmaus2::math::ConstantSumCombinatorics<libmaus2::math::GmpInteger>::kth(i);

			#if 0
			for ( uint64_t i = 0; i < cur.size(); ++i )
				std::cerr << cur[i] << ";";

			std::cerr << "\t";
			#endif

			libmaus2::math::GmpInteger const c =
				libmaus2::math::RationalPolynomialSupport<libmaus2::math::GmpInteger>::poleval(cur,libmaus2::math::Rational<libmaus2::math::GmpInteger>(n)).c;

			std::cerr << c;
			std::cerr << std::endl;

			cur = libmaus2::math::ConstantSumCombinatorics<libmaus2::math::GmpInteger>::next(cur,BNC);

			uint64_t const uc = static_cast<uint64_t>(c);
		}
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
