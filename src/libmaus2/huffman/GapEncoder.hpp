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

#if ! defined(GAPENCODER_HPP)
#define GAPENCODER_HPP

#include <libmaus2/huffman/CanonicalEncoder.hpp>
#include <libmaus2/huffman/HuffmanEncoderFile.hpp>
#include <libmaus2/huffman/GapDecoder.hpp>
#include <libmaus2/util/Histogram.hpp>
#include <libmaus2/math/bitsPerNum.hpp>
#include <libmaus2/huffman/PairAddSecond.hpp>

namespace libmaus2
{
	namespace huffman
	{
		struct GapEncoder
		{
			std::string const filename;
			bool const needescape;
			::libmaus2::huffman::HuffmanEncoderFile gapHEF;
			::libmaus2::huffman::CanonicalEncoder::unique_ptr_type GCE;
			::libmaus2::huffman::EscapeCanonicalEncoder::unique_ptr_type GECE;
			// (block position, sum in block)
			std::vector < IndexEntry > index;
			
			GapEncoder(
				std::string const & rfilename,
				::libmaus2::util::Histogram & gaphist,
				uint64_t const numentries
			)
			: filename(rfilename), 
			  needescape(::libmaus2::huffman::EscapeCanonicalEncoder::needEscape(gaphist.getFreqSymVector())), 
			  gapHEF(filename)
			{
				// std::cerr << "need escape: " << needescape << std::endl;
			
				if ( needescape )
				{
					::libmaus2::huffman::EscapeCanonicalEncoder::unique_ptr_type tGECE(new ::libmaus2::huffman::EscapeCanonicalEncoder(gaphist.getFreqSymVector()));
					GECE = UNIQUE_PTR_MOVE(tGECE);
				}
				else
				{
					::libmaus2::huffman::CanonicalEncoder::unique_ptr_type tGCE(new ::libmaus2::huffman::CanonicalEncoder(gaphist.getByType<int64_t>()));
					GCE = UNIQUE_PTR_MOVE(tGCE);
				}

				gapHEF.writeBit(needescape);
				gapHEF.writeElias2(numentries);
	
				if ( needescape )
					GECE->serialise(gapHEF);
				else
				{
					GCE->serialise(gapHEF);
					GCE->setupEncodeTable(1024);
				}				
			}
			
			void flush()
			{
				gapHEF.flush();		
			}
			
			void writeIndex()
			{
				gapHEF.flush();

				// get position of index
				uint64_t const indexpos = gapHEF.getPos();

				uint64_t const maxpos = index.size() ? index[index.size()-1].pos : 0;
				unsigned int const posbits = ::libmaus2::math::bitsPerNum(maxpos);
				
				uint64_t const kacc = std::accumulate(index.begin(),index.end(),0ull,IndexEntryKeyAdd());
				unsigned int const kbits = ::libmaus2::math::bitsPerNum(kacc);

				uint64_t const vacc = std::accumulate(index.begin(),index.end(),0ull,IndexEntryValueAdd());
				unsigned int const vbits = ::libmaus2::math::bitsPerNum(vacc);

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
				gapHEF.flushBitStream();
				
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
				gapHEF.flushBitStream();
			
				// write position of index in last 64 bits of file	
				for ( uint64_t i = 0; i < 64; ++i )
					gapHEF.writeBit( (indexpos & (1ull<<(63-i))) != 0 );

				gapHEF.flush();				
			}

			template<typename iterator, typename encoder_type>
			void encode(iterator ita, uint64_t const n, encoder_type * encoder)
			{
				uint64_t const blocksize = 32*1024;
				uint64_t const blocks = (n + blocksize-1)/blocksize;
				
				for ( uint64_t b = 0; b < blocks; ++b )
				{
					gapHEF.flushBitStream();
					uint64_t const pos = gapHEF.getPos();
					
					uint64_t const blow = std::min(b*blocksize,n);
					uint64_t const bhigh = std::min(blow+blocksize,n);
					uint64_t const bsize = bhigh-blow;
					gapHEF.writeElias2(bsize);
					gapHEF.flushBitStream();
					
					uint64_t sum = 0;
					for ( uint64_t i = 0; i < bsize; ++i )
					{
						uint64_t const v = *(ita++);
						encoder->encode(gapHEF,v);
						sum += v;
					}
					
					index.push_back(IndexEntry(pos,bsize,sum));
					
					gapHEF.flushBitStream();
				}
				
			}

			template<typename iterator, typename encoder_type>
			void encodeFast(iterator ita, uint64_t const n, encoder_type * encoder)
			{
				uint64_t const blocksize = 32*1024;
				uint64_t const blocks = (n + blocksize-1)/blocksize;
				
				for ( uint64_t b = 0; b < blocks; ++b )
				{
					gapHEF.flushBitStream();
					uint64_t const pos = gapHEF.getPos();
					
					uint64_t const blow = std::min(b*blocksize,n);
					uint64_t const bhigh = std::min(blow+blocksize,n);
					uint64_t const bsize = bhigh-blow;
					gapHEF.writeElias2(bsize);
					gapHEF.flushBitStream();
					
					uint64_t sum = 0;
					for ( uint64_t i = 0; i < bsize; ++i )
					{
						uint64_t const v = *(ita++);
						encoder->encodeFast(gapHEF,v);
						sum += v;
					}
					
					index.push_back(IndexEntry(pos,bsize,sum));
					
					gapHEF.flushBitStream();
				}
			}

