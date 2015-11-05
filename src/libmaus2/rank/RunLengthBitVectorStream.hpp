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
#if ! defined(LIBMAUS2_RANK_RUNLENGTHBITVECTORSTREAM_HPP)
#define LIBMAUS2_RANK_RUNLENGTHBITVECTORSTREAM_HPP

#include <libmaus2/aio/SynchronousGenericInput.hpp>
#include <libmaus2/rank/RunLengthBitVectorBase.hpp>
#include <libmaus2/gamma/GammaDecoder.hpp>
#include <libmaus2/util/NumberSerialisation.hpp>
#include <libmaus2/serialize/Serialize.hpp>

#include <istream>

namespace libmaus2
{
	namespace rank
	{
		struct RunLengthBitVectorStream
		{
			std::istream & in;
			uint64_t const blocksize;
			uint64_t const n;
			uint64_t const indexpos;
			uint64_t const blocks;
			uint64_t const pointerarrayoffset;
			uint64_t const baseoffset;
			uint64_t const rankaccbits;

			static uint64_t readAtWordOffset(std::istream & in, uint64_t const pos)
			{
				in.clear();
				in.seekg( static_cast<int64_t>(pos * sizeof(uint64_t)), std::ios::cur);
				uint64_t const v = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				in.seekg(-static_cast<int64_t>(  1 * sizeof(uint64_t)), std::ios::cur);
				in.seekg(-static_cast<int64_t>(pos * sizeof(uint64_t)), std::ios::cur);
				return v;
			}

			static uint64_t readBlockSize(std::istream & in) { return readAtWordOffset(in,0); }
			static uint64_t readN(std::istream & in) { return readAtWordOffset(in,1); }
			static uint64_t readIndexPos(std::istream & in) { return readAtWordOffset(in,2); }

			static uint64_t readBlocks(std::istream & in, uint64_t const indexpos)
			{
				in.clear();
				in.seekg(indexpos,std::ios::cur);
				uint64_t blocks;
				libmaus2::serialize::Serialize<uint64_t>::deserialize(in,&blocks);
				in.clear();
				in.seekg(-static_cast<int64_t>(sizeof(uint64_t)),std::ios::cur);
				in.seekg(-static_cast<int64_t>(indexpos),std::ios::cur);
				return blocks;
			}

			RunLengthBitVectorStream(std::istream & rin, uint64_t const rbaseoffset = 0)
			: in(rin),
			  blocksize(readBlockSize(in)),
			  n(readN(in)),
			  indexpos(readIndexPos(in)),
			  blocks(readBlocks(in,indexpos)),
			  pointerarrayoffset(indexpos + 1*sizeof(uint64_t)), baseoffset(rbaseoffset),
			  rankaccbits(libmaus2::rank::RunLengthBitVectorBase::getRankAccBits())
			{
				#if 0
				std::cerr << "indexpos=" << indexpos << std::endl;
				std::cerr << "blocksize=" << blocksize << std::endl;
				std::cerr << "blocks=" << blocks << std::endl;
				std::cerr << "n=" << n << std::endl;
				#endif
			}

			uint64_t getBlockPointer(uint64_t const b) const
			{
				in.clear();
				in.seekg(baseoffset + pointerarrayoffset + b * sizeof(uint64_t), std::ios::beg);
				uint64_t blockptr;
				libmaus2::serialize::Serialize<uint64_t>::deserialize(in,&blockptr);
				return blockptr + 4*8*sizeof(uint64_t);
			}

			bool operator[](uint64_t const i) const
			{
				return get(i);
			}

			bool get(uint64_t i) const
			{
				assert ( i < n );

				uint64_t const block = i / blocksize;
				i -= block*blocksize;

				uint64_t const blockptr = getBlockPointer(block);

				uint64_t const byteoff = (blockptr / (8*sizeof(uint64_t)))*8;
				uint64_t const bitoff = blockptr % (8*sizeof(uint64_t));

				in.clear();
				in.seekg(baseoffset + byteoff);

				::libmaus2::aio::SynchronousGenericInput<uint64_t> SGI(in,8,std::numeric_limits<uint64_t>::max(),false /* checkmod */);
				::libmaus2::gamma::GammaDecoder < ::libmaus2::aio::SynchronousGenericInput<uint64_t> > GD(SGI);
				if ( bitoff )
					GD.decodeWord(bitoff);

				GD.decodeWord(rankaccbits); // 1 bit accumulator
				bool sym = GD.decodeWord(1);

				uint64_t rl = GD.decode()+1;

				while ( i >= rl )
				{
					i -= rl;
					sym = ! sym;
					rl = GD.decode()+1;
				}

				return sym;
			}

			uint64_t rank1(uint64_t i) const
			{
				assert ( i < n );

				uint64_t const block = i / blocksize;
				i -= block*blocksize;

				uint64_t const blockptr = getBlockPointer(block);
				uint64_t const byteoff = (blockptr / (8*sizeof(uint64_t)))*8;
				uint64_t const bitoff = blockptr % (8*sizeof(uint64_t));

				in.clear();
				in.seekg(baseoffset + byteoff);

				::libmaus2::aio::SynchronousGenericInput<uint64_t> SGI(in,8,std::numeric_limits<uint64_t>::max(),false /* checkmod */);
				::libmaus2::gamma::GammaDecoder < ::libmaus2::aio::SynchronousGenericInput<uint64_t> > GD(SGI);
				if ( bitoff )
					GD.decodeWord(bitoff);

				uint64_t r = GD.decodeWord(rankaccbits); // 1 bit accumulator
				bool sym = GD.decodeWord(1);

				uint64_t rl = GD.decode()+1;

				while ( i >= rl )
				{
					if ( sym )
						r += rl;

					i -= rl;
					sym = ! sym;
					rl = GD.decode()+1;
				}

				if ( sym )
					return r+i+1;
				else
					return r;
			}

			uint64_t rankm1(uint64_t i) const
			{
				assert ( i < n );

				uint64_t const block = i / blocksize;
				i -= block*blocksize;

				uint64_t const blockptr = getBlockPointer(block);
				uint64_t const byteoff = (blockptr / (8*sizeof(uint64_t)))*8;
				uint64_t const bitoff = blockptr % (8*sizeof(uint64_t));

				in.clear();
				in.seekg(baseoffset + byteoff);

				::libmaus2::aio::SynchronousGenericInput<uint64_t> SGI(in,8,std::numeric_limits<uint64_t>::max(),false /* checkmod */);
				::libmaus2::gamma::GammaDecoder < ::libmaus2::aio::SynchronousGenericInput<uint64_t> > GD(SGI);
				if ( bitoff )
					GD.decodeWord(bitoff);

				uint64_t r = GD.decodeWord(rankaccbits); // 1 bit accumulator
				bool sym = GD.decodeWord(1);

				uint64_t rl = GD.decode()+1;

				// std::cerr << "sym=" << sym << " rl=" << rl << std::endl;

				while ( i >= rl )
				{
					if ( sym )
						r += rl;

					i -= rl;
					sym = ! sym;
					rl = GD.decode()+1;

					// std::cerr << "sym=" << sym << " rl=" << rl << std::endl;
				}

				if ( sym )
					return r+i;
				else
					return r;
			}
		};
	}
}
#endif
