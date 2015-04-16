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


#if ! defined(BITIOINPUT_HPP)
#define BITIOINPUT_HPP

#include <iostream>
#include <cassert>
#include <stdexcept>

#include <libmaus/exception/LibMausException.hpp>
#include <libmaus/util/unique_ptr.hpp>
#include <libmaus/math/lowbits.hpp>

namespace libmaus
{
	namespace bitio {

		// class for bitwise input
		template< typename iterator, bool eof_silent = false >
		class IteratorBitInputStream
		{
			private:		
			// start iterator
			iterator begin;
			// current iterator
			iterator in;
			// end iterator
			iterator end;
			// current byte
			uint8_t curByte;
			// number of bits processed in current byte (mask)
			uint8_t curByteMask;
			// total number of bits read
			uint64_t bitsRead;

			public:
			// constructor by iterators
			IteratorBitInputStream(iterator rin, iterator rend)
			: begin(rin), in(rin), end(rend) {
				bitsRead = curByte = curByteMask = 0;
			}

			std::ostream & printState(std::ostream & out) const {
				out << "Byte offset " << (in-begin) << " CurBytemask 0x" <<
					std::hex << static_cast<unsigned int>(curByteMask) << " CurByte 0x" << static_cast<unsigned int>(curByte) <<
					std::dec << " bitsRead " << bitsRead << std::endl;
				return out;
			}

			inline void skipBytesNoFlush(uint64_t len) throw() {
				assert(static_cast<long>(len) <= (end-in));
				bitsRead += 8*static_cast<uint64_t>(len);
				in += len;		
			}

			inline void skipBytes(uint64_t len) throw() {
				flush();
				skipBytesNoFlush(len);
			}
			
			inline void seekToByte(uint64_t offset) {
				flush();
				
				if ( static_cast<long>(offset) > (end-begin) ) {
					in = end;
					bitsRead = 8*static_cast<uint64_t>(end-begin);
					throw std::runtime_error("EOF in seekToByte()");
				} else {
					bitsRead = 8*static_cast<uint64_t>(offset);
					in = begin+offset;
				}
			}

			// get number of bits read
			inline uint64_t getBitsRead() const
			{
				return bitsRead;
			}

			// read one bit
			inline bool readBit()
			{
				if ( ! curByteMask )
				{
					if ( in == end )
					{
						if ( eof_silent )
							return 0;
						else
						{
							::libmaus::exception::LibMausException se;
							se.getStream() <<  "EOF in readBit()" << std::endl;
							se.finish();
							throw se;
						}
					}
				
					curByte = *(in++);
					curByteMask = 0x80;
				}

				++bitsRead;

				bool bit = (curByte & curByteMask)!=0;
				curByteMask >>= 1;
				return bit;
			}

			// read a string
			inline std::string getString(uint64_t len) {
				flush();
				if ( static_cast<long>(len) > (end-in) )
					throw std::runtime_error("EOF in getString()");
				std::string result(in,in+len);
				in += len;
				bitsRead += 8*static_cast<uint64_t>(len);
				return result;
			}

			// flush (align to byte bound)
			inline void flush() throw() {
				bitsRead += 7;
				bitsRead >>= 3;
				bitsRead <<= 3;
				curByteMask = 0;
			}
			
			// roll back by number of bits
			inline void rollback(uint64_t numbits) throw() {
				assert(numbits <= bitsRead);
				
				bitsRead -= static_cast<uint64_t>(numbits);
				in = begin + (bitsRead >> 3);
				unsigned int modul = bitsRead & 0x7;

				if ( modul ) {
					curByteMask = static_cast<uint8_t>(0x80 >> (modul));
					curByte = *(in++);
				} else {
					curByteMask = 0x0;
				}
			}
			
			inline uint8_t readNextRealByte() throw() {
				bitsRead += 8;
				return *(in++);
			}
			
			inline uint8_t getNextRealByte() throw() {
				return *(in++);
			}

			inline uint8_t getNextByte() throw() {
				if ( in == end )
					return 0;
				else
					return *(in++);
			}

