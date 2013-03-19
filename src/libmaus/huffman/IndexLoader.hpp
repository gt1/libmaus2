/**
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
**/

#if ! defined(INDEXLOADER_HPP)
#define INDEXLOADER_HPP

#include <libmaus/huffman/IndexLoaderBase.hpp>

#include <libmaus/bitio/readElias.hpp>
#include <libmaus/util/ReverseByteOrder.hpp>
#include <libmaus/huffman/IndexEntry.hpp>
#include <libmaus/util/iterator.hpp>
#include <libmaus/util/GetFileSize.hpp>
#include <libmaus/bitio/BitIOInput.hpp>
#include <iostream>

#if defined(__linux__)
#include <byteswap.h>
#endif

#if defined(__FreeBSD__)
#include <sys/endian.h>
#endif

namespace libmaus
{
	namespace huffman
	{
		struct IndexLoader : public IndexLoaderBase
		{
			/* 
			 * load indexes for multiple files 
			 */
			static ::libmaus::autoarray::AutoArray< ::libmaus::autoarray::AutoArray< IndexEntry > > 
				loadIndex(std::vector<std::string> const & filenames)
			{
				::libmaus::autoarray::AutoArray< ::libmaus::autoarray::AutoArray< IndexEntry > > index(filenames.size());
				
				for ( uint64_t i = 0; i < index.size(); ++i )
					index[i] = loadIndex(filenames[i]);
				
				return index;
			}
			

			/*
			 * load index for one file 
			 */
			static libmaus::autoarray::AutoArray< IndexEntry > loadIndex(std::string const & filename)
			{
				uint64_t const indexpos = getIndexPos(filename);

				std::ifstream indexistr(filename.c_str(),std::ios::binary);

				if ( ! indexistr.is_open() )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "RLDecoder::loadIndex(): Failed to open file " << filename << std::endl;
					se.finish();
					throw se;
				}

				// seek to index position
				indexistr.seekg(indexpos,std::ios::beg);
				if ( static_cast<int64_t>(indexistr.tellg()) != static_cast<int64_t>(indexpos) )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Failed to seek to index position " << indexpos << " in file " << filename << " of size " 
						<< ::libmaus::util::GetFileSize::getFileSize(filename) << std::endl;
					se.finish();
					throw se;
				}
				::libmaus::bitio::StreamBitInputStream SBIS(indexistr);
				
				// read size of index
				uint64_t const numentries = ::libmaus::bitio::readElias2(SBIS);
				// pos bits
				unsigned int const posbits = ::libmaus::bitio::readElias2(SBIS);
				
				// k bits
				unsigned int const kbits = ::libmaus::bitio::readElias2(SBIS);
				// k acc
				/* uint64_t const symacc = */ ::libmaus::bitio::readElias2(SBIS);

				// v bits
				unsigned int const vbits = ::libmaus::bitio::readElias2(SBIS);
				// v acc
				/* uint64_t const symacc = */ ::libmaus::bitio::readElias2(SBIS);
				
				// align
				SBIS.flush();
				
				SBIS.getBitsRead();
				
				// std::cerr << "numentries " << numentries << std::endl;
				
				// read index
				libmaus::autoarray::AutoArray< IndexEntry > index(numentries,false);
				
				// #define INDEXLOADERDEBUG
				
				#if defined(INDEXLOADERDEBUG)
				libmaus::autoarray::AutoArray< IndexEntry > tindex(numentries+1);
				#endif

				for ( uint64_t i = 0; i < numentries; ++i )
				{
					uint64_t const pos = SBIS.read(posbits);
					uint64_t const kcnt = SBIS.read(kbits);
					uint64_t const vcnt = SBIS.read(vbits);
					index[i] = IndexEntry(pos,kcnt,vcnt);

					#if defined(INDEXLOADERDEBUG)
					tindex[i] = index[i];
					#endif
				}
				if ( numentries )
				{
					assert ( index[0].kcnt == 0 );
					assert ( index[0].vcnt == 0 );
					for ( uint64_t i = 1; i < numentries; ++i )
					{
						index[i-1].kcnt = index[i].kcnt;
						index[i-1].vcnt = index[i].vcnt;
					}
				
					/* uint64_t const pos = */   SBIS.read(posbits);
					index[numentries-1].kcnt = SBIS.read(kbits);
					index[numentries-1].vcnt = SBIS.read(vbits);
					
					#if defined(INDEXLOADERDEBUG)
					tindex[numentries].kcnt = index[numentries-1].kcnt;
					tindex[numentries].vcnt = index[numentries-1].vcnt;
					#endif
				}
				
				for ( uint64_t i = numentries-1; i >= 1; --i )
				{
					index[i].kcnt -= index[i-1].kcnt;
					index[i].vcnt -= index[i-1].vcnt;
				}
					
				#if defined(INDEXLOADERDEBUG)
				for ( uint64_t i = 0; i < numentries; ++i )
				{
					assert ( index[i].kcnt == tindex[i+1].kcnt-tindex[i].kcnt );
					assert ( index[i].vcnt == tindex[i+1].vcnt-tindex[i].vcnt );
				}
				#endif
				
				#if defined(INDEXLOADERDEBUG)
				IndexDecoderData IDD(filename);
				assert ( IDD.numentries == numentries );
				assert ( IDD.posbits == posbits );
				assert ( IDD.kbits == kbits );
				assert ( IDD.vbits == vbits );
				// assert ( IDD.symacc == symacc );
				
				std::cerr << "((**++CHECKING " << numentries << "...";
				uint64_t tkacc = 0;
				uint64_t tvacc = 0;
				for ( uint64_t i = 0; i < numentries; ++i )
				{
					IndexEntry const P = IDD.readEntry(i);
					assert ( P.pos == index[i].pos );
					assert ( P.kcnt == tkacc );
					assert ( P.vcnt == tvacc );
									
					tkacc += index[i].kcnt;
					tvacc += index[i].vcnt;
				}
				assert ( tkacc == IDD.readEntry(numentries).kcnt );
				assert ( tvacc == IDD.readEntry(numentries).vcnt );
				std::cerr << "**++))" << std::endl;
				#endif

				// std::cerr << "loaded index of size " << numentries << std::endl;
				
				return index;
			}
		};
	}
}
#endif
