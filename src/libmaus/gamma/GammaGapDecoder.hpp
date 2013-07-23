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

#if ! defined(LIBMAUS_GAMMA_GAMMAGAPDECODER_HPP)
#define LIBMAUS_GAMMA_GAMMAGAPDECODER_HPP

#include <fstream>
#include <libmaus/huffman/IndexDecoderDataArray.hpp>
#include <libmaus/huffman/KvInitResult.hpp>
#include <libmaus/gamma/GammaDecoder.hpp>
#include <libmaus/aio/CheckedInputStream.hpp>
#include <libmaus/aio/SynchronousGenericInput.hpp>

namespace libmaus
{
	namespace gamma
	{		
		struct GammaGapDecoder
		{
			typedef GammaGapDecoder this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			::libmaus::huffman::IndexDecoderDataArray::unique_ptr_type const Pidda;
			::libmaus::huffman::IndexDecoderDataArray const & idda;
			
			::libmaus::aio::CheckedInputStream::unique_ptr_type istr;
			::libmaus::aio::SynchronousGenericInput<uint64_t>::unique_ptr_type SGI;
			::libmaus::gamma::GammaDecoder< ::libmaus::aio::SynchronousGenericInput<uint64_t> >::unique_ptr_type GD;

			::libmaus::autoarray::AutoArray<uint64_t, ::libmaus::autoarray::alloc_type_c > decodebuf;
			uint64_t * pa;
			uint64_t * pc;
			uint64_t * pe;

			uint64_t fileptr;
			uint64_t blockptr;

			void openNewFile()
			{
				if ( fileptr < idda.data.size() && blockptr < idda.data[fileptr].numentries )
				{
					/* open file */
					istr = UNIQUE_PTR_MOVE(
						::libmaus::aio::CheckedInputStream::unique_ptr_type(
							new ::libmaus::aio::CheckedInputStream(idda.data[fileptr].filename)
						)
					);

					/* seek to block */
					uint64_t const pos = idda.data[fileptr].getPos(blockptr);
					
					istr->seekg ( pos , std::ios::beg );
					
					if ( static_cast<int64_t>(istr->tellg()) != static_cast<int64_t>(pos) )
					{
						::libmaus::exception::LibMausException ex;
						ex.getStream() << "Failed to seek to position " << pos << " in file " << idda.data[fileptr].filename << std::endl;
						ex.finish();
						throw ex;
					}
					
					SGI = UNIQUE_PTR_MOVE(
						::libmaus::aio::SynchronousGenericInput<uint64_t>::unique_ptr_type(
							new ::libmaus::aio::SynchronousGenericInput<uint64_t>(
								*istr,64*1024,
								::std::numeric_limits<uint64_t>::max(),
								false /* do not check for multiples of entity size */
							)
						)
					);
					
					GD = UNIQUE_PTR_MOVE(
						::libmaus::gamma::GammaDecoder< ::libmaus::aio::SynchronousGenericInput<uint64_t> >::unique_ptr_type(
							new ::libmaus::gamma::GammaDecoder< ::libmaus::aio::SynchronousGenericInput<uint64_t> >(*SGI)
						)
					);
				}
			}
			
			uint64_t getN() const
			{
				return idda.kvec.size() ? idda.kvec[idda.kvec.size()-1] : 0;
			}

			/* decode next block */
			bool decodeBlock()
			{
				/* open new file if necessary */
				bool changedfile = false;
				while ( fileptr < idda.data.size() && blockptr == idda.data[fileptr].numentries )
				{
					fileptr++;
					blockptr = 0;
					changedfile = true;
				}
				if ( fileptr == idda.data.size() )
					return false;
				if ( changedfile )
					openNewFile();

				/* align to word boundary */
				GD->flush();
				/* read block size */
				uint64_t const blocksize = GD->decodeWord(32);

				/* increase size of memory buffer if necessary */
				if ( blocksize > decodebuf.size() )
					decodebuf.resize(blocksize);

				/* set buffer pointers */
				pa = decodebuf.begin();
				pc = pa;
				pe = pa + blocksize;

				/* decode block */
				for ( uint64_t i = 0; i < blocksize; ++i )
					decodebuf[i] = GD->decode();

				/* increment block pointer */
				blockptr++;
				
				return true;
			}

			/* decode next symbol */
			uint64_t decode()
			{
				if ( pc == pe )
					decodeBlock();
				assert ( pc != pe );
				return *(pc++);	
			}
			
			/* alias for decode() */
			uint64_t get()
			{
				return decode();
			}
			
			/* peek at next symbol without advancing decode pointer */
			uint64_t peek()
			{
				if ( pc == pe )
					decodeBlock();
				assert ( pc != pe );
				return *pc;				
			}
			
			/* set current symbol to v */
			void adjust(uint64_t const v)
			{
				assert ( pc != pe );
				*pc = v;
			}

			void init(uint64_t offset = 0, uint64_t * psymoffset = 0)
			{
				if ( ((idda.kvec.size()!=0) && (idda.kvec[idda.kvec.size()-1] != 0)) )
				{
					if ( offset >= idda.kvec[idda.kvec.size()-1] )
					{
						fileptr = idda.data.size();
						blockptr = 0;
					}
					else
					{
						::libmaus::huffman::FileBlockOffset const FBO = idda.findKBlock(offset);
						fileptr = FBO.fileptr;
						blockptr = FBO.blockptr;
						offset = FBO.offset;
					
						/* open file and seek to block */
						openNewFile();
						/* decode block in question */
						bool const blockok = decodeBlock();
						assert ( blockok );
						assert ( static_cast<int64_t>(offset) < (pe-pc) );
						
						/* symbol offset of block (sum over elements of previous blocks) */
						uint64_t symoffset = idda.data[FBO.fileptr].getValueCnt(FBO.blockptr);
						/* decode symbols up to offset in block */
						for ( uint64_t i = 0; i < offset; ++i )
							symoffset += decode();
						
						/* store prefix sum if pointer is given */
						if ( psymoffset )
							*psymoffset = symoffset;
					}
				}
			}
			
