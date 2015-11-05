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
#if ! defined(LIBMAUS2_RANK_RUNLENGTHBITVECTORGENERATORBASE_HPP)
#define LIBMAUS2_RANK_RUNLENGTHBITVECTORGENERATORBASE_HPP

#include <libmaus2/aio/SynchronousGenericOutput.hpp>
#include <libmaus2/gamma/GammaEncoder.hpp>
#include <libmaus2/rank/RunLengthBitVectorBase.hpp>
#include <libmaus2/util/shared_ptr.hpp>
#include <libmaus2/util/unique_ptr.hpp>

namespace libmaus2
{
	namespace rank
	{
		struct RunLengthBitVectorGeneratorBase
		{
			typedef RunLengthBitVectorGeneratorBase this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			// number of bits per rank acc
			uint64_t const rankaccbits;
			// rank acc (number of 1 bits seen so far)
			uint64_t bacc;
			// number of 0 and 1 bits in current block
			uint64_t blockcnt[2];
			// block size
			uint64_t const blocksize;
			// number of blocks
			uint64_t blocks;
			// gamma encoder
			::libmaus2::gamma::GammaEncoder < ::libmaus2::aio::SynchronousGenericOutput<uint64_t> > & GE;
			// index stream
			std::iostream & indexstr;

			RunLengthBitVectorGeneratorBase(
				uint64_t const rbacc, // start 1 bit accumulator value
				unsigned int const rblocksize, // block size
				::libmaus2::gamma::GammaEncoder < ::libmaus2::aio::SynchronousGenericOutput<uint64_t> > & rGE,
				std::iostream & rindexstr
			)
			:
				rankaccbits(libmaus2::rank::RunLengthBitVectorBase::getRankAccBits()),
				bacc(rbacc),
				blocksize(rblocksize),
				blocks(0),
				GE(rGE),
				indexstr(rindexstr)
			{
				blockcnt[0] = blockcnt[1] = 0;
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
						uint64_t const bitoff = GE.getOffset();
						// write bit offset
						libmaus2::serialize::Serialize<uint64_t>::serialize(indexstr,bitoff);
						// write accumulator bacc
						GE.encodeWord(bacc,rankaccbits);
						// encode first bit
						GE.encodeWord(sym,1);
						// increment number of blocks
						blocks += 1;
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

			uint64_t size() const
			{
				return blocks ?
						(
							(blockcnt[0]+blockcnt[1]) ? (blocks-1)*blocksize+blockcnt[0]+blockcnt[1] : (blocks*blocksize)
						)
						: 0;
			}
		};
	}
}
#endif
