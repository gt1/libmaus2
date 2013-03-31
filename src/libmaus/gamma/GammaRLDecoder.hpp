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

#if ! defined(LIBMAUS_GAMMA_GAMMARLDECODER_HPP)
#define LIBMAUS_GAMMA_GAMMARLDECODER_HPP

#include <libmaus/bitio/BitIOInput.hpp>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/bitio/readElias.hpp>
#include <libmaus/huffman/CanonicalEncoder.hpp>
#include <libmaus/util/GetFileSize.hpp>
#include <libmaus/util/IntervalTree.hpp>
#include <libmaus/huffman/IndexDecoderDataArray.hpp>

namespace libmaus
{
	namespace gamma
	{
		struct GammaRLDecoder
		{
			typedef GammaRLDecoder this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			
			::libmaus::huffman::IndexDecoderDataArray::unique_ptr_type Pidda;
			::libmaus::huffman::IndexDecoderDataArray const & idda;

			typedef std::pair<uint64_t,uint64_t> rl_pair;
			::libmaus::autoarray::AutoArray < rl_pair > rlbuffer;

			rl_pair * pa;
			rl_pair * pc;
			rl_pair * pe;

			::libmaus::aio::CheckedInputStream::unique_ptr_type CIS;
			::libmaus::aio::SynchronousGenericInput<uint64_t>::unique_ptr_type SGI;
			::libmaus::gamma::GammaDecoder< ::libmaus::aio::SynchronousGenericInput<uint64_t> >::unique_ptr_type GD;

			uint64_t fileptr;
			uint64_t blockptr;
			unsigned int albits;

			uint64_t getN() const
			{
				if ( idda.vvec.size() )
					return idda.vvec[idda.vvec.size()-1];
				else
					return 0;
			}

			// get length of file in symbols
			static uint64_t getLength(std::string const & filename)
			{
				::libmaus::aio::CheckedInputStream CIS(filename);
				::libmaus::aio::SynchronousGenericInput<uint64_t> SGI(CIS,64);
				return SGI.get();
			}
			
			// get length of file in symbols
			static unsigned int getAlBits(std::string const & filename)
			{
				::libmaus::aio::CheckedInputStream CIS(filename);
				::libmaus::aio::SynchronousGenericInput<uint64_t> SGI(CIS,64);
				SGI.get(); // file length
				return SGI.get();
			}
			
			// get length of vector of files in symbols
			static uint64_t getLength(std::vector<std::string> const & filenames)
			{
				uint64_t s = 0;
				for ( uint64_t i = 0; i < filenames.size(); ++i )
					s += getLength(filenames[i]);
				return s;
			}

			bool openNewFile()
			{
				if ( fileptr < idda.data.size() ) // file ptr valid, does file exist?
				{
					assert ( blockptr < idda.data[fileptr].numentries ); // check block pointer
					
					albits = getAlBits(idda.data[fileptr].filename);

					// open new input file stream
					CIS = UNIQUE_PTR_MOVE(
						::libmaus::aio::CheckedInputStream::unique_ptr_type(
							new ::libmaus::aio::CheckedInputStream(idda.data[fileptr].filename)
						)
					);
					
					// seek to position and check if we succeeded
					CIS->seekg(idda.data[fileptr].getPos(blockptr),std::ios::beg);

					SGI = UNIQUE_PTR_MOVE(
						::libmaus::aio::SynchronousGenericInput<uint64_t>::unique_ptr_type(
							new ::libmaus::aio::SynchronousGenericInput<uint64_t>(*CIS,8*1024,
								std::numeric_limits<uint64_t>::max() /* total words */,false /* checkmod */
							)
						)
					);
					
					GD = UNIQUE_PTR_MOVE(
						::libmaus::gamma::GammaDecoder< ::libmaus::aio::SynchronousGenericInput<uint64_t> >::unique_ptr_type(
							new ::libmaus::gamma::GammaDecoder< ::libmaus::aio::SynchronousGenericInput<uint64_t> >(*SGI)
						)
					);

					return true;
				}
				else
				{
					return false;
				}
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
			
				// read block size
				uint64_t const bs = GD->decode();
				
				// increase buffersize if necessary
				if ( bs > rlbuffer.size() )
					rlbuffer = ::libmaus::autoarray::AutoArray < rl_pair >(bs,false);
				
				// set up pointers
				pa = rlbuffer.begin();
				pc = pa;
				pe = pc + bs;
				
				// decode symbols
				for ( uint64_t i = 0; i < bs; ++i )
				{
					uint64_t const sym = GD->decodeWord(albits);
					uint64_t const rl  = GD->decode();
					rlbuffer[i] = rl_pair(sym,rl);
				}

				// byte align
				GD->flush();

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
			
			inline int get()
			{
				return decode();
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
			
			GammaRLDecoder(
				std::vector<std::string> const & rfilenames, uint64_t offset = 0
			)
			: 
			  Pidda(UNIQUE_PTR_MOVE(::libmaus::huffman::IndexDecoderDataArray::construct(rfilenames))),
			  idda(*Pidda),
			  pa(0), pc(0), pe(0),
			  fileptr(0), blockptr(0)
			{
				init(offset);
			}

			GammaRLDecoder(
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
		};
	}
}
#endif
