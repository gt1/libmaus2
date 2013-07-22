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
#if ! defined(LIBMAUS_GAMMA_GAMMAGAPENCODER_HPP)
#define LIBMAUS_GAMMA_GAMMAGAPENCODER_HPP

#include <libmaus/aio/CheckedOutputStream.hpp>
#include <libmaus/gamma/GammaEncoder.hpp>
#include <libmaus/huffman/IndexEntry.hpp>
#include <libmaus/util/NumberSerialisation.hpp>
#include <libmaus/aio/SynchronousGenericOutput.hpp>
#include <libmaus/bitio/FastWriteBitWriter.hpp>
#include <libmaus/math/bitsPerNum.hpp>
#include <libmaus/gamma/GammaGapDecoder.hpp>

namespace libmaus
{
	namespace gamma
	{
		struct GammaGapEncoder
		{
			typedef GammaGapEncoder this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			
			static uint64_t const blocksize = 256*1024;
			
			::libmaus::aio::CheckedOutputStream COS;
			::std::vector< ::libmaus::huffman::IndexEntry > index;
			
			GammaGapEncoder(std::string const & filename)
			: COS(filename) 
			{
			}
			
			uint64_t writeFileSize(uint64_t const n)
			{
				::libmaus::aio::SynchronousGenericOutput<uint64_t> SGO(COS,64);
				SGO.put(n);
				SGO.flush();
				return SGO.getWrittenBytes();
			}
			
			template<typename iterator>
			uint64_t encodeInternal(iterator ita, uint64_t const n, uint64_t off)
			{
				::libmaus::aio::SynchronousGenericOutput<uint64_t> SGO(COS,64*1024);
				::libmaus::gamma::GammaEncoder < ::libmaus::aio::SynchronousGenericOutput<uint64_t> > GE(SGO);
				uint64_t const numblocks = (n + blocksize-1)/blocksize;
				
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
					
					::libmaus::huffman::IndexEntry const entry(pos+off,bcnt,vacc);
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
			
			void writeIndex(uint64_t const indexpos)
			{
				::libmaus::aio::SynchronousGenericOutput<uint8_t> SGO(COS,64*1024);
				::libmaus::aio::SynchronousGenericOutput<uint8_t>::iterator_type it(SGO);
				::libmaus::bitio::FastWriteBitWriterStream8Std FWBWS(it);
				writeIndex(FWBWS,indexpos);

				FWBWS.flush();
				SGO.flush();
			}
			
			template<typename iterator>
			void encode(iterator ita, uint64_t const n)
			{			
				uint64_t const fslen = writeFileSize(n);
				uint64_t const indexpos = fslen + encodeInternal<iterator>(ita,n,fslen);
												
				writeIndex(indexpos);
				
				COS.flush();
			}

			template<typename iterator>
			void encode(iterator ita, iterator ite)
			{
				uint64_t const n = ite-ita;
				encode(ita,n);
			}

			// merge multiple gap arrays to one by adding them up per rank
			static void merge(
				std::vector < std::vector<std::string> > const & infilenames,
				std::string const & outfilename
			)
			{
				if ( ! infilenames.size() )
				{
					GammaGapEncoder GGE(outfilename);
					uint64_t const * p = 0;
					GGE.encode(p,p);
				}
				else
				{
					uint64_t const n = GammaGapDecoder::getLength(infilenames[0]);
					for ( uint64_t i = 1; i < infilenames.size(); ++i  )
						if ( GammaGapDecoder::getLength(infilenames[i]) != n )
						{
							::libmaus::exception::LibMausException se;
							se.getStream() << "Incoherent gap file sizes in GammaGapEncoder::merge" << std::endl;
							se.finish();
							throw se;
						}
						
					GammaGapEncoder::unique_ptr_type GGE(new GammaGapEncoder(outfilename));
					::libmaus::autoarray::AutoArray<GammaGapDecoder::unique_ptr_type> GGD(infilenames.size());
					for ( uint64_t i = 0; i < infilenames.size(); ++i )
						GGD[i] = UNIQUE_PTR_MOVE(GammaGapDecoder::unique_ptr_type(new GammaGapDecoder(infilenames[i])));

					uint64_t const fslen = GGE->writeFileSize(n);
					uint64_t const numblocks = (n + blocksize-1)/blocksize;
					::libmaus::aio::SynchronousGenericOutput<uint64_t>::unique_ptr_type SGO(
						new ::libmaus::aio::SynchronousGenericOutput<uint64_t>(GGE->COS,64*1024));
					::libmaus::gamma::GammaEncoder < ::libmaus::aio::SynchronousGenericOutput<uint64_t> >::unique_ptr_type
						GE(new ::libmaus::gamma::GammaEncoder < ::libmaus::aio::SynchronousGenericOutput<uint64_t> > (*SGO));
					
					for ( uint64_t b = 0; b < numblocks; ++b )
					{
						uint64_t const blow = std::min(b*blocksize,n);
						uint64_t const bhigh = std::min(blow+blocksize,n);
						uint64_t const bcnt = bhigh-blow;

						uint64_t const pos = SGO->getWrittenBytes();
						
						GE->encodeWord(bcnt,32);
						uint64_t vacc = 0;
						
						for ( uint64_t i = 0; i < bcnt; ++i )
						{
							uint64_t v = 0;
							for ( uint64_t j = 0; j < GGD.size(); ++j )
								v += GGD[j]->decode();
							vacc += v;
							GE->encode(v);
						}
						GE->flush();

						::libmaus::huffman::IndexEntry const entry(pos+fslen,bcnt,vacc);
						GGE->index.push_back(entry);						
					}
					
					GE.reset();
					SGO->flush();
					uint64_t const indexpos = fslen + SGO->getWrittenBytes();
					SGO.reset();
					
					GGE->writeIndex(indexpos);
					GGE->COS.flush();
					GGE.reset();

					for ( uint64_t i = 0; i < infilenames.size(); ++i )
						GGD[i].reset();
						
					bool const check = true;
					if ( check )
					{
						for ( uint64_t i = 0; i < infilenames.size(); ++i )
							GGD[i] = UNIQUE_PTR_MOVE(GammaGapDecoder::unique_ptr_type(new GammaGapDecoder(infilenames[i])));
						GammaGapDecoder OGGD(std::vector<std::string>(1,outfilename));
						
						for ( uint64_t i = 0; i < n; ++i )
						{
							uint64_t v = 0;
							for ( uint64_t j = 0; j < infilenames.size(); ++j )
								v += GGD[j]->decode();
							
							uint64_t const vv = OGGD.decode();
							
							if ( v != vv )
							{
								::libmaus::exception::LibMausException se;
								se.getStream() << "Merging failed in GammaGapEncoder::merge()";
								se.finish();
								throw se;
							}
						}
						
						std::cerr << "[D] merge succesfull." << std::endl;
					}
				}
			}                                                                                        
		};
	}
}
#endif