			template<typename iterator>
			void encodeInternal(iterator ita, uint64_t const n)
			{
				if ( needescape )
					encode(ita,n,GECE.get());
				else
					encodeFast(ita,n,GCE.get());
			}

			template<typename iterator>
			void encode(iterator ita, iterator ite)
			{
				encodeInternal(ita,ite-ita);
				writeIndex();
				flush();
			}

			template<typename iterator>
			void encode(iterator ita, uint64_t const n)
			{
				encodeInternal(ita,n);
				writeIndex();
				flush();
			}

			// merge multiple gap arrays to one by adding them up per rank
			static void merge(
				std::vector < std::vector<std::string> > const & infilenames, 
				std::string const & outfilename
			)
			{
				if ( ! infilenames.size() )
				{				
					::libmaus2::util::Histogram hist;
					GapEncoder enc(outfilename,hist,0);
					enc.flush();
				}
				else
				{
					typedef GapDecoder decoder_type;
					typedef decoder_type::unique_ptr_type decoder_ptr_type;
					
					::libmaus2::util::Histogram hist;
					::libmaus2::autoarray::AutoArray<decoder_ptr_type> decoders(infilenames.size());

					// check length of files
					uint64_t const n = infilenames.size() ? GapDecoder::getLength(infilenames[0]) : 0;
					for ( uint64_t i = 1; i < infilenames.size(); ++i )
						assert ( GapDecoder::getLength(infilenames[i]) == n );

					// set up decoders
					for ( uint64_t i = 0; i < infilenames.size(); ++i )
					{
						// std::cerr << "Setting up decoder for " << infilenames[i] << std::endl;
						for ( uint64_t j = 0; j < infilenames[i].size(); ++j )
							assert ( ::libmaus2::util::GetFileSize::fileExists(infilenames[i][j]) );
						
						decoder_ptr_type tdecodersi(new decoder_type(infilenames[i]));
						decoders[i] = UNIQUE_PTR_MOVE(tdecodersi);
					}

					// compute histogram for output
					for ( uint64_t i = 0; i < n; ++i )
					{
						uint64_t v = 0;
						for ( uint64_t j = 0; j < decoders.size(); ++j )
							v += decoders[j]->decode();
						hist(v);
					}
					
					// hist.printType<uint64_t>(std::cerr);
						
					// set up encoder
					GapEncoder enc(outfilename,hist,n);

					// set up decoders
					for ( uint64_t i = 0; i < infilenames.size(); ++i )
					{
						decoder_ptr_type tdecodersi(new decoder_type(infilenames[i]));
						decoders[i] = UNIQUE_PTR_MOVE(tdecodersi);
					}
					
					uint64_t const bs = 32*1024;
					uint64_t const blocks = ( n + bs - 1 ) / bs;
					::libmaus2::autoarray::AutoArray<uint64_t> B(bs,false);

					for ( uint64_t b = 0; b < blocks; ++b )
					{
						uint64_t const blow = std::min(b * bs,n);
						uint64_t const bhigh = std::min(blow+bs,n);
						uint64_t const blen = bhigh-blow;

						std::fill ( B.begin(), B.begin() + blen , 0ull );
						
						for ( uint64_t i = 0; i < blen; ++i )
							for ( uint64_t j = 0; j < decoders.size(); ++j )
								B[i] += decoders[j]->decode();
								
						#if 0
						uint64_t maxval = 0;
						for ( uint64_t i = 0; i < blen; ++i )
							maxval = std::max(maxval,B[i]);
						
						if ( maxval > 64*1024 )
							std::cerr << "maxval = " << maxval << std::endl;
						#endif
								
						enc.encodeInternal(B.begin(),blen /*B.begin() + blen*/);
					}
	
					enc.writeIndex();
					enc.flush();

					#if 1
					bool const checkit = true;
					
					if ( checkit )
					{
						std::cerr << "Checking merged...";
						// set up decoders
						for ( uint64_t i = 0; i < infilenames.size(); ++i )
						{
							decoder_ptr_type tdecodersi(new decoder_type(infilenames[i]));
							decoders[i] = UNIQUE_PTR_MOVE(tdecodersi);
						}
						GapDecoder wdec(std::vector<std::string>(1,outfilename));
						
						for ( uint64_t i = 0; i < n; ++i )
						{
							uint64_t vr = 0;
							for ( uint64_t j = 0; j < decoders.size(); ++j )
								vr += decoders[j]->decode();
							uint64_t const vn = wdec.decode();
							assert ( vr == vn );
						}
						std::cerr << "done." << std::endl;
					}
					#endif
				}
			}

		};
	}
}
#endif
