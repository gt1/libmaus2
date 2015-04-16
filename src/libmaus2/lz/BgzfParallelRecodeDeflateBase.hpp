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
#if ! defined(LIBMAUS_LZ_BGZFPARALLELRECODEBASE_HPP)
#define LIBMAUS_LZ_BGZFPARALLELRECODEBASE_HPP

#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/lz/BgzfConstants.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct BgzfParallelRecodeDeflateBase : public ::libmaus2::lz::BgzfConstants
		{
			libmaus2::autoarray::AutoArray<uint8_t> B;
			
			uint8_t * const pa;
			uint8_t * pc;
			uint8_t * const pe;
			
			BgzfParallelRecodeDeflateBase()
			: B(getBgzfMaxBlockSize(),false), 
			  pa(B.begin()), 
			  pc(B.begin()), 
			  pe(B.end())
			{
			
			}
		};
	}
}
#endif
