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
#if !defined(LIBMAUS2_HUFFMAN_INDEXWRITER_HPP)
#define LIBMAUS2_HUFFMAN_INDEXWRITER_HPP

#include <vector>
#include <libmaus2/huffman/IndexEntry.hpp>
#include <libmaus2/math/numbits.hpp>
#include <libmaus2/math/bitsPerNum.hpp>

namespace libmaus2
{
	namespace huffman
	{
		struct IndexWriter
		{
			template<typename writer_type>
			static void writeIndex(
				writer_type & writer,
				std::vector < IndexEntry > const & index,
				uint64_t const indexpos,
				uint64_t const numsyms = std::numeric_limits<uint64_t>::max()
			)
			{
				uint64_t const maxpos = index.size() ? index[index.size()-1].pos : 0;
				unsigned int const posbits = ::libmaus2::math::bitsPerNum(maxpos);

				uint64_t const kacc = std::accumulate(index.begin(),index.end(),0ull,IndexEntryKeyAdd());
				unsigned int const kbits = ::libmaus2::math::bitsPerNum(kacc);

				uint64_t const vacc = std::accumulate(index.begin(),index.end(),0ull,IndexEntryValueAdd());
				unsigned int const vbits = ::libmaus2::math::bitsPerNum(vacc);

				// index size (number of blocks)
				writer.writeElias2(index.size());
				// write number of bits per file position
				writer.writeElias2(posbits);

				// write kbits
				writer.writeElias2(kbits);
				// write kacc
				writer.writeElias2(kacc);

				// write vbits
				writer.writeElias2(vbits);
				// write vacc
				writer.writeElias2(vacc);

				// align
				writer.flushBitStream();

				uint64_t tkacc = 0, tvacc = 0;
				// write index
				for ( uint64_t i = 0; i < index.size(); ++i )
				{
					writer.write(index[i].pos,posbits);

					writer.write(tkacc,kbits);
					tkacc += index[i].kcnt;

					writer.write(tvacc,vbits);
					tvacc += index[i].vcnt;
				}
				writer.write(0,posbits);
				writer.write(tkacc,kbits);
				writer.write(tvacc,vbits);
				writer.flushBitStream();

				if ( numsyms != std::numeric_limits<uint64_t>::max() )
					assert ( numsyms == vacc );

				for ( uint64_t i = 0; i < 64; ++i )
					writer.writeBit( (indexpos & (1ull<<(63-i))) != 0 );
				writer.flushBitStream();
			}
		};
	}
}
#endif