			void initKV(uint64_t kvtarget, ::libmaus::huffman::KvInitResult & result)
			{
				result = ::libmaus::huffman::KvInitResult();
			
				if ( 
					(
						(idda.kvec.size()!=0) 
						&& 
						(idda.kvec[idda.kvec.size()-1] != 0)
					) 
				)
				{
					if ( 
						kvtarget >= 
						idda.kvec[idda.kvec.size()-1] + idda.vvec[idda.vvec.size()-1]
					)
					{
						fileptr = idda.data.size();
						blockptr = 0;
						
						result.koffset = idda.kvec[idda.kvec.size()-1];
						result.voffset = idda.vvec[idda.vvec.size()-1];
						result.kvoffset = result.koffset + result.voffset;
						result.kvtarget = 0;
					}
					else
					{
						::libmaus::huffman::FileBlockOffset const FBO = idda.findKVBlock(kvtarget);
						fileptr = FBO.fileptr;
						blockptr = FBO.blockptr;
					
						/* open file and seek to block */
						openNewFile();
						/* decode block in question */
						bool const blockok = decodeBlock();
						assert ( blockok );
						
						/* key/symbol offset of block (sum over elements of previous blocks) */
						uint64_t kvoffset = idda.data[FBO.fileptr].getKeyValueCnt(FBO.blockptr);
						uint64_t voffset = idda.data[FBO.fileptr].getValueCnt(FBO.blockptr);
						uint64_t koffset = idda.data[FBO.fileptr].getKeyCnt(FBO.blockptr);
						
						assert ( kvtarget >= kvoffset );
						kvtarget -= kvoffset;
						
						// std::cerr << "fileptr=" << fileptr << " blockptr=" << blockptr << " kvtarget=" << kvtarget << std::endl;
						
						while ( kvtarget >= peek() + 1 )
						{
							uint64_t const gi = decode();
							kvoffset += (gi+1);
							kvtarget -= (gi+1);
							voffset += gi;
							koffset += 1;
						}
						if ( koffset + 1 == getN() && kvtarget >= peek() )
						{
							uint64_t const gi = decode();
							kvoffset += gi;
							kvtarget -= gi;
							voffset  += gi;
							koffset  += 0;
						}
						else
						{
							assert ( pc != pe );
							assert ( kvtarget <= peek() );
							assert ( kvtarget <= *pc );

							*pc -= kvtarget;
						}
						
						result.koffset  = koffset;
						result.voffset  = voffset;
						result.kvoffset = kvoffset;
						result.kvtarget = kvtarget;
					}
				}
			}
			
			GammaGapDecoder(
				::libmaus::huffman::IndexDecoderDataArray const & ridda,
				uint64_t kvtarget,
				::libmaus::huffman::KvInitResult & result 
			)
			:
			  Pidda(),
			  idda(ridda),
			  /* buffer */
			  decodebuf(), pa(0), pc(0), pe(0), 
			  /* file and segment pointers */
			  fileptr(0), blockptr(0)
			{
				initKV(kvtarget, result);
			}

			GammaGapDecoder(
				std::vector<std::string> const & rfilenames,
				uint64_t kvtarget,
				::libmaus::huffman::KvInitResult & result 
			)
			:
			  Pidda(UNIQUE_PTR_MOVE(::libmaus::huffman::IndexDecoderDataArray::construct(rfilenames))),
			  idda(*Pidda),
			  /* buffer */
			  decodebuf(), pa(0), pc(0), pe(0), 
			  /* file and segment pointers */
			  fileptr(0), blockptr(0)
			{
				initKV(kvtarget, result);
			}

			GammaGapDecoder(
				std::vector<std::string> const & rfilenames, 
				uint64_t offset = 0, 
				uint64_t * psymoffset = 0
			)
			:
			  Pidda(UNIQUE_PTR_MOVE(::libmaus::huffman::IndexDecoderDataArray::construct(rfilenames))),
			  idda(*Pidda),
			  /* buffer */
			  decodebuf(), pa(0), pc(0), pe(0), 
			  /* file and segment pointers */
			  fileptr(0), blockptr(0)
			{
				init(offset,psymoffset);
			}

			GammaGapDecoder(
				::libmaus::huffman::IndexDecoderDataArray const & ridda,
				uint64_t offset = 0, 
				uint64_t * psymoffset = 0
			)
			:
			  Pidda(),
			  idda(ridda),
			  /* buffer */
			  decodebuf(), pa(0), pc(0), pe(0), 
			  /* file and segment pointers */
			  fileptr(0), blockptr(0)
			{
				init(offset,psymoffset);
			}
			
			// get length of file in symbols
			static uint64_t getLength(std::string const & filename)
			{
				::libmaus::aio::SynchronousGenericInput<uint64_t> SGI(filename,64);
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

			struct iterator
			{
				GammaGapDecoder * owner;
				uint64_t v;
				
				iterator()
				: owner(0), v(0)
				{
				
				}
				iterator(GammaGapDecoder * rowner)
				: owner(rowner), v(owner->decode())
				{
				
				}
				
				uint64_t operator*() const
				{
					return v;
				}
				
				iterator operator++(int)
				{
					iterator copy = *this;
					v = owner->decode();
					return copy;
				}
				
				iterator operator++()
				{
					v = owner->decode();
					return *this;
				}
			};
			
			iterator begin()
			{
				return iterator(this);
			}
		};
	}
}
#endif
