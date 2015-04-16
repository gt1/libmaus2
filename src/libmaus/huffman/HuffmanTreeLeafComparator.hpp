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

#if ! defined(HUFFMANTREELEAFCOMPARATOR_HPP)
#define HUFFMANTREELEAFCOMPARATOR_HPP

#include <libmaus/huffman/HuffmanTreeLeaf.hpp>

namespace libmaus
{
	namespace huffman
	{
		struct HuffmanTreeLeafComparator
		{
			bool operator()(HuffmanTreeLeaf const * const A, HuffmanTreeLeaf const * const B)
			{
				if ( A->frequency != B->frequency )
					return A->frequency < B->frequency;
				else
					return A->symbol < B->symbol;
			}
		};
	}
}
#endif
