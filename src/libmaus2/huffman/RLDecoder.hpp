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

#if ! defined(RLDECODER_HPP)
#define RLDECODER_HPP

#include <libmaus/aio/CheckedInputStream.hpp>
#include <libmaus/bitio/BitIOInput.hpp>
#include <libmaus/bitio/readElias.hpp>
#include <libmaus/huffman/CanonicalEncoder.hpp>
#include <libmaus/huffman/IndexDecoderDataArray.hpp>
#include <libmaus/util/HistogramSet.hpp>

namespace libmaus
{
	namespace huffman
	{
		struct RLDecoder
		{
			typedef RLDecoder this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			
			::libmaus::huffman::IndexDecoderDataArray::unique_ptr_type Pidda;
			::libmaus::huffman::IndexDecoderDataArray const & idda;

			typedef std::pair<uint64_t,uint64_t> rl_pair;
			::libmaus::autoarray::AutoArray < rl_pair > rlbuffer;

			rl_pair * pa;
			rl_pair * pc;
			rl_pair * pe;

			::libmaus::util::unique_ptr<std::ifstream>::type istr;

			#if defined(SLOWDEC)
			::libmaus::bitio::StreamBitInputStream::unique_ptr_type SBIS;
			#else
			typedef ::libmaus::huffman::BitInputBuffer4 sbis_type;			
			sbis_type::unique_ptr_type SBIS;
			#endif
			
			uint64_t fileptr;
			uint64_t blockptr;

			bool openNewFile()
			{
				if ( fileptr < idda.data.size() ) // file ptr valid, does file exist?
				{
					assert ( blockptr < idda.data[fileptr].numentries ); // check block pointer

					// open new input file stream
					::libmaus::util::unique_ptr<std::ifstream>::type tistr(
                                                new std::ifstream(idda.data[fileptr].filename.c_str(),std::ios::binary));
					istr = UNIQUE_PTR_MOVE(tistr);
				
					// check whether file is open
					if ( ! istr->is_open() )
					{
						::libmaus::exception::LibMausException se;
						se.getStream() << "RLDecoder::openNewFile(): Failed to open file " 
							<< idda.data[fileptr].filename << std::endl;
						se.finish();
						throw se;
					}

					// seek to position and check if we succeeded
					istr->clear();
					istr->seekg(idda.data[fileptr].getPos(blockptr),std::ios::beg);
					
					if ( static_cast<int64_t>(istr->tellg()) != static_cast<int64_t>(idda.data[fileptr].getPos(blockptr)) )
					{
						::libmaus::exception::LibMausException se;
						se.getStream() << "RLDecoder::openNewFile(): Failed to seek in file " 
							<< idda.data[fileptr].filename << std::endl;
						se.finish();
						throw se;
					}

					// set up bit input
					sbis_type::raw_input_ptr_type ript(new sbis_type::raw_input_type(*istr));
					sbis_type::unique_ptr_type tSBIS(
                                                        new sbis_type(ript,static_cast<uint64_t>(64*1024))
                                                );
					SBIS = UNIQUE_PTR_MOVE(tSBIS);

					return true;
				}
				else
				{
					return false;
				}
			}
			
			uint64_t getN() const
			{
				if ( idda.vvec.size() )
					return idda.vvec[idda.vvec.size()-1];
				else
					return 0;
			}
			
			// init by position in stream
			void init(uint64_t offset)
			{
				if ( offset < idda.vvec[idda.vvec.size()-1] )
				{
					::libmaus::huffman::FileBlockOffset const FBO = idda.findVBlock(offset);
					fileptr = FBO.fileptr;
					blockptr = FBO.blockptr;
					uint64_t restoffset = FBO.offset;
										
					openNewFile();
					
					// this would be quicker using run lengths
					while ( restoffset )
					{
						decode();
						--restoffset;
					}
				}			
			}
			
			RLDecoder(
				std::vector<std::string> const & rfilenames, uint64_t offset = 0
			)
			: 
			  Pidda(::libmaus::huffman::IndexDecoderDataArray::construct(rfilenames)),
			  idda(*Pidda),
			  pa(0), pc(0), pe(0),
			  fileptr(0), blockptr(0)
			{
				init(offset);
			}