			// read numbits unsigned number
			inline uint64_t read(uint64_t numbits) throw()
			{
				unsigned int bitsleft = (bitsRead & 0x7) ? (8-(bitsRead & 0x7)) : 0;
				bitsRead += static_cast<uint64_t>(numbits);

				if ( numbits <= bitsleft ) {
					unsigned int shift = (bitsleft - numbits);
					uint8_t shifted = static_cast<uint8_t>(curByte >> shift);
					curByteMask >>= numbits;
					return ((1ull << numbits)-1) & shifted;
				}

				numbits -= bitsleft;
				uint64_t result = (((1ull<<bitsleft)-1) & static_cast<uint64_t>(curByte)) << (numbits);

				while ( numbits >= 8 ) {
					numbits -= 8;
					result |= (static_cast<uint64_t>(getNextByte()) << numbits);
				}

				if ( numbits ) {
					curByte = getNextByte();
					curByteMask = static_cast<uint8_t>(0x80) >> numbits;
					result |= (static_cast<uint64_t>(curByte) >> (8-numbits));
				} else {
					curByteMask = 0;
				}

				return result;
			}

			inline uint64_t readElias() {
				uint64_t numbits = 0;
				
				while ( readBit() == 0 )
					numbits++;
					
				return read(numbits);
			}
		};

		// class for bitwise input
		template<typename _stream_type>
		class StreamBitInputStreamTemplate
		{
			public:
			typedef _stream_type stream_type;
			typedef StreamBitInputStreamTemplate<stream_type> this_type;
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
		
			private:	
			// input stream
			stream_type & in;
			// current byte
			uint8_t curByte;
			// number of bits processed in current byte (mask)
			uint8_t curByteMask;
			// total number of bits read
			uint64_t bitsRead;

			public:
			// constructor by iterators
			StreamBitInputStreamTemplate(stream_type & rin)
			: in(rin) {
				bitsRead = curByte = curByteMask = 0;
			}

			// get number of bits read
			inline uint64_t getBitsRead() const
			{
				return bitsRead;
			}

			// read one bit
			inline bool readBit()
			{
				if ( ! curByteMask )
				{
					int const ncurByte = in.get();

					if ( ncurByte < 0 )
					{
						::libmaus::exception::LibMausException se;
						se.getStream() << "EOF in StreamBitInputStream::readBit()." << std::endl;
						se.finish();
						throw se;					
					}

					curByte = ncurByte;
					curByteMask = 0x80;
				}

				++bitsRead;

				bool bit = (curByte & curByteMask)!=0;
				curByteMask >>= 1;
				return bit;
			}

			// flush (align to byte bound)
			inline void flush()
			{
				if ( bitsRead & 7 )
				{
					bitsRead += 7;
					bitsRead >>= 3;
					bitsRead <<= 3;
					curByteMask = 0;
				}
			}
			
			inline uint8_t getNextByte()
			{
				int c = in.get();
				
				if ( c < 0 )
					return 0;
				else
					return c;
			}

			// read numbits unsigned number
			inline uint64_t read(unsigned int numbits)
			{
				unsigned int const bitsleft = (bitsRead & 0x7) ? (8-(bitsRead & 0x7)) : 0;
				bitsRead += static_cast<uint64_t>(numbits);

				if ( numbits <= bitsleft ) {
					unsigned int const shift = (bitsleft - numbits);
					uint8_t const shifted = static_cast<uint8_t>(curByte >> shift);
					curByteMask >>= numbits;
					return ::libmaus::math::lowbits(numbits) /* ((1 << numbits)-1) */ & shifted;
				}

				numbits -= bitsleft;
				uint64_t result = (::libmaus::math::lowbits(bitsleft) /* ((1<<bitsleft)-1) */ & static_cast<uint64_t>(curByte)) << (numbits);

				while ( numbits >= 8 ) 
				{
					numbits -= 8;
					result |= (static_cast<uint64_t>(getNextByte()) << numbits);
				}

				if ( numbits ) 
				{
					curByte = getNextByte();
					curByteMask = static_cast<uint8_t>(0x80) >> numbits;
					result |= (static_cast<uint64_t>(curByte) >> (8-numbits));
				} 
				else 
				{
					curByteMask = 0;
				}

				return result;
			}

			inline uint64_t readElias() {
				unsigned int numbits = 0;
				
				while ( readBit() == 0 )
					numbits++;
					
				return read(numbits);
			}
		};

		typedef StreamBitInputStreamTemplate<std::istream> StreamBitInputStream;
		
		template<typename N, unsigned int k>
		struct FullMask
		{
			static N const v = (FullMask<N,k-1>::v << 1) | static_cast<N>(1);
		};
		
