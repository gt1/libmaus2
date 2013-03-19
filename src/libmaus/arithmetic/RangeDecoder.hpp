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
#if ! defined(LIBMAUS_ARITHMETIC_RANGEDECODER_HPP)
#define LIBMAUS_ARITHMETIC_RANGEDECODER_HPP

#include <libmaus/arithmetic/RangeCoder.hpp>

namespace libmaus
{
	namespace arithmetic
	{
		template<typename _input_type>
		struct RangeDecoder : public RangeCoder
		{
			typedef _input_type input_type;

			private:
			input_type & Input;
			uint64_t Code;

			uint32_t GetCurrentCount(uint32_t TotalRange)
			{
				return (Code-Low)/(Range/=TotalRange);
			}
			void RemoveRange(uint32_t SymbolLow, uint32_t SymbolHigh, uint32_t /* TotalRange */)
			{
				Low += SymbolLow*Range;
				Range *= SymbolHigh-SymbolLow;

				while (
					(Low ^ (Low+Range))<Top() 
					|| 
					(Range<Bottom() && ((Range= -Low & (Bottom()-1)),1))
				)
				{
					Code= (Code<<8) | Input.get(), Range<<=8, Low<<=8;
				}
			}

			public:
			RangeDecoder(input_type &IStream) : Input(IStream), Code(0)
			{
				for(unsigned int i=0;i<8u;i++)
				Code = (Code << 8) | Input.get();
			}

			template<typename model_type>
			unsigned int decode(model_type const & model)
			{
				uint32_t const Count = GetCurrentCount(model.getTotal());

				unsigned int Symbol;
				for(Symbol=model.getSigma()-1;model.getLow(Symbol)>Count;Symbol--)
				{
				}

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
		};
	}
}
#endif
