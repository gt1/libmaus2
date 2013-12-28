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
#if ! defined(LIBMAUS_RANK_RUNLENGTHBITVECTOR_HPP)
#define LIBMAUS_RANK_RUNLENGTHBITVECTOR_HPP

#include <libmaus/rank/RunLengthBitVectorBase.hpp>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/gamma/GammaDecoder.hpp>
#include <libmaus/util/GetObject.hpp>
#include <libmaus/util/NumberSerialisation.hpp>

namespace libmaus
{
	namespace rank
	{
		struct RunLengthBitVector
		{
			// size of a single block in encoded bits
			uint64_t const blocksize;
			// total length of bit stream in bits
			uint64_t const n;
			// position of index
			uint64_t const indexpos;
			// rank accumulator bits
			uint64_t const rankaccbits;
			
			libmaus::autoarray::AutoArray<uint64_t> const data;
			libmaus::autoarray::AutoArray<uint64_t> const index;
			
			RunLengthBitVector(std::istream & in)
			: 
			  blocksize(libmaus::util::NumberSerialisation::deserialiseNumber(in)), 
			  n(libmaus::util::NumberSerialisation::deserialiseNumber(in)),
			  indexpos(libmaus::util::NumberSerialisation::deserialiseNumber(in)),
			  rankaccbits(libmaus::rank::RunLengthBitVectorBase::getRankAccBits(n)),
			  data(in),
			  index(in)
			{
				#if 0
				std::cerr << "blocksize=" << blocksize << std::endl;	
				std::cerr << "blocks=" << index.size() << std::endl;	
				std::cerr << "n=" << n << std::endl;	
				#endif
			}
			
			uint64_t getBlockPointer(uint64_t const b) const
			{
				return index[b];
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
				uint64_t const wordoff = (blockptr / (8*sizeof(uint64_t)));
				uint64_t const bitoff = blockptr % (8*sizeof(uint64_t));
				
				::libmaus::util::GetObject<uint64_t const *> GO(data.begin() + wordoff);
				::libmaus::gamma::GammaDecoder < ::libmaus::util::GetObject<uint64_t const *> > GD(GO);
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
				uint64_t const wordoff = (blockptr / (8*sizeof(uint64_t)));
				uint64_t const bitoff = blockptr % (8*sizeof(uint64_t));
				
				::libmaus::util::GetObject<uint64_t const *> GO(data.begin() + wordoff);
				::libmaus::gamma::GammaDecoder < ::libmaus::util::GetObject<uint64_t const *> > GD(GO);
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
				uint64_t const wordoff = (blockptr / (8*sizeof(uint64_t)));
				uint64_t const bitoff = blockptr % (8*sizeof(uint64_t));
				
				::libmaus::util::GetObject<uint64_t const *> GO(data.begin() + wordoff);
				::libmaus::gamma::GammaDecoder < ::libmaus::util::GetObject<uint64_t const *> > GD(GO);
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

			/**
			 * Return the position of the ii'th 0 bit. This function is implemented using a 
			 * binary search on the rank1 function.
			 **/
			uint64_t select1(uint64_t const ii) const
			{
				uint64_t const i = ii+1;

				uint64_t left = 0, right = n;
				
				while ( (right-left) )
				{
					uint64_t const d = right-left;
					uint64_t const d2 = d>>1;
					uint64_t const mid = left + d2;

					// number of ones is too small
					if ( rank1(mid) < i )
						left = mid+1;
					// number of ones is too large
					else if ( rank1(mid) > i )
						right = mid;
					// if this is the leftmost occurence in the interval, return it
					else if ( (!mid) || (rank1(mid-1) != i) )
						return mid;
					// otherwise, go on and search to the left
					else
						right = mid;
				}
				
				return n;		
			}

			/**
			 * return number of 0 bits up to (and including) index i
			 * @param i
			 * @return inverse population count
			 **/
			uint64_t rank0(uint64_t const i) const
			{
				return (i+1) - rank1(i);
			}

			/**
			 * return number of 0 bits up to (and excluding) index i
			 * @param i
			 * @return inverse population count
			 **/
			uint64_t rankm0(uint64_t const i) const
			{
				return i - rankm1(i);
			}

			/**
			 * Return the position of the ii'th 0 bit. This function is implemented using a 
			 * binary search on the rank0 function.
			 **/
			uint64_t select0(uint64_t const ii) const
			{
				uint64_t const i = ii+1;

				uint64_t left = 0, right = n;
				
				while ( (right-left) )
				{
					uint64_t const d = right-left;
					uint64_t const d2 = d>>1;
					uint64_t const mid = left + d2;

					// number of ones is too small
					if ( rank0(mid) < i )
						left = mid+1;
					// number of ones is too large
					else if ( rank0(mid) > i )
						right = mid;
					// if this is the leftmost occurence in the interval, return it
					else if ( (!mid) || (rank0(mid-1) != i) )
						return mid;
					// otherwise, go on and search to the left
					else
						right = mid;
				}
				
				return n;		
			}

			uint64_t inverseSelect1(uint64_t i, unsigned int & rsym) const
			{
				uint64_t const ii = i;
				assert ( i < n );
				
				uint64_t const block = i / blocksize;
				i -= block*blocksize;

				uint64_t const blockptr = getBlockPointer(block);		
				uint64_t const wordoff = (blockptr / (8*sizeof(uint64_t)));
				uint64_t const bitoff = blockptr % (8*sizeof(uint64_t));
				
				::libmaus::util::GetObject<uint64_t const *> GO(data.begin() + wordoff);
				::libmaus::gamma::GammaDecoder < ::libmaus::util::GetObject<uint64_t const *> > GD(GO);
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

				rsym = sym;

				if ( sym )
				{
					return r+i+1;
				}
				else
				{
					return ii+1-r;
				}
			}
		};
	}
}
#endif
