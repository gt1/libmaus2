/*
    libmaus
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
#if ! defined(LIBMAUS_LZ_SIMPLECOMPRESSEDINPUTBLOCKCONCATBLOCK_HPP)
#define LIBMAUS_LZ_SIMPLECOMPRESSEDINPUTBLOCKCONCATBLOCK_HPP

#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/lz/DecompressorObjectFactory.hpp>
#include <libmaus/lz/SimpleCompressedStreamNamedInterval.hpp>

namespace libmaus
{
	namespace lz
	{
		struct SimpleCompressedInputBlockConcatBlock
		{
			typedef SimpleCompressedInputBlockConcatBlock this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
		
			libmaus::autoarray::AutoArray<uint8_t> I;
			libmaus::autoarray::AutoArray<uint8_t> O;
			uint64_t metasize;
			uint64_t compsize;
			uint64_t uncompsize;
			bool eof;
			libmaus::lz::SimpleCompressedStreamNamedInterval const * currentInterval;
			uint64_t blockstreampos;

			libmaus::lz::DecompressorObject::unique_ptr_type Pdecompressor;
			libmaus::lz::DecompressorObject & decompressor;

			SimpleCompressedInputBlockConcatBlock(libmaus::lz::DecompressorObjectFactory & factory)
			: metasize(0), compsize(0), uncompsize(0), eof(true), currentInterval(0), blockstreampos(0),
			  Pdecompressor(factory()), decompressor(*Pdecompressor)
			{
			
			}			

			bool uncompressBlock()
			{
				if ( uncompsize > O.size() )
					O = libmaus::autoarray::AutoArray<uint8_t>(uncompsize,false);			

				bool ok = true;

				if ( uncompsize )
				{
					ok = decompressor.rawuncompress(
						reinterpret_cast<char const *>(I.begin()),compsize,
						reinterpret_cast<char *>(O.begin()),uncompsize
					);
				}
								
				if ( blockstreampos == currentInterval->end.first )
				{
					uncompsize = currentInterval->end.second;
				}
				if ( blockstreampos == currentInterval->start.first )
				{
					std::memmove(
						O.begin(),
						O.begin() + currentInterval->start.second,
						uncompsize-currentInterval->start.second
					);
					
					uncompsize -= currentInterval->start.second;
				}
				
				return ok;
			}
		};
	}
}
#endif
