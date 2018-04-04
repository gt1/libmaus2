/*
    libmaus2
    Copyright (C) 2018 German Tischler-Höhle

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
#if !defined(LIBMAUS2_LZ_BGZFINFLATBASEALLOCATOR_HPP)
#define LIBMAUS2_LZ_BGZFINFLATBASEALLOCATOR_HPP

#include <libmaus2/lz/BgzfInflateBase.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct BgzfInflateBaseAllocator
		{
			BgzfInflateBaseAllocator() {}

			libmaus2::lz::BgzfInflateBase::shared_ptr_type operator()()
			{
				libmaus2::lz::BgzfInflateBase::shared_ptr_type tptr(new libmaus2::lz::BgzfInflateBase);
				return tptr;
			}
		};
	}
}
#endif
