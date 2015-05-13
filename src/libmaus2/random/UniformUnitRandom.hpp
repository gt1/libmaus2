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
#if ! defined(LIBMAUS2_RANDOM_UNIFORMUNITRANDOM_HPP)
#define LIBMAUS2_RANDOM_UNIFORMUNITRANDOM_HPP

#include <libmaus2/random/Random.hpp>

namespace libmaus2
{
	namespace random
	{
		struct UniformUnitRandom
		{
			/* produce random number between 0 and 1 */
			static double uniformUnitRandom()
			{
				return static_cast<double>(libmaus2::random::Random::rand32()) / static_cast<double>((1ull<<32)-1);
			}
		};
	}
}
#endif
