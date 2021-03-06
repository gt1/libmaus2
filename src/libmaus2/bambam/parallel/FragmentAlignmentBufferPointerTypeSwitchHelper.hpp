/*
    libmaus2
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#if ! defined(LIBMAUS2_BAMBAM_PARALLEL_FRAGMENTALIGNMENTBUFFERPOINTERTYPESWITCHHELPER_HPP)
#define LIBMAUS2_BAMBAM_PARALLEL_FRAGMENTALIGNMENTBUFFERPOINTERTYPESWITCHHELPER_HPP

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{
			template<typename _A, typename _B, size_t k> struct FragmentAlignmentBufferPointerTypeSwitchHelper {};
			template<typename _A, typename _B>           struct FragmentAlignmentBufferPointerTypeSwitchHelper<_A,_B,1> { typedef _A type; };
			template<typename _A, typename _B>           struct FragmentAlignmentBufferPointerTypeSwitchHelper<_A,_B,0> { typedef _B type; };
		}
	}
}
#endif
