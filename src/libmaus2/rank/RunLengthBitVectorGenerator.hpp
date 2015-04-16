/*
    libmaus2
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#if ! defined(LIBMAUS2_RANK_RUNLENGTHBITVECTORGENERATOR_HPP)
#define LIBMAUS2_RANK_RUNLENGTHBITVECTORGENERATOR_HPP

#include <libmaus2/rank/RunLengthBitVectorGeneratorGammaBase.hpp>
#include <libmaus2/rank/RunLengthBitVectorGeneratorBase.hpp>
#include <libmaus2/aio/SynchronousGenericOutput.hpp>
#include <libmaus2/gamma/GammaEncoder.hpp>
#include <libmaus2/gamma/GammaDecoder.hpp>
#include <libmaus2/rank/RunLengthBitVectorBase.hpp>
#include <libmaus2/util/NumberSerialisation.hpp>
#include <cassert>

namespace libmaus2
{
	namespace rank
	{
		struct RunLengthBitVectorGenerator 
			: 
				public RunLengthBitVectorGeneratorGammaBase,
				public RunLengthBitVectorGeneratorBase
		{
			typedef RunLengthBitVectorGenerator this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
		
			// current bit symbol for putbit method
			bool single_cursym;
			// current run length in putbit
			uint64_t single_runlength;
						
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
				uint64_t const rblocksize = 64ull*1024ull,
				bool const putheader = true
			)
			:
			  RunLengthBitVectorGeneratorGammaBase(rostr),
			  RunLengthBitVectorGeneratorBase(0,rblocksize,RunLengthBitVectorGeneratorGammaBase::GE,rindexstr), 
			  single_cursym(true), 
			  single_runlength(0)
			{
				if ( putheader )
				{
					// space for block size
					SGO.put(0);
					// space for length of vector in bits
					SGO.put(rn);

					// space for index position
					SGO.put(0);		
					// space for auto array header
					SGO.put(0);
				}
			}
			
			
			void runFlush(uint64_t const rlpad)
			{
				if ( single_runlength + rlpad )
				{
					putrun(single_cursym,single_runlength + rlpad);
					single_runlength = 0;
					single_cursym = ! single_cursym;
				}
			}
			
			uint64_t flush()
			{
				// bit padding at end for rankm calls
				static uint64_t const rlpad = 1;
			
				runFlush(rlpad);
				
				RunLengthBitVectorGeneratorGammaBase::GE.flush();
				RunLengthBitVectorGeneratorGammaBase::SGO.flush();
				uint64_t const indexpos = SGO.getWrittenBytes();
				
				// seek to start of index file
				indexstr.seekg(indexstr.tellp());
				indexstr.seekg(-static_cast<int64_t>(sizeof(uint64_t) * blocks),std::ios::cur);
				assert ( static_cast<int64_t>(indexstr.tellg()) == static_cast<int64_t>(0) );
				
				// write number of blocks
				libmaus2::serialize::Serialize<uint64_t>::serialize(ostr,blocks);
				
				// index pointers
				for ( uint64_t i = 0; i < blocks; ++i )
				{
					uint64_t v;
					libmaus2::serialize::Serialize<uint64_t>::deserialize(indexstr,&v);
					assert ( v >= (getNumPreDataWords()*8*sizeof(uint64_t)) );
					v -= (getNumPreDataWords()*8*sizeof(uint64_t));
					libmaus2::serialize::Serialize<uint64_t>::serialize(ostr,v);
				}
				
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
				assert ( static_cast<int64_t>(ostr.tellp()) == static_cast<int64_t>(0) );
				// write block size
				libmaus2::util::NumberSerialisation::serialiseNumber(ostr,blocksize);
				// write size of bit vector in bits
				libmaus2::util::NumberSerialisation::serialiseNumber(ostr,size()-rlpad);
				// write position of index
				libmaus2::util::NumberSerialisation::serialiseNumber(ostr,indexpos);
				assert ( indexpos % sizeof(uint64_t) == 0 );
				assert ( indexpos >= 4*sizeof(uint64_t) );
				// length of data in words
				libmaus2::serialize::Serialize<uint64_t>::serialize(ostr,indexpos/sizeof(uint64_t)-4);
				// flush output stream
				ostr.flush();
				
				// seek back to end of file
				ostr.seekp(-static_cast<int64_t>(4*sizeof(uint64_t)),std::ios::cur);
				assert ( static_cast<int64_t>(ostr.tellp()) == static_cast<int64_t>(0) );
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
