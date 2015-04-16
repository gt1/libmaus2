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

#if ! defined(BITIOOUTPUT_HPP)
#define BITIOOUTPUT_HPP

#include <iostream>
#include <vector>
#include <cassert>

#include <libmaus2/autoarray/AutoArray.hpp>

namespace libmaus2
{
	namespace bitio 
	{

		inline void writeBit(uint8_t * const U, unsigned int const i, bool const b)
		{
			unsigned int const byte = (i>>3);
			unsigned int const bit = i-(byte<<3);
			unsigned int const mask = (0x80>>bit);
			
			if ( b )
				U[byte] |= mask;
			else
				U[byte] &= (~mask);
		}
		
		struct ByteStreamBitWriter
		{
			uint8_t * U;
			uint8_t mask;
			uint8_t cur;
			
			ByteStreamBitWriter(uint8_t * rU) : U(rU), mask(0x80), cur(0) {}
			
			void writeBit(bool const bit)
			{
				if ( bit ) 
				{
					cur |= mask;
				}
				
				mask >>= 1;
				
				if ( ! mask )
				{
					*(U++) = cur;
					mask = 0x80;
					cur = 0;
				}
			}
			void flush()
			{
				while ( mask != 0x80 )
					writeBit(0);
			}
		};

		class BitOutputStream
		{
			public:
			virtual void writeBit(bool) = 0;
			virtual void flush() = 0;
			virtual void flushK(unsigned int k) = 0;
			virtual unsigned long getBitsWritten() const = 0;
			virtual ~BitOutputStream() throw() {}

			// write number
			template<typename N>
			void write(N b, size_t numbits)
			{
				if ( numbits ) {
					assert( (numbits == 8*sizeof(N)) || ((b >> numbits) == 0) );
				
					unsigned long long mask = 0x1ull << (numbits-1);

					for ( ; mask ; mask >>=1 )
						writeBit((b&mask)!=0);
				}
			}
			
			// write number in simple elias code (unary length, binary digits)
			void writeElias(unsigned long const b) {
				unsigned long tmpb = b;
				size_t numbits = 0;

				while ( tmpb ) {
					tmpb >>= 1;
					writeBit(0);
					++numbits;
				}
				
				writeBit(1);
				
				write(b,numbits);
			}
			
			virtual unsigned long getBytesWritten() const {
				return (getBitsWritten()+7)/8;
			}
			
			virtual void writeByte(uint8_t b) {
				write(b,8);
			}
		};

		// class for bitwise output
		template< template<typename,typename> class container >
		class ContainerBitOutputStream : public BitOutputStream, public container<uint8_t, std::allocator<uint8_t> >
		{
			private:
			// current byte
			unsigned char curByte;
			// number of bits in current byte
			unsigned char curByteFill;
			// number of bits written by flush operations
			unsigned long flushBits;

			public:
			// constructor
			ContainerBitOutputStream() : container<uint8_t, std::allocator<uint8_t> >() {
				curByte = 0;
				curByteFill = 0;
				flushBits = 0;
			}
			virtual ~ContainerBitOutputStream() throw() {}
			
			typedef typename container<uint8_t, std::allocator<uint8_t> >::const_iterator const_iterator;

			std::pair<const_iterator,const_iterator> getIteratorPair() const {
				return
					std::make_pair(
						container<uint8_t, std::allocator<uint8_t> >::begin(),
						container<uint8_t, std::allocator<uint8_t> >::end()
						);
			}

			std::ostream & printState(std::ostream & out) {
				out << std::hex << "curByte: " << static_cast<unsigned int>(curByte) 
					<< " curByteFill: " << std::dec << static_cast<unsigned int>(curByteFill)
					<< " bitsWritten " << getBitsWritten()
					<< " flushBits " << flushBits;
				out << " [";
				
				for ( size_t i = 0; i < container<uint8_t, std::allocator<uint8_t> >::size(); ++i )
					out << std::hex << "0x" << static_cast<unsigned int>(container<uint8_t, std::allocator<uint8_t> >::operator[](i)) << std::dec << ";";
				out << "]" << std::endl;
				return out;
			}

			void reset() {
				container<uint8_t, std::allocator<uint8_t> >::operator=(container<uint8_t, std::allocator<uint8_t> >());
				curByte = 0;
				curByteFill = 0;
				flushBits = 0;		
			}
			
			std::ostream & writeContent(std::ostream & out) const {
				unsigned char * a = 0;
				
				try {
					a = new unsigned char[ container<uint8_t,std::allocator<uint8_t> >::size()];
					std::copy(container<uint8_t, std::allocator<uint8_t> >::begin(),container<uint8_t, std::allocator<uint8_t> >::end(),a);
					out.write(reinterpret_cast<char const *>(a),container<uint8_t, std::allocator<uint8_t> >::size());
				} catch(...) {
					delete [] a;
					throw;
				}
				
				return out;
			}

			// write one bit
			void writeBit(bool bit)
			{
				curByte <<= 1;
				curByte |= (bit?1:0);
				curByteFill++;

				// write byte if full
				if ( curByteFill == 8 )
				{
					container<uint8_t, std::allocator<uint8_t> >::push_back(curByte);
					curByte = curByteFill = 0;
				}
			}
			
			unsigned long getNumFlushBits() const {
				return flushBits;
			}

			// flush (align to byte bound) by writing 0 bits
			void flush()
			{
				while ( curByteFill != 0 )
				{
					writeBit(0);
					++flushBits;
				}
			}
			virtual void flushK(unsigned int k)
			{
				flush();
				
				while ( container<uint8_t, std::allocator<uint8_t> >::size() % k )
				{
					writeBit(0);
					flush();
				}
			}

			// return number of bits written
			unsigned long getBitsWritten() const
			{
				return container<uint8_t, std::allocator<uint8_t> >::size() * 8 + curByteFill;
			}
			
			template<typename it>
			void append(it a, it b) {
				std::back_insert_iterator< std::vector<uint8_t> > backins(*this);
				std::copy(a,b,backins);
			}
			::libmaus2::autoarray::AutoArray<uint8_t> toArray(unsigned int k=1)
			{
				flushK(k);
				unsigned int const bytes = container<uint8_t, std::allocator<uint8_t> >::size();
				::libmaus2::autoarray::AutoArray<uint8_t> A( bytes ,false);
				for ( unsigned int i = 0; i < bytes; ++i ) A[i] = (*this)[i];
				return A;
			}
		};
	}
}
#endif
