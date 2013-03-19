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
#if ! defined(LIBMAUS_ARITHMETIC_RANGEENCODER_HPP)
#define LIBMAUS_ARITHMETIC_RANGEENCODER_HPP

#include <libmaus/arithmetic/RangeCoder.hpp>

namespace libmaus
{
	namespace arithmetic
	{
		template<typename _output_type>
		struct RangeEncoder : public RangeCoder
		{
			typedef _output_type output_type;

			private:
			bool Flushed;
			output_type & Output;

			void EncodeRange(uint32_t const SymbolLow, uint32_t const SymbolHigh, uint32_t const TotalRange)
			{
				Low += SymbolLow*(Range/=TotalRange);
				Range *= SymbolHigh-SymbolLow;
				
				while (
					(Low ^ (Low+Range))<Top()
					|| 
					(Range<Bottom() && ((Range= -Low & (Bottom()-1)),1))
				)
				{
					Output.put(Low>>56);
					Range<<=8;
					Low<<=8;
				}                                        
			}
			
			public:
			RangeEncoder(output_type & rOutput) : Flushed(false), Output(rOutput) {}
			~RangeEncoder() { if (!Flushed) flush(); }
			

			template<typename model_type>
			void encode(model_type const & model, unsigned int const sym)
			{
				EncodeRange(model.getLow(sym),model.getHigh(sym),model.getTotal());
			}

			template<typename model_type>
			void encodeUpdate(model_type & model, unsigned int const sym)
			{
				encode(model,sym);
				model.update(sym);
			}

			void flush()
			{
				if(!Flushed)
				{
					for( unsigned int i = 0; i < 8; i++ )
					{
						Output.put(Low>>56);
						Low<<=8;
					}
					Flushed=true;
				}                                                                                                                                        
			}
		};
	}
}
#endif
