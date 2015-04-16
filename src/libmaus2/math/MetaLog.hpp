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

#if ! defined(METALOG_HPP)
#define METALOG_HPP

namespace libmaus
{
	namespace math
	{
		template<unsigned long long k>
		struct MetaLog2
		{
			static unsigned int const log = 1 + MetaLog2<k/2>::log;
		};
		template<>
		struct MetaLog2<1>
		{
			static unsigned int const log = 0;
		};

		template<unsigned int b, unsigned int e>
		struct MetaPow
		{
			static unsigned long long const val = b * MetaPow<b,e-1>::val;
		};
		template<unsigned int b>
		struct MetaPow<b,0>
		{
			static unsigned long long const val = 1;
		};
		
		template<unsigned long long k>
		struct MetaNumBits
		{
		        static unsigned int const bits = 1 + MetaNumBits< (k >> 1) >::bits;
		};
		template<>
		struct MetaNumBits<0>
		{
		        static unsigned int const bits = 0;
		};
		
		template<unsigned long long k>
		struct MetaLog2Up
		{
			static unsigned int const log =
				(
					( 
						MetaPow< 2, 
						         MetaLog2<k>::log 
					        >::val == k 
					) 
					? 
					MetaLog2<k>::log : (MetaLog2<k>::log+1)
				);
		};
		template<unsigned int k>
		struct MetaLowBits
		{
			static unsigned long long const lowbits = ((MetaLowBits<k-1>::lowbits << 1) | 1ull);	
		};
		template<>
		struct MetaLowBits<0>
		{
			static unsigned long long const lowbits = 0;
		};
	}
}
#endif
