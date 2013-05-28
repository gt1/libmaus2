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
#if ! defined(ARITHCODER_HPP)
#define ARITHCODER_HPP

#include <libmaus/types/types.hpp>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/cumfreq/DynamicCumulativeFrequencies.hpp>

namespace libmaus
{
	namespace arithmetic
	{
		struct ArithmeticCoder
		{
			public:
			static const uint32_t MaxRange = 0x3FFFFFFFul;

			protected:
			ArithmeticCoder() : High(0xFFFFFFFFul), Low(0) {}

			uint64_t High;
			uint64_t Low;
		};

		struct LogModel
		{
			typedef uint32_t accu_type;
			
			private:
			unsigned int const sigma;
			::libmaus::cumfreq::DynamicCumulativeFrequencies DCF;
			
			public:
			LogModel(unsigned int const rsigma)
			: sigma(rsigma), DCF(sigma) 
			{
				for ( uint64_t i = 0; i < sigma; ++i )
					DCF.inc(i);
			}

			void rescale()
			{
				DCF.half();
				for(unsigned int i=1;i <= sigma;i++) 
					while ( DCF[i] == DCF[i-1] )
						DCF.inc(i);
			}

			void update(unsigned int const symbol)
			{
				DCF.inc(symbol);
			}

			accu_type getLow(unsigned int const symbol) const
			{
				return DCF[symbol];
			}
			accu_type getHigh(unsigned int const symbol) const
			{
				return DCF[symbol+1];
			}
			accu_type getTotal() const
			{
				return DCF[sigma];
			}
			unsigned int getSigma() const
			{
				return sigma;
			}
		};

		struct FixedModel
		{
			private:
			typedef uint32_t accu_type;
			::libmaus::autoarray::AutoArray<accu_type> Freq;

			public:
			FixedModel(::libmaus::autoarray::AutoArray<uint32_t> const & freqs)
			: Freq(freqs.size()+1,false)
			{
				uint64_t s = 0;
				for ( uint64_t i = 0; i < freqs.size(); ++i )
				{
					Freq[i] = s;
					
					if ( freqs[i] )
						s += freqs[i];
					else
						s += 1;
				}
				Freq[freqs.size()] = s;
			}

			accu_type getLow(unsigned int const symbol) const
			{
				return Freq[symbol];
			}
			accu_type getHigh(unsigned int const symbol) const
			{
				return Freq[symbol+1];
			}
			accu_type getTotal() const
			{
				return Freq[Freq.size()-1];
			}
			unsigned int getSigma() const
			{
				return Freq.size()-1;
			}
		};

		struct Model
		{
			private:
			typedef uint32_t accu_type;
			::libmaus::autoarray::AutoArray<accu_type> Freq;
			public:

			Model(unsigned int const sigma)
			: Freq(sigma+1)
			{
				for ( uint64_t i = 0; i < Freq.size(); ++i )
					Freq[i] = i;
			}

			void rescale()
			{
				for(unsigned int i=1;i < Freq.size();i++)
				{
					Freq[i]/=2;
					if(Freq[i]<=Freq[i-1]) Freq[i]=Freq[i-1]+1;
				}
			}

			void update(unsigned int const symbol)
			{
				for(unsigned int j=symbol+1;j<Freq.size();j++) Freq[j]++;
				if(Freq[Freq.size()-1] >= ArithmeticCoder::MaxRange)
					rescale();
			}

			accu_type getLow(unsigned int const symbol) const
			{
				return Freq[symbol];
			}
			accu_type getHigh(unsigned int const symbol) const
			{
				return Freq[symbol+1];
			}
			accu_type getTotal() const
			{
				return Freq[Freq.size()-1];
			}
			unsigned int getSigma() const
			{
				return Freq.size()-1;
			}
		};
	}
}
#endif

