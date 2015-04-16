/**
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
**/
#if ! defined(LIBMAUS_SUFFIXSORT_BWTMERGEENUMBASE_HPP)
#define LIBMAUS_SUFFIXSORT_BWTMERGEENUMBASE_HPP

namespace libmaus2
{
	namespace suffixsort
	{
		struct BwtMergeEnumBase
		{
			enum bwt_merge_sort_input_type 
			{
				bwt_merge_input_type_byte = 0,
				bwt_merge_input_type_compact = 1,
				bwt_merge_input_type_pac = 2,
				bwt_merge_input_type_pac_term = 3,
				bwt_merge_input_type_utf8 = 4,
				bwt_merge_input_type_lz4 = 5
			};
		};
	}
}
#endif
