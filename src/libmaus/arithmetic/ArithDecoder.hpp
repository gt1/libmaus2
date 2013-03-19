/*	
	author : Sachin Garg (Copyright 2006, see http://www.sachingarg.com/compression/entropy_coding/64bit)
		
	Further modifications by German Tischler

	License:

	Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

	1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
	2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
	3. The name of the author may not be used to endorse or promote products derived from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE AUTHOR ''AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#if ! defined(ARITHDECODER_HPP)
#define ARITHDECODER_HPP

#include <libmaus/arithmetic/ArithCoder.hpp>

// #include <iostream>

namespace libmaus
{
	namespace arithmetic
	{
		template<typename _bitstream_type>
		struct ArithmeticDecoder : public ArithmeticCoder
		{
			typedef _bitstream_type bitstream_type;

			private:
			uint64_t Code;
			bitstream_type &input;

			public:
			ArithmeticDecoder(bitstream_type &BitIStream) : Code(0), input(BitIStream)
			{
				for(int i=0;i<32;i++)
				{
					Code<<=1;
					Code+=input.readBit();;
				}
			}

			template<typename model_type>
			unsigned int decode(model_type const & model)
			{
				uint32_t const Count = GetCurrentCount(model.getTotal());

				unsigned int Symbol;
				for(Symbol=model.getSigma()-1;model.getLow(Symbol)>Count;Symbol--)
				{}

				RemoveRange(model.getLow(Symbol),model.getHigh(Symbol),model.getTotal());
				
				return Symbol;
			}

			template<typename model_type>
			unsigned int decodeUpdate(model_type & model)
			{
				unsigned int const Symbol = decode(model);
				model.update(Symbol);
				return Symbol;
			}

			uint32_t GetCurrentCount(uint32_t TotalRange)
			{
				uint64_t const TempRange=(High-Low)+1;
				return (uint32_t)(((((Code-Low)+1)*(uint64_t)TotalRange)-1)/TempRange);
			}

			void RemoveRange(uint32_t SymbolLow,uint32_t SymbolHigh,uint32_t TotalRange)
			{
				uint64_t const TempRange=(High-Low)+1;
				High=Low+((TempRange*(uint64_t)SymbolHigh)/TotalRange)-1;
				Low	=Low+((TempRange*(uint64_t)SymbolLow )/TotalRange);

				while(true)
				{
					if((High & 0x80000000) == (Low & 0x80000000))
					{
					}
					else
					{
						if((Low	& 0x40000000) && !(High	& 0x40000000))
						{
							Code ^=	0x40000000;
							Low	 &=	0x3FFFFFFF;
							High |=	0x40000000;
						}
						else
							return;
					}
					Low	 = (Low	<< 1) &	0xFFFFFFFF;
					High = ((High<<1) |	1) & 0xFFFFFFFF;

					Code <<=1;
					Code|=input.readBit();
					Code &=0xFFFFFFFF;
				}
			}
			
			uint8_t getNext8()
			{
				uint8_t v = 0;
				for ( uint64_t i = 0; i < 8; ++i )
				{
					v <<= 1;
					v |= (input.readBit()?1:0);
				}
				return v;
			}
			
			void readEndMarker()
			{
				std::vector <uint8_t> V;
				
				while ( ! V.size() )
				{
					// search for marker
					while ( getNext8() != 0xFF )
					{
					}
					// get counter
					uint8_t pre = getNext8();
					if ( pre >= 16 )
						continue;

					while ( (V.size()<3) || (V.back() != 0) )
					{
						uint8_t const mark = getNext8();
						
						if ( mark != 0xFF )
						{
							V.resize(0);
							break;
						}
						
						uint8_t const v = getNext8();
						
						if ( V.size() && V.back() != v+1 )
						{
							V.resize(0);
							break;
						}
						
						V.push_back(v);
					}
				}
			}
			
			void flush(bool const useEndMarker = false)
			{
				input.flush();
				if ( useEndMarker )
					readEndMarker();
			}
		};
	}
}
#endif

