/*
Copyright (c) 2008, 2009 Andrew I. Schein
Copyright (c) 2010, German Tischler, Solon Pissis
Copyright (c) 2010-2013 German Tischler
Copyright (c) 2011-2013 Genome Research Limited

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <libmaus/types/types.hpp>
#include <libmaus/autoarray/AutoArray.hpp>

namespace libmaus
{
	namespace sorting
	{
		template<typename data_type, typename projector_type>
		struct SerialRadixSort64
		{
			static inline uint64_t u8_0(data_type const & v, projector_type const & p) { return ((p(v))         & 0x7FFULL); }
			static inline uint64_t u8_1(data_type const & v, projector_type const & p) { return (((p(v)) >> 11) & 0x7FFULL); }
			static inline uint64_t u8_2(data_type const & v, projector_type const & p) { return (((p(v)) >> 22) & 0x7FFULL); }
			static inline uint64_t u8_3(data_type const & v, projector_type const & p) { return (((p(v)) >> 33) & 0x7FFULL); }
			static inline uint64_t u8_4(data_type const & v, projector_type const & p) { return (((p(v)) >> 44) & 0x7FFULL); }
			static inline uint64_t u8_5(data_type const & v, projector_type const & p) { return (((p(v)) >> 55) & 0x7FFULL); }

			static inline void radixSort(
				::libmaus::autoarray::AutoArray < data_type > & Areader, 
				uint64_t const n,
				projector_type const & p
			)
			{
				radixSort(Areader.get(),n,p);
			}
			
			static inline void radixSort(
				data_type * const Areader,
				uint64_t const n,
				projector_type const & p
			)
			{
				static unsigned int const HIST_SIZE = 2048;

				data_type * reader = Areader;
				// if (n < HIST_SIZE) { std::sort(reader,reader+n); return; }
				
				/* allocate 6 lists of HIST_SIZE elements */
				::libmaus::autoarray::AutoArray< uint64_t > Ab0(6 * HIST_SIZE, false);
				uint64_t * const b0   = Ab0.get();
				uint64_t * const b1   = b0 + HIST_SIZE;
				uint64_t * const b2   = b1 + HIST_SIZE;
				uint64_t * const b3   = b2 + HIST_SIZE;
				uint64_t * const b4   = b3 + HIST_SIZE;
				uint64_t * const b5   = b4 + HIST_SIZE;

				/* erase lists */
				std::fill(b0, b0+6*HIST_SIZE, 0);

				/* create histograms */
				for (uint64_t i=0; i < n; i++)
				{
					b0[u8_0(reader[i],p)]++;
					b1[u8_1(reader[i],p)]++;
					b2[u8_2(reader[i],p)]++;
					b3[u8_3(reader[i],p)]++;
					b4[u8_4(reader[i],p)]++;
					b5[u8_5(reader[i],p)]++;
				}

				/* compute prefix sums */
				uint64_t sum0=0,sum1=0,sum2=0,sum3=0,sum4=0,sum5=0,tsum=0;
				for (unsigned int j = 0; j < HIST_SIZE; j++)
				{
					tsum  = b0[j] + sum0; b0[j] = sum0 - 1; sum0  = tsum;
					tsum  = b1[j] + sum1; b1[j] = sum1 - 1; sum1  = tsum;
					tsum  = b2[j] + sum2; b2[j] = sum2 - 1; sum2  = tsum;
					tsum  = b3[j] + sum3; b3[j] = sum3 - 1; sum3  = tsum;
					tsum  = b4[j] + sum4; b4[j] = sum4 - 1; sum4  = tsum;
					tsum  = b5[j] + sum5; b5[j] = sum5 - 1; sum5  = tsum;
				}

				::libmaus::autoarray::AutoArray< data_type > Awriter(n,false);
				data_type *writer = Awriter.get();

				/* sort */
				for (uint64_t i=0; i < n; i++) writer[++b0[u8_0(reader[i],p)]] = reader[i]; std::swap(reader,writer);
				for (uint64_t i=0; i < n; i++) writer[++b1[u8_1(reader[i],p)]] = reader[i]; std::swap(reader,writer);
				for (uint64_t i=0; i < n; i++) writer[++b2[u8_2(reader[i],p)]] = reader[i]; std::swap(reader,writer);
				for (uint64_t i=0; i < n; i++) writer[++b3[u8_3(reader[i],p)]] = reader[i]; std::swap(reader,writer);
				for (uint64_t i=0; i < n; i++) writer[++b4[u8_4(reader[i],p)]] = reader[i]; std::swap(reader,writer);
				for (uint64_t i=0; i < n; i++) writer[++b5[u8_5(reader[i],p)]] = reader[i];
			}
		};
	}
}