			RLDecoder(
				::libmaus::huffman::IndexDecoderDataArray const & ridda,
				uint64_t offset = 0
			)
			:
			  Pidda(),
			  idda(ridda), 
			  pa(0), pc(0), pe(0),
			  fileptr(0), blockptr(0)
			{
				init(offset);
			}
			
			bool fillBuffer()
			{
				bool newfile = false;
				
				// check if we need to open a new file
				while ( fileptr < idda.data.size() && blockptr == idda.data[fileptr].numentries )
				{
					fileptr++;
					blockptr = 0;
					newfile = true;
				}
				// we have reached the end, no more blocks
				if ( fileptr == idda.data.size() )
					return false;
				// open new file if necessary
				if ( newfile )
					openNewFile();
			
				// byte align stream
				SBIS->flush();
			
				// read block size
				uint64_t const bs = ::libmaus::bitio::readElias2(*SBIS);
				bool const cntescape = SBIS->readBit();

				// read huffman code maps
				::libmaus::autoarray::AutoArray< std::pair<int64_t, uint64_t> > symmap = ::libmaus::huffman::CanonicalEncoder::deserialise(*SBIS);
				::libmaus::autoarray::AutoArray< std::pair<int64_t, uint64_t> > cntmap = ::libmaus::huffman::CanonicalEncoder::deserialise(*SBIS);
				
				// construct decoder for symbols
				::libmaus::huffman::CanonicalEncoder symdec(symmap);
				
				// construct decoder for runlengths
				::libmaus::huffman::EscapeCanonicalEncoder::unique_ptr_type esccntdec;
				::libmaus::huffman::CanonicalEncoder::unique_ptr_type cntdec;
				if ( cntescape )
				{
					::libmaus::huffman::EscapeCanonicalEncoder::unique_ptr_type tesccntdec(new ::libmaus::huffman::EscapeCanonicalEncoder(cntmap));
					esccntdec = UNIQUE_PTR_MOVE(tesccntdec);
				}
				else
				{
					::libmaus::huffman::CanonicalEncoder::unique_ptr_type tcntdec(new ::libmaus::huffman::CanonicalEncoder(cntmap));
					cntdec = UNIQUE_PTR_MOVE(tcntdec);
				}
				
				// increase buffersize if necessary
				if ( bs > rlbuffer.size() )
					rlbuffer = ::libmaus::autoarray::AutoArray < rl_pair >(bs,false);
				
				// set up pointers
				pa = rlbuffer.begin();
				pc = pa;
				pe = pc + bs;
				
				// byte align input stream
				SBIS->flush();

				// decode symbols
				for ( uint64_t i = 0; i < bs; ++i )
				{
					uint64_t const sym = symdec.fastDecode(*SBIS);
					rlbuffer[i].first = sym;
				}

				// byte align
				SBIS->flush();

				// decode runlengths
				if ( cntescape )
					for ( uint64_t i = 0; i < bs; ++i )
					{
						uint64_t const cnt = esccntdec->fastDecode(*SBIS);
						rlbuffer[i].second = cnt;
					}
				else
					for ( uint64_t i = 0; i < bs; ++i )
					{
						uint64_t const cnt = cntdec->fastDecode(*SBIS);
						rlbuffer[i].second = cnt;
					}

				// byte align
				SBIS->flush();
			
				// next block
				blockptr++;
			
				return true;
			}
			
			inline int decode()
			{
				if ( pc == pe )
				{
					fillBuffer();
					if ( pc == pe )
						return -1;
				}
				assert ( pc->second );
				int const sym = pc->first;
				if ( ! --(pc->second) )
					pc++;
				return sym;
			}

			inline std::pair<int64_t,uint64_t> decodeRun()
			{
				if ( pc == pe )
				{
					fillBuffer();
					if ( pc == pe )
						return std::pair<int64_t,uint64_t>(-1,0);
				}
				assert ( pc->second );
				int64_t const sym = pc->first;
				uint64_t const freq = pc->second;
				++pc;
				return std::pair<int64_t,uint64_t>(sym,freq);
			}
			
			inline void putBack(std::pair<int64_t,uint64_t> const & P)
			{
				*(--pc) = P;
			}
			
			inline int get()
			{
				return decode();
			}

			// get length of file in symbols
			static uint64_t getLength(std::string const & filename)
			{
				std::ifstream istr(filename.c_str(),std::ios::binary);
				assert ( istr.is_open() );
				::libmaus::bitio::StreamBitInputStream SBIS(istr);	
				// SBIS.readBit(); // need escape
				return ::libmaus::bitio::readElias2(SBIS);
			}
			
