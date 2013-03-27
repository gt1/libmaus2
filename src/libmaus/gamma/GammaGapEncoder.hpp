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
#if ! defined(LIBMAUS_GAMMA_GAMMAGAPENCODER_HPP)
#define LIBMAUS_GAMMA_GAMMAGAPENCODER_HPP

#include <libmaus/aio/CheckedOutputStream.hpp>
#include <libmaus/gamma/GammaEncoder.hpp>
#include <libmaus/huffman/IndexEntry.hpp>
#include <libmaus/util/NumberSerialisation.hpp>
#include <libmaus/aio/SynchronousGenericOutput.hpp>
#include <libmaus/bitio/FastWriteBitWriter.hpp>
#include <libmaus/math/bitsPerNum.hpp>

namespace libmaus
{
	namespace gamma
	{
		struct GammaGapEncoder
		{
			::libmaus::aio::CheckedOutputStream COS;
			::std::vector< ::libmaus::huffman::IndexEntry > index;
			
			GammaGapEncoder(std::string const & filename)
			: COS(filename) 
			{
			}
			
			template<typename iterator>
			uint64_t encodeInternal(iterator ita, iterator ite)
			{
				uint64_t const blocksize = 256*1024;
				uint64_t const n = ite-ita;
				::libmaus::aio::SynchronousGenericOutput<uint64_t> SGO(COS,64*1024);
				::libmaus::gamma::GammaEncoder < ::libmaus::aio::SynchronousGenericOutput<uint64_t> > GE(SGO);
				uint64_t const numblocks = (n + blocksize-1)/blocksize;

				SGO.put(n);
				
				for ( uint64_t b = 0; b < numblocks; ++b )
				{
					uint64_t const blow = std::min(b*blocksize,n);
					uint64_t const bhigh = std::min(blow+blocksize,n);
					uint64_t const bcnt = bhigh-blow;
					
					uint64_t const pos = SGO.getWrittenBytes();
					uint64_t vacc = 0;
					
					GE.encodeWord(bcnt,32);
					for ( uint64_t i = 0; i < bcnt; ++i )
					{
						uint64_t const v = *(ita++);
						GE.encode(v);
						vacc += v;
					}
					GE.flush();
					
					::libmaus::huffman::IndexEntry const entry(pos,bcnt,vacc);
					index.push_back(entry);										
				}
				
				SGO.flush();
				
				return SGO.getWrittenBytes();
			}

			void writeIndex(::libmaus::bitio::FastWriteBitWriterStream8Std & gapHEF, uint64_t const indexpos)
			{
				uint64_t const maxpos = index.size() ? index[index.size()-1].pos : 0;
				unsigned int const posbits = ::libmaus::math::bitsPerNum(maxpos);
				
				uint64_t const kacc = std::accumulate(index.begin(),index.end(),0ull,::libmaus::huffman::IndexEntryKeyAdd());
				unsigned int const kbits = ::libmaus::math::bitsPerNum(kacc);

				uint64_t const vacc = std::accumulate(index.begin(),index.end(),0ull,::libmaus::huffman::IndexEntryValueAdd());
				unsigned int const vbits = ::libmaus::math::bitsPerNum(vacc);

				// write number of entries in index
				gapHEF.writeElias2(index.size());
				// write number of bits per file position
				gapHEF.writeElias2(posbits);
				
				// write number of bits per sym acc
				gapHEF.writeElias2(kbits);
				// write symacc
				gapHEF.writeElias2(kacc);

				// write number of bits per sym acc
				gapHEF.writeElias2(vbits);
				// write symacc
				gapHEF.writeElias2(vacc);
				
				// align
				gapHEF.flush();
				
				uint64_t tkacc = 0, tvacc = 0;
				for ( uint64_t i = 0; i < index.size(); ++i )
				{
					gapHEF.write(index[i].pos,posbits); // position of block
					gapHEF.write(tkacc,kbits); // sum of values inblock
					gapHEF.write(tvacc,vbits); // sum of values inblock
					tkacc += index[i].kcnt;
					tvacc += index[i].vcnt;
				}
				gapHEF.write(0,posbits); // position of block
				gapHEF.write(tkacc,kbits); // sum of values inblock
				gapHEF.write(tvacc,vbits); // sum of values inblock
				gapHEF.flush();
			
				// write position of index in last 64 bits of file	
				for ( uint64_t i = 0; i < 64; ++i )
					gapHEF.writeBit( (indexpos & (1ull<<(63-i))) != 0 );

				gapHEF.flush();				
			}

			template<typename iterator>
			void encode(iterator ita, iterator ite)
			{
				uint64_t const indexpos = encodeInternal<iterator>(ita,ite);
								
				::libmaus::aio::SynchronousGenericOutput<uint8_t> SGO(COS,64*1024);
				::libmaus::aio::SynchronousGenericOutput<uint8_t>::iterator_type it(SGO);
				::libmaus::bitio::FastWriteBitWriterStream8Std FWBWS(it);
								
				writeIndex(FWBWS,indexpos);
				
				FWBWS.flush();
				SGO.flush();
				COS.flush();
			}
		};
	}
}
#endif
