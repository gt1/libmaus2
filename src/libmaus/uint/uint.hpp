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


#if ! defined(UINT_HPP)
#define UINT_HPP

#include <cassert>
#include <ostream>
#include <string>

#include <iostream>
#include <stdexcept>

#include <libmaus/rank/popcnt.hpp>
#include <libmaus/bitio/BitIOOutput.hpp>
#include <libmaus/bitio/putBits.hpp>

namespace libmaus
{
	namespace uint
	{
		template<unsigned int _words>
		struct UInt
		{
			static unsigned int const words = _words;
		
			uint64_t A[words];

			static unsigned int popcnt(uint64_t const u)
			{
				return ::libmaus::rank::PopCnt8<sizeof(unsigned long)>::popcnt8(u);
			}
			
			void keepLowBits(uint64_t pos)
			{
				uint64_t const eword = pos/64;
				uint64_t const ebit = pos-eword*64;
				
				if ( ebit )
				{
					A[eword] &= (1ull << ebit)-1;
				}
				else
				{
					A[eword] = 0;
				}
				
				for ( uint64_t i = eword+1; i < words; ++i )
					A[i] = 0;
			}
			
			void keepLowHalf()
			{
				if ( words % 2 == 0 )
				{
					for ( uint64_t i = words/2; i < words; ++i )
						A[i] = 0;
				}
				else
				{
					for ( uint64_t i = words/2+1; i < words; ++i )
						A[i] = 0;
					A[words/2] &= 0xFFFFFFFFULL;
				}
			}
			
			uint64_t select1(uint64_t rank) const
			{
				uint64_t pos = 0;

				uint64_t crank;		
				uint64_t i = 0;
				while ( rank >= (crank=popcnt(A[i])) )
				{
					rank -= crank;
					pos += 64;
					i++;
				}
				
				uint64_t word = A[i];
				
				if ( rank >= (crank=popcnt(word&0x00000000FFFFFFFFULL)) )
				{
					rank -= crank;
					pos += 32;
					word >>= 32;
				}
				if ( rank >= (crank=popcnt(word&0x000000000000FFFFULL)) )
				{
					rank -= crank;
					pos += 16;
					word >>= 16;
				}
				if ( rank >= (crank=popcnt(word&0x00000000000000FFULL)) )
				{
					rank -= crank;
					pos += 8;
					word >>= 8;
				}
				if ( rank >= (crank=popcnt(word&0x000000000000000FULL)) )
				{
					rank -= crank;
					pos += 4;
					word >>= 4;
				}
				if ( rank >= (crank=popcnt(word&0x0000000000000003ULL)) )  
				{
					rank -= crank;
					pos += 2;
					word >>= 2;
				}
				if ( rank >= (crank=popcnt(word&0x0000000000000001ULL)) )
				{
					rank -= crank;
					pos += 1;
					word >>= 1;
				}
				
				assert ( rank == 0 );
				assert ( (word & 1) != 0 );
				
				return pos;
			}

			uint64_t select0(uint64_t rank) const
			{
				uint64_t pos = 0;

				uint64_t crank;		
				uint64_t i = 0;
				while ( rank >= (crank=popcnt(~A[i])) )
				{
					rank -= crank;
					pos += 64;
					i++;
				}
				
				uint64_t word = ~A[i];
				
				if ( rank >= (crank=popcnt(word&0xFFFFFFFFUL)) )
				{
					rank -= crank;
					pos += 32;
					word >>= 32;
				}
				if ( rank >= (crank=popcnt(word&0xFFFFUL)) )
				{
					rank -= crank;
					pos += 16;
					word >>= 16;
				}
				if ( rank >= (crank=popcnt(word&0xFFUL)) )
				{
					rank -= crank;
					pos += 8;
					word >>= 8;
				}
				if ( rank >= (crank=popcnt(word&0xFUL)) )
				{
					rank -= crank;
					pos += 4;
					word >>= 4;
				}
				if ( rank >= (crank=popcnt(word&0x3UL)) )
				{
					rank -= crank;
					pos += 2;
					word >>= 2;
				}
				if ( rank >= (crank=popcnt(word&0x1UL)) )
				{
					rank -= crank;
					pos += 1;
					word >>= 1;
				}
				
				assert ( rank == 0 );
				assert ( (word & 1) != 0 );
				
				return pos;
			}
			
			uint64_t rank1(uint64_t pos) const
			{
				pos += 1;
				uint64_t const eword = pos / 64;
				uint64_t const ebit = pos - eword*64;
				uint64_t const emask = (1ull<<ebit)-1;
				uint64_t pc = 0;
				
				for ( uint64_t i = 0; i < eword; ++i )
				{
					pc += popcnt(A[i]);
				}
				
				if ( ebit )
					pc += popcnt(A[eword] & emask);
					
				return pc;
			}