			// get length of vector of files in symbols
			static uint64_t getLength(std::vector<std::string> const & filenames)
			{
				uint64_t s = 0;
				for ( uint64_t i = 0; i < filenames.size(); ++i )
					s += getLength(filenames[i]);
				return s;
			}
			
			// compute run length histogram for run length values stored in file given by name
			static libmaus::util::Histogram::unique_ptr_type getRunLengthHistogram(std::string const & filename)
			{
				libmaus::huffman::IndexDecoderData IDD(filename);
				
				#if defined(_OPENMP)
				uint64_t const numthreads = omp_get_max_threads();
				#else
				uint64_t const numthreads = 1;
				#endif
				
				libmaus::util::HistogramSet HS(numthreads,256);
				
				uint64_t const numentries = IDD.numentries;
				uint64_t const entriesperthread = (numentries + numthreads - 1)/numthreads;
				uint64_t const numpacks = ( numentries + entriesperthread - 1 ) / entriesperthread;
				
				#if defined(_OPENMP)
				#pragma omp parallel for
				#endif
				for ( int64_t t = 0; t < static_cast<int64_t>(numpacks); ++t )
				{
					uint64_t const plow = t * entriesperthread;
					uint64_t const phigh = std::min(plow+entriesperthread,numentries);
					libmaus::util::Histogram & hist = HS[t];

					libmaus::huffman::IndexEntry const ientry = IDD.readEntry(plow);
					
					libmaus::aio::CheckedInputStream CIS(filename);
					CIS.clear();
					CIS.seekg(ientry.pos,std::ios::beg);					

					// set up bit input
					sbis_type::raw_input_ptr_type ript(new sbis_type::raw_input_type(CIS));
					sbis_type::unique_ptr_type SBIS(new sbis_type(ript,static_cast<uint64_t>(64*1024)));
					
					for ( uint64_t b = plow; b < phigh; ++b )
					{					
						// byte align stream
						SBIS->flush();
					
						// read block size
						uint64_t const bs = ::libmaus::bitio::readElias2(*SBIS);
						bool const cntescape = SBIS->readBit();

						// read huffman code maps
						::libmaus::autoarray::AutoArray< std::pair<int64_t, uint64_t> > symmap = ::libmaus::huffman::CanonicalEncoder::deserialise(*SBIS);
						::libmaus::autoarray::AutoArray< std::pair<int64_t, uint64_t> > cntmap = ::libmaus::huffman::CanonicalEncoder::deserialise(*SBIS);
						
						// construct decoder for symbols
						::libmaus::huffman::CanonicalEncoder symdec(symmap);
						
						// construct decoder for runlengths
						::libmaus::huffman::EscapeCanonicalEncoder::unique_ptr_type esccntdec;
						::libmaus::huffman::CanonicalEncoder::unique_ptr_type cntdec;
						if ( cntescape )
						{
							::libmaus::huffman::EscapeCanonicalEncoder::unique_ptr_type tesccntdec(new ::libmaus::huffman::EscapeCanonicalEncoder(cntmap));
							esccntdec = UNIQUE_PTR_MOVE(tesccntdec);
						}
						else
						{
							::libmaus::huffman::CanonicalEncoder::unique_ptr_type tcntdec(new ::libmaus::huffman::CanonicalEncoder(cntmap));
							cntdec = UNIQUE_PTR_MOVE(tcntdec);
						}
						
						// byte align input stream
						SBIS->flush();

						// decode symbols
						for ( uint64_t i = 0; i < bs; ++i )
							symdec.fastDecode(*SBIS);

						// byte align
						SBIS->flush();

						// decode runlengths
						if ( cntescape )
							for ( uint64_t i = 0; i < bs; ++i )
							{
								uint64_t const cnt = esccntdec->fastDecode(*SBIS);
								hist(cnt);
							}
						else
							for ( uint64_t i = 0; i < bs; ++i )
							{
								uint64_t const cnt = cntdec->fastDecode(*SBIS);
								hist(cnt);
							}

						// byte align
						SBIS->flush();
					}
				}
				
				libmaus::util::Histogram::unique_ptr_type tptr(HS.merge());
				return UNIQUE_PTR_MOVE(tptr);
			}
		};
	}
}
#endif
