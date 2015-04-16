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
#if ! defined(LIBMAUS2_MATH_DOUBLECODE_HPP)
#define LIBMAUS2_MATH_DOUBLECODE_HPP

#include <cmath>
#include <cassert>
#include <limits>
#include <libmaus2/LibMausConfig.hpp>

namespace libmaus2
{
	namespace math
	{
		struct DoubleCode
		{
			enum bias { expbias = 1022 };

			static uint64_t encodeDouble(double const v)
			{
				assert ( std::numeric_limits<double>::is_iec559 );
			
				// copy 
				double t = v;
				
				// check for nan
				assert ( t == t );
				
				if ( (!(t<0)) && (!(t>0)) )
					return 0;

				// handle sign		
				bool const sign = t < 0.0;
			
				// flip sign if negative
				if ( sign )
					t = -t;
					
				assert ( t > 0 );

				// we currently do not handle denormalised numbers
				// if number is denormalized, then turn it into the
				// smallest normalised one
				if ( t < std::numeric_limits<double>::min() )
				{
					t = std::numeric_limits<double>::min();
				}

				uint64_t o = 0;
				
				if ( t > std::numeric_limits<double>::max() )
				{
					o |= 0x7FFull << 52;
				}
				else if ( t )
				{
					int exp = 0;
					t = frexp(t,&exp);
					
					exp += static_cast<int>(expbias);
					assert ( exp >= 0 );
					uint64_t const uexp = static_cast<uint64_t>(exp);
					
					// std::cerr << "exp=" << exp << std::endl;
					
					t *= 2;
					assert ( t >= 1 );
					t -= 1;
					
					for ( uint64_t i = 0; i < 52; ++i )
					{
						o <<= 1;
						t *= 2;
						if ( t >= 1 )
						{
							o |= 1;
							t -= 1;
						}
					}
					
					o |= (uexp<<52);
				}
				o |= static_cast<uint64_t>(sign) << 63;
				
				return o;
			}
			
			static double decodeDouble(uint64_t const o)
			{
				if ( ! o )
					return 0.0;
			
				// extract sign
				bool const sign = (o >> 63)&1;
				
				// extract exponent
				int64_t const preexp = static_cast<int64_t>((o >> 52) & 0x7FF);
				double d;
				
				// infinity
				if ( 
					(preexp == 0x7FF) 
					&& 
					(
						o & ((1ull<<52)-1)
					) 
					== 0 
				)
				{
					d = std::numeric_limits<double>::infinity();
				}
				// finite
				else
				{
					int64_t const exp = preexp - static_cast<int>(expbias);

					// decode mantissa
					d = 1;
					for ( uint64_t m = 1ull<<51; m; m >>= 1 )
					{
						d *= 2;
						if ( o & m )
							d += 1;
					}
					
					// shift into place (add exponent to d)
					d = ldexp(d,exp-53);
				}
				
				return sign ? -d : d;
			}
		};
	}
}
#endif
