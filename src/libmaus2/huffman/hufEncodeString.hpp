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

#if ! defined(HUFENCODESTRING_HPP)
#define HUFENCODESTRING_HPP

#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/huffman/huffman.hpp>
#include <libmaus2/bitio/FastWriteBitWriter.hpp>

namespace libmaus2
{
	namespace huffman
	{
		template<typename iterator>
		inline autoarray::AutoArray < uint64_t > hufEncodeString ( iterator a, iterator e, huffman::HuffmanTreeNode const * root )
		{
			// build encoding table
			unsigned int const lookupwords = 2;
			::libmaus2::huffman::EncodeTable<lookupwords> enctable ( root );

			uint64_t const codelength = enctable.getCodeLength(a,e);
			uint64_t const arraylength = (codelength + 63)/64+1;
			::libmaus2::autoarray::AutoArray<uint64_t> acode(arraylength);

			bitio::FastWriteBitWriter8 fwbw8(acode.get());

			for ( ; a != e ; ++a )
				enctable [ *a ].first.serialize ( fwbw8 , enctable[*a].second );
			fwbw8.flush();

			return acode;
		}
	}
}
#endif