			uint64_t rank0(uint64_t pos) const
			{
				pos += 1;
				uint64_t const eword = pos / 64;
				uint64_t const ebit = pos - eword*64;
				uint64_t const emask = (1ull<<ebit)-1;
				uint64_t pc = 0;
				
				for ( uint64_t i = 0; i < eword; ++i )
				{
					pc += popcnt(~A[i]);
				}
				
				if ( ebit )
					pc += popcnt((~A[eword]) & emask);
					
				return pc;
			}
			
			bool getBit(uint64_t pos) const
			{
				uint64_t const eword = pos/64;
				uint64_t const ebit = 1ull<<(pos-eword*64);
				return A[eword]&ebit;
			}

			void setBit(uint64_t pos, bool b)
			{
				uint64_t const eword = pos / 64;
				uint64_t const ebit = pos - eword*64;
				uint64_t const emask = ~(1ull<<ebit);
				A[eword] &= emask;
				A[eword] |= (static_cast<uint64_t>(b) << ebit);
			}
			/**
			 * insert bit between position pos-1 and pos
			 **/
			void insertBit(uint64_t pos, bool b)
			{
				UInt<words> lowmask;
				UInt<words> highmask;

				uint64_t const fullwords = pos/64;
				uint64_t const restlowbits = pos - fullwords*64;
				for ( uint64_t i = 0; i < fullwords; ++i )
					lowmask.A[i] = 0xFFFFFFFFFFFFFFFFull;
				if ( restlowbits )
				{
					lowmask.A[fullwords] = (1ull << restlowbits)-1ull;
					highmask.A[fullwords] = ~((1ull << restlowbits)-1ull);
				}
				else
					highmask.A[fullwords] = 0xFFFFFFFFFFFFFFFFull;
				
				for ( uint64_t i = fullwords+1; i < words; ++i )
					highmask.A[i] = 0xFFFFFFFFFFFFFFFFull;
				
				UInt<words> low = (*this);
				low &= lowmask;
				UInt<words> bit(static_cast<uint64_t>(b));
				bit <<= pos;
				UInt<words> high = (*this);	
				high &= highmask;
				high <<= 1;
				
				for ( uint64_t i = 0; i < words; ++i )
					A[i] = low.A[i] | bit.A[i] | high.A[i];
			}
			/**
			 * delete bit at position pos
			 **/
			void deleteBit(uint64_t pos)
			{
				UInt<words> lowmask;
				UInt<words> highmask;

				uint64_t const fullwords = pos/64;
				uint64_t const restlowbits = pos - fullwords*64;
				for ( uint64_t i = 0; i < fullwords; ++i )
					lowmask.A[i] = 0xFFFFFFFFFFFFFFFFull;
				if ( restlowbits )
					lowmask.A[fullwords] = (1ull << restlowbits)-1ull;

				for ( uint64_t i = 0; i < words; ++i )
					highmask.A[i] = ~(lowmask.A[i]);
				highmask.setBit(pos,false);
				
				UInt<words> low = (*this);
				low &= lowmask;
				
				UInt<words> high = (*this);	
				high &= highmask;
				high >>= 1;
				
				for ( uint64_t i = 0; i < words; ++i )
					A[i] = low.A[i] | high.A[i];		
			}
			
			operator uint64_t() const
			{
				return A[0];
			}

			
			bool operator==(UInt<words> const & u) const
			{
				for ( unsigned int i = 0; i < words; ++i )
					if ( A[i] != u.A[i] )
						return false;
				return true;
			}
			bool operator!=(UInt<words> const & u) const
			{
				for ( unsigned int i = 0; i < words; ++i )
					if ( A[i] != u.A[i] )
						return true;
				return false;
			}
			
			UInt() 
			{ 
				for ( unsigned int i = 0; i < words; ++i ) A[i] = 0; 
			}
			UInt(uint64_t v) 
			{ 
				for ( unsigned int i = 0; i < words; ++i ) A[i] = 0; A[0] = v; 
			}
			template<unsigned int otherwords>
			UInt(UInt<otherwords> const & u)
			{
				for ( unsigned int i = 0; i < ((words<otherwords)?words:otherwords); ++i )
					A[i] = u.A[i];	
				if ( words > otherwords )
					for ( unsigned int i = otherwords; i != words; ++i )
						A[i] = 0;
			}
			template<unsigned int otherwords>
			UInt<words> & operator=(UInt<otherwords> const & u)
			{
				for ( unsigned int i = 0; i < ((words<otherwords)?words:otherwords); ++i )
					A[i] = u.A[i];	
				return *this;
			}
			UInt(std::istream & in) 
			{ 
				deserialize(in);
			}

