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
#if ! defined(LIBMAUS_RANK_RUNLENGTHBITVECTORGENERATOR_HPP)
#define LIBMAUS_RANK_RUNLENGTHBITVECTORGENERATOR_HPP

#include <libmaus/aio/SynchronousGenericOutput.hpp>
#include <libmaus/gamma/GammaEncoder.hpp>
#include <libmaus/gamma/GammaDecoder.hpp>
#include <libmaus/rank/RunLengthBitVectorBase.hpp>
#include <libmaus/util/NumberSerialisation.hpp>
#include <cassert>

namespace libmaus
{
	namespace rank
	{
		struct RunLengthBitVectorGenerator
		{
			typedef RunLengthBitVectorGenerator this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
		
			// block size in symbols
			uint64_t const blocksize;
			
			// current bit symbol for putbit method
			bool single_cursym;
			// current run length in putbit
			uint64_t single_runlength;
			
			// number of 0 and 1 bits in current block
			uint64_t blockcnt[2];
			// accumulator for 1 bits
			uint64_t bacc;
			// number of blocks
			uint64_t blocks;
			
			std::ostream & ostr;
			std::iostream & indexstr;

			::libmaus::aio::SynchronousGenericOutput<uint64_t> SGO;
			::libmaus::gamma::GammaEncoder < ::libmaus::aio::SynchronousGenericOutput<uint64_t> > GE;
			
			uint64_t const rankaccbits;
			
			/*
			 * file format:
			 * - blocksize
			 * - length in bits
			 * - index position
			 * - length of data in 64 bit words (AutoArray header)
			 * - data
			 * - number of blocks
			 * - block index
			 */
			 
			static uint64_t getNumPreDataWords()
			{
				return 4;
			}
			
			RunLengthBitVectorGenerator(
				std::ostream & rostr,
				std::iostream & rindexstr,
				uint64_t const rn,
				uint64_t const rblocksize = 64ull*1024ull
			)
			: blocksize(rblocksize), 
			  single_cursym(true), 
			  single_runlength(0), 
			  bacc(0),
			  blocks(0),
			  ostr(rostr),
			  indexstr(rindexstr),
			  SGO(ostr,64*1024),
			  GE(SGO),
			  rankaccbits(libmaus::rank::RunLengthBitVectorBase::getRankAccBits())
			{
				blockcnt[0] = blockcnt[1] = 0;

				// space for block size
				SGO.put(0);
				// space for length of vector in bits
				SGO.put(rn);

				// space for index position
				SGO.put(0);		
				// space for auto array header
				SGO.put(0);
			}
			
			uint64_t size() const
			{
				return blocks ? 
						(
							(blockcnt[0]+blockcnt[1]) ? (blocks-1)*blocksize+blockcnt[0]+blockcnt[1] : (blocks*blocksize)
						) 
						: 0;
			}
			
			void runFlush()
			{
				if ( single_runlength )
				{
					putrun(single_cursym,single_runlength);
					single_runlength = 0;
					single_cursym = ! single_cursym;
				}
			}
			
			uint64_t flush()
			{
				runFlush();
				
				GE.flush();
				SGO.flush();
				uint64_t const indexpos = SGO.getWrittenBytes();
				
				// seek to start of index file
				indexstr.seekg(indexstr.tellp());
				indexstr.seekg(-static_cast<int64_t>(sizeof(uint64_t) * blocks),std::ios::cur);
				assert ( indexstr.tellg() == 0 );
				
				// write number of blocks
				libmaus::serialize::Serialize<uint64_t>::serialize(ostr,blocks);
				// copy index
				libmaus::util::GetFileSize::copy(indexstr,ostr,blocks*sizeof(uint64_t));
				
				// seek to start of file
				ostr.seekp(
					- static_cast<int64_t>(
						// length of index
						1 * sizeof(uint64_t) + sizeof(uint64_t) * blocks +
						// length of data
						indexpos
					),
					std::ios::cur
				);
				// check
				assert ( ostr.tellp() == 0 );
				// write block size
				libmaus::util::NumberSerialisation::serialiseNumber(ostr,blocksize);
				// write size of bit vector in bits
				libmaus::util::NumberSerialisation::serialiseNumber(ostr,size());
				// write position of index
				libmaus::util::NumberSerialisation::serialiseNumber(ostr,indexpos);
				assert ( indexpos % sizeof(uint64_t) == 0 );
				assert ( indexpos >= 4*sizeof(uint64_t) );
				// length of data in words
				libmaus::serialize::Serialize<uint64_t>::serialize(ostr,indexpos/sizeof(uint64_t)-4);
				// flush output stream
				ostr.flush();
				
				// seek back to end of file
				ostr.seekp(-static_cast<int64_t>(4*sizeof(uint64_t)),std::ios::cur);
				assert ( ostr.tellp() == 0 );
				ostr.seekp(
					static_cast<int64_t>(
						// length of index
						1 * sizeof(uint64_t) + sizeof(uint64_t) * blocks +
						// length of data
						indexpos
					),
					std::ios::cur
				);
				ostr.clear();
				
				// return size of stream
				return
					// length of index
					1 * sizeof(uint64_t) + sizeof(uint64_t) * blocks +
					// length of data
					indexpos;
			}
			
			void putrun(bool const sym, uint64_t len)
			{
				// std::cerr << size() << " " << "putrun(" << sym << "," << len << ")" << std::endl;
			
				assert ( len );
				
				// block is not complete
				do
				{
					// std::cerr << "subputrun(" << sym << "," << len << ")" << std::endl;
				
					uint64_t const oldsum = blockcnt[0] + blockcnt[1];
			
					// start of new block?
					if ( !oldsum )
					{
						// bit offset for block
						uint64_t const bitoff = GE.getOffset() - (getNumPreDataWords()*8*sizeof(uint64_t));
						
						// std::cerr << "block " << blocks << " blockptr=" << bitoff << std::endl;
						
						// write bit offset
						// libmaus::util::NumberSerialisation::serialiseNumber(indexstr,bitoff);
						libmaus::serialize::Serialize<uint64_t>::serialize(indexstr,bitoff);
						// increment number of blocks
						blocks += 1;
						// write accumulator bacc
						GE.encodeWord(bacc,rankaccbits);
						// encode first bit				
						GE.encodeWord(sym,1);
					}
					
					assert ( oldsum < blocksize );
					uint64_t const space = blocksize - oldsum;

					// block is not yet complete
					if ( len < space )
					{
						// write run (sym,len)
						GE.encode(len-1);
						
						blockcnt[sym] += len;
						len = 0;
					}
					// block is completed by this run
					else
					{
						// write run (sym,towrite)
						GE.encode(space-1);
						
						blockcnt[sym] += space;
						len -= space;
						
						assert ( blockcnt[0] + blockcnt[1] == blocksize );
						
						// update rank accumulator
						bacc += blockcnt[1];
						// reset in block counters
						blockcnt[0] = blockcnt[1] = 0;
					}
				}
				while ( len ) ;
			}
			
			void putbit(bool const bit)
			{
				// run continued
				if ( bit == single_cursym )
				{
					++single_runlength;
				}
				// end of run
				else 
				{
					if ( single_runlength )
						putrun(single_cursym,single_runlength);
				
					// start of next run
					single_cursym = bit;
					single_runlength = 1;
				}
			}
		};
	}
}
#endif
