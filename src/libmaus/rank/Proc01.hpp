/*
    libmaus
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

#if ! defined(PROC01_HPP)
#define PROC01_HPP

namespace libmaus
{
	namespace rank
	{
		/**
		 * processing class for 01 pattern
		 **/
		template<typename type>
		struct Proc01
		{
			/**
			 * returns the low bit of a
			 * @param a
			 * @return a & 1
			 **/
			static unsigned int lowbit(type a)
			{
				return a & 0x1;
			}
			/**
			 * returns the high bit of a as low bit
			 * @param a
			 * @return high bit of a
			 **/
			static unsigned int highbit(type a)
			{
				return a >> (8*sizeof(type)-1);
			}
			/**
			 * returns number b such that the population count
			 * of b reflects the number of the occurrences of
			 * 01 in a
			 **/
			static type proc01(type const a)
			{
				return (((a>>1) ^ a) & a) << 1;
			}
			/**
			 * returns number c such that the population count
			 * of c reflects the number of the occurrences of
			 * 01 in a and in the last bit of a and the first of b
			 **/
			static type proc01(type const a, type const b)
			{
				return proc01(a) | ((!lowbit(a)) & highbit(b));
			}
		};
	}
}
#endif