			template<unsigned int otherwords>
			void operator |= ( UInt<otherwords> const & u )
			{
				for ( unsigned int i = 0; i < ((words<otherwords)?words:otherwords); ++i )
					A[i] |= u.A[i];
			}
			template<unsigned int otherwords>
			void operator &= ( UInt<otherwords> const & u )
			{
				for ( unsigned int i = 0; i < ((words<otherwords)?words:otherwords); ++i )
					A[i] &= u.A[i];
			}

			bool operator[](unsigned int i) const
			{
				unsigned int const fullword = i/64;
				i -= 64*fullword;
				return ((A[fullword] >> i) & 1);
			}
				
			void operator<<=(unsigned int c)
			{
				unsigned int const fullwords = c/64;
				
				// std::cerr << "fullwords: " << fullwords << std::endl;

				if ( fullwords >= words )
				{
					for ( unsigned int i = 0; i < words; ++i ) A[i] = 0;
				}
				else
				{
					for ( unsigned int i = 0; i < words-fullwords; ++i )
					{
						#if 0
						std::cerr << "Assigning " << words - i - 1 - fullwords
							<< " to " << words-i-1 << std::endl;
						#endif
						A [ words-i-1 ] = A [ words - i - 1 - fullwords ];
					}
					for ( unsigned int i = 0; i < fullwords; ++i )
						A [ i ] = 0;
					
					c -= fullwords * 64;
					
					#if 0
					std::cerr << "rest of c is " << c << std::endl;
					#endif
					
					uint64_t const andmask = (1ull << c) - 1;
					unsigned int const shift = 64-c;
					
					for ( unsigned int i = 0; i+1 < words; ++i )
					{
						A[ words - i - 1 ] <<= c;
						A[ words - i - 1 ] |= (A [ words - i - 2 ] >> shift) & andmask;
					}
					
					A [ 0 ] <<= c;			
				}
			}
			void operator>>=(unsigned int c)
			{
				unsigned int const fullwords = c/64;
				
				#if 0
				std::cerr << "fullwords: " << fullwords << std::endl;
				#endif

				if ( fullwords >= words )
				{
					for ( unsigned int i = 0; i < words; ++i ) A[i] = 0;
				}
				else
				{
					for ( unsigned int i = 0; i < words-fullwords; ++i )
					{
						#if 0
						std::cerr << "Assigning " << i
							<< " to " << i+fullwords << std::endl;
						#endif
						A [ i ] = A [ i+fullwords ];
					}
					for ( unsigned int i = 0; i < fullwords; ++i )
						A [ words-i-1 ] = 0;
					
					c -= fullwords * 64;
					
					#if 0
					std::cerr << "rest of c is " << c << std::endl;
					#endif

					uint64_t const andmask = (1ull<<c)-1;
					unsigned int const shift = 64-c;
					
					for ( unsigned int i = 0; i+1 < words; ++i )
					{
						A[ i ] >>= c;
						A[ i ] |= (A [ i+1 ] & andmask) << shift;
					}
					
					A [ words-1 ] >>= c;
				}
			}

			void serialize(std::ostream & out) const
			{
				out.write ( reinterpret_cast<char const *>(&A[0]) , words * sizeof(uint64_t) );
			}
			void deserialize(std::istream & in)
			{
				in.read ( reinterpret_cast<char *>(&A[0]) , words * sizeof(uint64_t) );
			}
			
			void serializeSlow(bitio::BitOutputStream & out, unsigned int numbits) const
			{
				UInt<words> U(1ull); U <<= (numbits-1);
				
				for ( unsigned int i = 0; i < numbits; ++i, U >>= 1 )
					out.writeBit ( (U & (*this)) != UInt<words>(0ull) );
			}
			template<typename writer_type>
			void serialize(writer_type & out, unsigned int numbits) const
			{
				if ( numbits % 64 )
					out.write ( A[ numbits / 64 ], numbits % 64 );
				
				unsigned int const restwords = numbits / 64;
				
				for ( unsigned int i = 0; i < restwords; ++i )
					out.write ( A[restwords-i-1], 64 );
			}

			void serialize(uint64_t * const acode, uint64_t offset, unsigned int numbits) const
			{
				unsigned int mod = numbits % 64;
				
				if ( mod )
				{
					bitio::putBits ( acode, offset, mod, A[numbits/64] & ((1ull<<mod)-1) );
					offset += mod;
				}
				
				unsigned int const restwords = numbits / 64;
				
				for ( unsigned int i = 0; i < restwords; ++i )
				{
					bitio::putBits ( acode, offset, 64, A[restwords-i-1]);
					offset += 64;
				}		
			}

