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

#if ! defined(LIBMAUS_GAMMA_GAMMARLDECODER_HPP)
#define LIBMAUS_GAMMA_GAMMARLDECODER_HPP

#include <libmaus/bitio/BitIOInput.hpp>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/bitio/readElias.hpp>
#include <libmaus/huffman/CanonicalEncoder.hpp>
#include <libmaus/util/GetFileSize.hpp>
#include <libmaus/util/IntervalTree.hpp>
#include <libmaus/huffman/IndexDecoderDataArray.hpp>
#include <libmaus/huffman/IndexLoader.hpp>
#include <libmaus/gamma/GammaDecoder.hpp>
#include <libmaus/aio/SynchronousGenericInput.hpp>

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

			::libmaus::huffman::IndexEntryContainerVector const * iecv;

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
			
			uint64_t bufsize;

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
				::libmaus::aio::SynchronousGenericInput<uint64_t> SGI(CIS,64,
					std::numeric_limits<uint64_t>::max() /* total words */,false /* checkmod */
				);
				return SGI.get();
			}
			
			// get alphabet bits
			static unsigned int getAlBits(std::string const & filename)
			{
				::libmaus::aio::CheckedInputStream CIS(filename);
				::libmaus::aio::SynchronousGenericInput<uint64_t> SGI(CIS,64,
					std::numeric_limits<uint64_t>::max() /* total words */,false /* checkmod */
				);
				SGI.get(); // file length
				return SGI.get();
			}

			// get alphabet bits
			static unsigned int getAlBits(std::vector<std::string> const & filenames)
			{
				if ( ! filenames.size() )
					return 0;
					
				unsigned int const albits_0 = getAlBits(filenames[0]);
				
				for ( uint64_t i = 1; i < filenames.size(); ++i )
				{
					unsigned int const albits_i = getAlBits(filenames[i]);
					assert ( albits_i == albits_0 );
				}
				
				return albits_0;
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
					::libmaus::aio::CheckedInputStream::unique_ptr_type tCIS(
                                                        new ::libmaus::aio::CheckedInputStream(idda.data[fileptr].filename)
                                                );
					CIS = UNIQUE_PTR_MOVE(tCIS);
					
					// seek to position and check if we succeeded
					if ( iecv )
						CIS->seekg(iecv->A[fileptr]->index[blockptr].pos,std::ios::beg);
					else
						CIS->seekg(idda.data[fileptr].getPos(blockptr),std::ios::beg);

					::libmaus::aio::SynchronousGenericInput<uint64_t>::unique_ptr_type tSGI(
                                                        new ::libmaus::aio::SynchronousGenericInput<uint64_t>(*CIS,bufsize,
                                                                std::numeric_limits<uint64_t>::max() /* total words */,false /* checkmod */
                                                        )
                                                );
					SGI = UNIQUE_PTR_MOVE(tSGI);
					
					::libmaus::gamma::GammaDecoder< ::libmaus::aio::SynchronousGenericInput<uint64_t> >::unique_ptr_type tGD(
                                                        new ::libmaus::gamma::GammaDecoder< ::libmaus::aio::SynchronousGenericInput<uint64_t> >(*SGI)
                                                );
					GD = UNIQUE_PTR_MOVE(tGD);

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
			
			inline void putBack(std::pair<int64_t,uint64_t> const & P)
			{
				*(--pc) = P;
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
					uint64_t restoffset;
				
					if ( iecv )
					{
						std::pair<uint64_t,uint64_t> P = iecv->lookupValue(offset);
						fileptr = P.first;
						blockptr = P.second;
						restoffset = offset - iecv->A[fileptr]->index[blockptr].vcnt;
					}
					else
					{
						::libmaus::huffman::FileBlockOffset const FBO = idda.findVBlock(offset);
						fileptr = FBO.fileptr;
						blockptr = FBO.blockptr;
						restoffset = FBO.offset;
					}
					
					#if 0
					if ( iecv )
					{
						std::pair<uint64_t,uint64_t> P2 = iecv->lookupValue(offset);
						
						uint64_t const fileptr2 = P2.first;
						uint64_t const blockptr2 = P2.second;
						uint64_t const restoffset2 = 
							offset - iecv->A[fileptr2]->index[blockptr2].vcnt;
							
						bool const ok =
							(fileptr==fileptr2)
							&&
							(blockptr==blockptr2)
							&&
							(restoffset2 == restoffset);
							
						if ( ok )
						{
							std::cerr << "***OK****" << std::endl;
						}
						else
						{
							std::cerr << "***FAILURE: "
								<< " fileptr=" << fileptr << " fileptr2=" << fileptr2 
								<< " blockptr=" << blockptr << " blockptr2=" << blockptr2 
								<< " restoffset=" << restoffset << " restoffset2=" << restoffset2
								<< std::endl; 
						}
					}
					#endif
										
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
				std::vector<std::string> const & rfilenames, uint64_t offset = 0, uint64_t const rbufsize = 64*1024
			)
			: 
			  Pidda(::libmaus::huffman::IndexDecoderDataArray::construct(rfilenames)),
			  idda(*Pidda),
			  iecv(0),
			  pa(0), pc(0), pe(0),
			  fileptr(0), blockptr(0),
			  bufsize(rbufsize)
			{
				init(offset);
			}

			GammaRLDecoder(
				::libmaus::huffman::IndexDecoderDataArray const & ridda,
				uint64_t offset = 0,
				uint64_t const rbufsize = 64*1024
			)
			:
			  Pidda(),
			  idda(ridda), 
			  iecv(0),
			  pa(0), pc(0), pe(0),
			  fileptr(0), blockptr(0),
			  bufsize(rbufsize)
			{
				init(offset);
			}			

			GammaRLDecoder(
				::libmaus::huffman::IndexDecoderDataArray const & ridda,
				::libmaus::huffman::IndexEntryContainerVector const * riecv,
				uint64_t offset = 0,
				uint64_t const rbufsize = 64*1024
			)
			:
			  Pidda(),
			  idda(ridda), 
			  iecv(riecv),
			  pa(0), pc(0), pe(0),
			  fileptr(0), blockptr(0),
			  bufsize(rbufsize)
			{
				init(offset);
			}			
		};
	}
}
#endif
