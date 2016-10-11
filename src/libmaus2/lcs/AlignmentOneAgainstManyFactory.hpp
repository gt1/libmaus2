/*
    libmaus2
    Copyright (C) 2016 German Tischler

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
#if ! defined(LIBMAUS2_LCS_ALIGNMENTONEAGAINSTMANYFACTORY_HPP)
#define LIBMAUS2_LCS_ALIGNMENTONEAGAINSTMANYFACTORY_HPP

#include <libmaus2/lcs/AlignmentOneAgainstManyGeneric.hpp>

#if defined(LIBMAUS2_HAVE_x86_64)
#include <libmaus2/util/I386CacheLineSize.hpp>
#include <libmaus2/lcs/AlignmentOneAgainstManyAVX2.hpp>
#endif

namespace libmaus2
{
	namespace lcs
	{
		struct AlignmentOneAgainstManyFactory
		{
			static libmaus2::lcs::AlignmentOneAgainstManyInterface::unique_ptr_type uconstruct()
			{
				#if defined(LIBMAUS2_HAVE_x86_64)
				if ( libmaus2::util::I386CacheLineSize::hasAVX2() )
				{
					libmaus2::lcs::AlignmentOneAgainstManyInterface::unique_ptr_type tptr(
						new libmaus2::lcs::AlignmentOneAgainstManyAVX2
					);
					return UNIQUE_PTR_MOVE(tptr);
				}
				else
				#endif
				{
					libmaus2::lcs::AlignmentOneAgainstManyInterface::unique_ptr_type tptr(
						new libmaus2::lcs::AlignmentOneAgainstManyGeneric
					);
					return UNIQUE_PTR_MOVE(tptr);
				}
			}
		};
	}
}
#endif