			template<typename writer_type>
			void serializeReverse(writer_type & out, unsigned int numbits) const
			{
				for ( unsigned int i = 0; i < numbits; ++i )
					out.writeBit( getBit(i) );
			}

			static inline unsigned int numBits(uint64_t b)
			{
				unsigned int c = 0;

				if ( b >> 32 ) { c += 32; b >>= 32; }
				if ( b >> 16 ) { c += 16; b >>= 16; }
				if ( b >>  8 ) { c +=  8; b >>=  8; }
				if ( b >>  4 ) { c +=  4; b >>=  4; }
				if ( b >>  2 ) { c +=  2; b >>=  2; }
				if ( b >>  1 ) { c +=  1; b >>=  1; }
				if ( b       ) { c +=  1; b >>=  1; }

				return c;
			}
			unsigned int numBits() const
			{
				for ( unsigned int i = 0; i < words; ++i )
					if ( A[words-i-1] )
						return (words-i-1)*64 + numBits ( A[words-i-1] );
					
				return 0;
			}
			/**
			 * compute parentheses description
			 * @return parentheses description
			 **/
			std::string bitsToString() const
			{
				unsigned int const c = numBits();
				
				if ( ! c )
					return std::string();
				
				UInt<words> highestbit(1);
				highestbit <<= (c-1);
				assert ( ((*this) & highestbit) != UInt<words>() );
				
				std::string s(c,' ');
				
				for ( unsigned int i = 0; i < c; ++ i, highestbit >>= 1 )
				{
					if ( ((*this) & highestbit) != UInt<words>() )
						s[i] = '(';
					else
						s[i] = ')';
				}
				
				return s;
			}
			/**
			 * convert parentheses description to object
			 * @param parentheses description
			 **/
			void stringToBits(std::string const & s)
			{
				for ( unsigned int i = 0; i < words; ++i )
					A[i] = 0;

				for ( unsigned int i = 0; i < s.size(); ++i )
				{
					(*this) <<= 1;
					if ( s[i] == '(' )
						(*this) |= UInt<words>(1);
				}		
			}
		};

		template<unsigned int words>
		UInt<words> operator>> (UInt<words> const & u, unsigned int c)
		{
			UInt<words> t = u;
			t >>= c;
			return t;
		}
		template<unsigned int words>
		UInt<words> operator<< (UInt<words> const & u, unsigned int c)
		{
			UInt<words> t = u;
			t <<= c;
			return t;
		}
		template<unsigned int words>
		UInt<words> operator& (UInt<words> const & u, UInt<words> const & v)
		{
			UInt<words> t = u;
			t &= v;
			return t;
		}
		template<unsigned int words>
		UInt<words> operator| (UInt<words> const & u, UInt<words> const & v)
		{
			UInt<words> t = u;
			t |= v;
			return t;
		}

		template<unsigned int words>
		bool operator< (UInt<words> const & u, UInt<words> const & v)
		{
			for ( unsigned int i = 0; i < words; ++i )
				if ( u.A[words-i-1] != v.A[words-i-1] )
					return u.A[words-i-1] < v.A[words-i-1];
			return false;
		}

		template<unsigned int words>
		std::ostream & operator<< (std::ostream & out, UInt<words> const & u)
		{
			std::string s(64*words,' ');

			unsigned int p = 0;
			for ( unsigned int i = 0; i < words; ++i )
			{
				uint64_t v = u.A[i];
				
				for ( unsigned int j = 0; j < 64; ++j, v>>=1 )
					s[p++] = (v & 1) ? '1' : '0';
			}
			
			out << s;
			
			return out;
		}

		template<unsigned int words>
		std::ostream & print (std::ostream & out, UInt<words> const & u)
		{
			UInt<words> mask (1);
			unsigned int num = u.numBits();
			
			for ( unsigned int i = 0; i < num; ++i, mask <<= 1 )
				if ( (u & mask) != UInt<words>(0) )
					out << '1';
				else
					out << '0';

			return out;
		}

		template<unsigned int words>
		std::ostream & print (std::ostream & out, UInt<words> const & u, unsigned int const num)
		{
			UInt<words> mask (1);
			
			for ( unsigned int i = 0; i < num; ++i, mask <<= 1 )
				if ( (u & mask) != UInt<words>(0) )
					out << '1';
				else
					out << '0';

			return out;
		}
	}
}
#endif