		template<typename N>
		struct FullMask<N,1>
		{
			static N const v = 1;
		};

		template<typename N>
		struct FullMask<N,0>
		{
			static N const v = 0;
		};
		
		template<typename stream_type, typename entity_type>
		struct StreamGetAdapter
		{
			static bool getNext(stream_type & stream, entity_type & v)
			{
				int const rv = stream.get();
				
				if ( rv < 0 )
					return false;
				else
				{
					v = rv;
					return true;
				}
			}
		};

		// class for bitwise input
		template<typename _stream_type, typename _entity_type, typename _adapter_type = StreamGetAdapter<_stream_type,_entity_type> >
		class MarkerStreamBitInputStreamTemplate
		{
			public:
			typedef _stream_type stream_type;
			typedef _entity_type entity_type;
			typedef MarkerStreamBitInputStreamTemplate<stream_type,entity_type> this_type;
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			static entity_type const top_bit = static_cast<entity_type>(1) << (8*sizeof(entity_type)-1);
			static entity_type const sub_top_bit = (top_bit >> 1);
			static entity_type const full_mask = FullMask<entity_type,8*sizeof(entity_type)>::v;
			static entity_type const sub_full_mask = (full_mask - 1);
		
			private:	
			// input stream
			stream_type & in;
			// current byte
			entity_type curByte;
			// number of bits processed in current byte (mask)
			entity_type curByteMask;
			// total number of bits read
			uint64_t bitsRead;

			public:
			// constructor by iterators
			MarkerStreamBitInputStreamTemplate(stream_type & rin)
			: in(rin), curByte(0), curByteMask(0), bitsRead(0)
			{
			}

			// get number of bits read
			inline uint64_t getBitsRead() const
			{
				return bitsRead;
			}
			
			void fillByteChecked()
			{
				int const ncurByte = in.get();

				if ( ncurByte < 0 )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "EOF in MarkerStreamBitInputStream::fillByteChecked()." << std::endl;
					se.finish();
					throw se;					
				}
				
				if ( ncurByte == sub_full_mask )
				{
					curByteMask = sub_top_bit;
					curByte = ncurByte >> 1;
				}
				else
				{
					curByteMask = top_bit;
					curByte = ncurByte;
				}
			}

			void fillByteUnchecked()
			{
				int const ncurByte = in.get();

				if ( ncurByte == sub_full_mask )
				{
					curByteMask = sub_top_bit;
					curByte = ncurByte >> 1;
				}
				else
				{
					curByteMask = top_bit;
					curByte = ncurByte;
				}
			}

			// read one bit
			inline bool readBit()
			{
				if ( ! curByteMask )
					fillByteChecked();

				++bitsRead;

				bool bit = (curByte & curByteMask)!=0;
				curByteMask >>= 1;
				return bit;
			}

			// flush (align to byte bound)
			inline void flush()
			{
				while ( curByteMask )
					readBit();
			}

			// read numbits unsigned number
			inline uint64_t read(unsigned int numbits)
			{
				bitsRead += static_cast<uint64_t>(numbits);
				uint64_t result = 0;
			
				while ( numbits )
				{
					// check how many bits are left in the buffer
					unsigned int const bitsleft = curByteMask ? (__builtin_ctz(curByteMask)+1) : 0;
					
					// we have sufficient bits in the current word to finish
					if ( numbits <= bitsleft ) 
					{
						unsigned int const shift = (bitsleft - numbits);
						entity_type const shifted = static_cast<entity_type>(curByte >> shift);
						
						result <<= numbits;
						result |= ::libmaus::math::lowbits(numbits) & shifted;
						// all requested bits read
						curByteMask >>= numbits;
						numbits = 0;
					}
					// current word does not contain sufficient bits, copy all bits to output
					else
					{
						result <<= bitsleft;
						result |= (::libmaus::math::lowbits(bitsleft) & curByte);
						fillByteChecked();
						numbits -= bitsleft;
					}
				}
				
				return result;
			}

			inline uint64_t readElias() {
				unsigned int numbits = 0;
				
				while ( readBit() == 0 )
					numbits++;
					
				return read(numbits);
			}
		};

		typedef MarkerStreamBitInputStreamTemplate<std::istream,uint8_t> MarkerStreamBitInputStream;
	}
}
#endif
