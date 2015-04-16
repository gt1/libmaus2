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

#if ! defined(ISQRT_HPP)
#define ISQRT_HPP

#include <libmaus2/types/types.hpp>

namespace libmaus2
{
  namespace math
  {
          /**
           * compute \lfloor\sqrt{n}\rfloor
           **/
          inline uint64_t isqrt(uint64_t const n)
          {
            uint64_t s = 0;
            uint64_t u = 1;
            uint64_t r = 0;
            
            // invariant at start of loop: r = sqrt(s)
            while ( s < n )
            {
              s += u;
              u += 2;
              
              if ( s <= n )
                r += 1;
            }
            
            return r;
          }
  }
}
#endif
