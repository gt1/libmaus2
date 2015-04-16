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
#if ! defined(LIBMAUS_FASTX_PHRED_HPP)
#define LIBMAUS_FASTX_PHRED_HPP

namespace libmaus2
{
	namespace fastx
	{
		struct Phred
		{
			/*
			 * phred_error[i] = 1.0/pow(10.0,i/10.0)
			 */
			static double const phred_error[256];
			/*
			 * return probability that value with quality i is correct
			 * (i.e. 1.0-error probability)
			 */
			static double probCorrect(unsigned int const i)
			{
				return 1.0 - phred_error[i];
			}
		};
	}
}
#endif
