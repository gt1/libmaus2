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

#if ! defined(GAPDECODER_HPP)
#define GAPDECODER_HPP

#include <fstream>
#include <libmaus2/bitio/BitIOInput.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/bitio/readElias.hpp>
#include <libmaus2/huffman/CanonicalEncoder.hpp>
#include <libmaus2/util/IntervalTree.hpp>
#include <libmaus2/huffman/RLDecoder.hpp>
#include <libmaus2/huffman/IndexDecoderDataArray.hpp>
#include <libmaus2/huffman/KvInitResult.hpp>

namespace libmaus2
{
	namespace huffman
	{
		
		struct GapDecoder
		{
			typedef GapDecoder this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			::libmaus2::huffman::IndexDecoderDataArray::unique_ptr_type const Pidda;
			::libmaus2::huffman::IndexDecoderDataArray const & idda;
			
			::libmaus2::util::unique_ptr<std::ifstream>::type istr;
			typedef ::libmaus2::huffman::BitInputBuffer4 sbis_type;
			typedef sbis_type::unique_ptr_type sbis_ptr_type;
			sbis_ptr_type SBIS;			
			bool needescape;
			
			::libmaus2::huffman::EscapeCanonicalEncoder::unique_ptr_type ECE;
			::libmaus2::huffman::CanonicalEncoder::unique_ptr_type CE;
			
			::libmaus2::autoarray::AutoArray<uint64_t, ::libmaus2::autoarray::alloc_type_c > decodebuf;
			uint64_t * pa;
			uint64_t * pc;
			uint64_t * pe;

			uint64_t fileptr;
			uint64_t blockptr;

			void openNewFile()
			{
				if ( fileptr < idda.data.size() )
				{
					/* open file */
					::libmaus2::util::unique_ptr<std::ifstream>::type tistr(
                                                new std::ifstream(idda.data[fileptr].filename.c_str(),std::ios::binary));
					istr = UNIQUE_PTR_MOVE(tistr);
					
					assert ( istr->is_open() );
						
					/* inst SBIS */
					sbis_type::raw_input_ptr_type ript(new sbis_type::raw_input_type(*istr));
					sbis_ptr_type tSBIS(new sbis_type(ript,64*1024));
					SBIS = UNIQUE_PTR_MOVE(tSBIS);
					/* do we need escape symbols */
					needescape = SBIS->readBit();
					/* read number of entries in file */
					/* n = */ ::libmaus2::bitio::readElias2(*SBIS);
					/* deserialise freqs */
					::libmaus2::autoarray::AutoArray< std::pair<int64_t, uint64_t> > dist =
						::libmaus2::huffman::CanonicalEncoder::deserialise(*SBIS);

					/* instantiate Huffman decoder */
					if ( needescape )
					{
						::libmaus2::huffman::EscapeCanonicalEncoder::unique_ptr_type tECE(new ::libmaus2::huffman::EscapeCanonicalEncoder(dist));
						ECE = UNIQUE_PTR_MOVE(tECE);
					}
					else
					{
						::libmaus2::huffman::CanonicalEncoder::unique_ptr_type tCE(new ::libmaus2::huffman::CanonicalEncoder(dist));
						CE = UNIQUE_PTR_MOVE(tCE);
					}
					
					/* seek to block */
					if ( blockptr < idda.data[fileptr].numentries )
					{
						uint64_t const pos = idda.data[fileptr].getPos(blockptr);
					
						istr->clear();
						istr->seekg ( pos , std::ios::beg );
						assert ( static_cast<int64_t>(istr->tellg()) == static_cast<int64_t>(pos) );
						sbis_type::raw_input_ptr_type ript(new sbis_type::raw_input_type(*istr));
						sbis_ptr_type tSBIS(new sbis_type(ript,64*1024));
						SBIS = UNIQUE_PTR_MOVE(tSBIS);
					}
				}
			}
			
			uint64_t getN() const
			{
				return idda.kvec.size() ? idda.kvec[idda.kvec.size()-1] : 0;
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
						::libmaus2::huffman::FileBlockOffset const FBO = idda.findKBlock(offset);
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
			

			void initKV(uint64_t kvtarget, KvInitResult & result)
			{
				result = KvInitResult();
			
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
						::libmaus2::huffman::FileBlockOffset const FBO = idda.findKVBlock(kvtarget);
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
			
			GapDecoder(
				::libmaus2::huffman::IndexDecoderDataArray const & ridda,
				uint64_t kvtarget,
				KvInitResult & result 
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

			GapDecoder(
				std::vector<std::string> const & rfilenames,
				uint64_t kvtarget,
				KvInitResult & result 
			)
			:
			  Pidda(::libmaus2::huffman::IndexDecoderDataArray::construct(rfilenames)),
			  idda(*Pidda),
			  /* buffer */
			  decodebuf(), pa(0), pc(0), pe(0), 
			  /* file and segment pointers */
			  fileptr(0), blockptr(0)
			{
				initKV(kvtarget, result);
			}

			GapDecoder(
				std::vector<std::string> const & rfilenames, 
				uint64_t offset = 0, 
				uint64_t * psymoffset = 0
			)
			:
			  Pidda(::libmaus2::huffman::IndexDecoderDataArray::construct(rfilenames)),
			  idda(*Pidda),
			  /* buffer */
			  decodebuf(), pa(0), pc(0), pe(0), 
			  /* file and segment pointers */
			  fileptr(0), blockptr(0)
			{
				init(offset,psymoffset);
			}

			GapDecoder(
				::libmaus2::huffman::IndexDecoderDataArray const & ridda,
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

				/* align to byte boundary */
				SBIS->flush();
				/* read block size */
				uint64_t const blocksize = ::libmaus2::bitio::readElias2(*SBIS);

				/* align to byte boundary */
				SBIS->flush();
			
				/* increase size of memory buffer if necessary */
				if ( blocksize > decodebuf.size() )
					decodebuf.resize(blocksize);

				/* set buffer pointers */
				pa = decodebuf.begin();
				pc = pa;
				pe = pa + blocksize;

				/* decode block */
				if ( needescape )
				{
					for ( uint64_t i = 0; i < blocksize; ++i )
						decodebuf[i] = ECE->fastDecode(*SBIS);
					SBIS->flush();
				}
				else
				{
					for ( uint64_t i = 0; i < blocksize; ++i )
						decodebuf[i] = CE->fastDecode(*SBIS);
					SBIS->flush();
				}

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

			// get length of file in symbols
			static uint64_t getLength(std::string const & filename)
			{
				std::ifstream istr(filename.c_str(),std::ios::binary);
				assert ( istr.is_open() );
				::libmaus2::bitio::StreamBitInputStream SBIS(istr);	
				SBIS.readBit(); // need escape
				return ::libmaus2::bitio::readElias2(SBIS);
			}
			
			// get length of vector of files in symbols
			static uint64_t getLength(std::vector<std::string> const & filenames)
			{
				uint64_t s = 0;
				for ( uint64_t i = 0; i < filenames.size(); ++i )
					s += getLength(filenames[i]);
				return s;
			}
		};
	}
}
#endif
