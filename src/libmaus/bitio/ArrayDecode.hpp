/**
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
**/
#if ! defined(LIBMAUS_BITIO_ARRAYDECODE_HPP)
#define LIBMAUS_BITIO_ARRAYDECODE_HPP

#include <libmaus/bitio/BitIOInput.hpp>
#include <libmaus/util/GetObject.hpp>
#include <libmaus/util/PutObject.hpp>

namespace libmaus
{
	namespace bitio
	{
		struct ArrayDecode
		{

			/*
			 * decode array of 1 bit values
			 */
			template<typename stream_type, typename out_stream_type>
			static void decodeArray1(stream_type & in, out_stream_type & out, uint64_t n)
			{
				while ( n >= 8 )
				{
					uint8_t const inp = in.get();
					
					out.put ( (inp >> 7) & 1 );
					out.put ( (inp >> 6) & 1 );
					out.put ( (inp >> 5) & 1 );
					out.put ( (inp >> 4) & 1 );
					out.put ( (inp >> 3) & 1 );
					out.put ( (inp >> 2) & 1 );
					out.put ( (inp >> 1) & 1 );
					out.put ( (inp >> 0) & 1 );
					
					n -= 8;
				}
				
				if ( n )
				{
					uint8_t const inp = in.get();
					
					for ( int shift = 7; n--; shift -= 1 )
						out.put ( (inp >> shift) & 0x1 );
				}
			}

			/*
			 * decode array of 2 bit values
			 */
			template<typename stream_type, typename out_stream_type>
			static void decodeArray2(stream_type & in, out_stream_type & out, uint64_t n)
			{
				while ( n >= 4 )
				{
					uint8_t const inp = in.get();
					
					out.put ( (inp >> 6) & 3 );
					out.put ( (inp >> 4) & 3 ); 
					out.put ( (inp >> 2) & 3 );
					out.put ( (inp >> 0) & 3 );
					
					n -= 4;
				}
				
				if ( n )
				{
					uint8_t const inp = in.get();
					
					switch ( n )
					{
						case 1: out.put ( (inp>>6) & 0x3 );                                                         break;
						case 2: out.put ( (inp>>6) & 0x3 ); out.put ( (inp>>4) & 0x3 );                             break;
						case 3: out.put ( (inp>>6) & 0x3 ); out.put ( (inp>>4) & 0x3 ); out.put ( (inp>>2) & 0x3 ); break;
					}
				}
			}

			/*
			 * decode array of 3 bit values
			 */
			template<typename stream_type, typename out_stream_type>
			static void decodeArray3(stream_type & in, out_stream_type & out, uint64_t n)
			{
				while ( n >= 8 )
				{
					uint32_t inp = 0;
					inp |= static_cast<uint32_t>(in.get()) << 16;
					inp |= static_cast<uint32_t>(in.get()) <<  8;
					inp |= static_cast<uint32_t>(in.get())      ;

					out.put ( (inp >> 21) & 0x7 );
					out.put ( (inp >> 18) & 0x7 );
					out.put ( (inp >> 15) & 0x7 );
					out.put ( (inp >> 12) & 0x7 );
					out.put ( (inp >> 9) & 0x7 );
					out.put ( (inp >> 6) & 0x7 );
					out.put ( (inp >> 3) & 0x7 );
					out.put ( (inp >> 0) & 0x7 );
					n -= 8;
				}
				
				if ( n )
				{
					assert ( n < 8 );

					uint32_t inp  = static_cast<uint32_t>(in.get()) << 16;		
					if ( n >= 3 )
						 inp |= static_cast<uint32_t>(in.get()) << 8;
					if ( n >= 6 )
						 inp |= static_cast<uint32_t>(in.get());
					
					for ( int shift = 21; n--; shift -= 3 )
						out.put( (inp >> shift) & 0x7 );
				}
			}

			/*
			 * decode array of 4 bit values
			 */
			template<typename stream_type, typename out_stream_type>
			static void decodeArray4(stream_type & in, out_stream_type & out, uint64_t n)
			{
				while ( n >= 2 )
				{
					uint8_t const inp = in.get();

					out.put ( (inp >> 4) & 0xF );
					out.put ( (inp >> 0) & 0xF );
					
					n -= 2;
				}
				
				if ( n )
				{
					out.put ( (in.get() >> 4) & 0xF );
				}
			}

			/*
			 * decode array of 5 bit values
			 */
			template<typename stream_type, typename out_stream_type>
			static void decodeArray5(stream_type & in, out_stream_type & out, uint64_t n)
			{
				while ( n >= 8 )
				{
					uint64_t inp  = 0;
					
					inp |= static_cast<uint64_t>(in.get()) << 32;
					inp |= static_cast<uint64_t>(in.get()) << 24;
					inp |= static_cast<uint64_t>(in.get()) << 16;
					inp |= static_cast<uint64_t>(in.get()) <<  8;
					inp |= static_cast<uint64_t>(in.get())      ;

					out.put ( (inp >> 35) & 0x1F );
					out.put ( (inp >> 30) & 0x1F );
					out.put ( (inp >> 25) & 0x1F );
					out.put ( (inp >> 20) & 0x1F );
					out.put ( (inp >> 15) & 0x1F );
					out.put ( (inp >> 10) & 0x1F );
					out.put ( (inp >> 5) & 0x1F );
					out.put ( (inp >> 0) & 0x1F );
					n -= 8;
				}
				
				if ( n )
				{
					assert ( n < 8 );

					uint64_t inp  = static_cast<uint64_t>(in.get()) << 32;		
					if ( n >= 2 )
						 inp |= static_cast<uint64_t>(in.get()) << 24;
					if ( n >= 4 )
						 inp |= static_cast<uint64_t>(in.get()) << 16;
					if ( n >= 5 )
						 inp |= static_cast<uint64_t>(in.get()) <<  8;
					if ( n >= 7 )
						 inp |= static_cast<uint64_t>(in.get()) <<  0;
					
					for ( int shift = 35 ; n--; shift -= 5 )
						out.put ( (inp >> shift) & 0x1F );
				}
			}

			/*
			 * decode array of 6 bit values
			 */
			template<typename stream_type, typename out_stream_type>
			static void decodeArray6(stream_type & in, out_stream_type & out, uint64_t n)
			{
				while ( n >= 4 )
				{
					uint32_t inp  = 0;
					
					inp |= (static_cast<uint32_t>(in.get()) << 16);
					inp |= (static_cast<uint32_t>(in.get()) <<  8);
					inp |= (static_cast<uint32_t>(in.get())      );

					out.put ( (inp >> 18) & 0x3F );
					out.put ( (inp >> 12) & 0x3F );
					out.put ( (inp >>  6) & 0x3F );
					out.put ( (inp >>  0) & 0x3F );
					n -= 4;
				}
				
				if ( n )
				{
					assert ( n < 4 );
					
					#if 0
					[0,6) 5/8 = 0 : 1
					[6,12) 11/8 = 1 : 2
					[12,18) 17/8 = 2 : 3
					[18,24) 23/8 = 2 : 4
					#endif

					uint32_t inp  = static_cast<uint32_t>(in.get()) << 16;
					if ( n >= 2 )
						 inp |= static_cast<uint32_t>(in.get()) << 8;
					if ( n >= 3 )
						 inp |= static_cast<uint32_t>(in.get()) << 0;
					
					for ( int shift = 18; n--; shift -= 6 )
						out.put ( (inp >> shift) & 0x3F );
				}
			}

			/*
			 * decode array of 7 bit values
			 */
			template<typename stream_type, typename out_stream_type>
			static void decodeArray7(stream_type & in, out_stream_type & out, uint64_t n)
			{
				while ( n >= 8 )
				{
					uint64_t inp  = 0;
					
					inp |= (static_cast<uint64_t>(in.get()) << 48);
					inp |= (static_cast<uint64_t>(in.get()) << 40);
					inp |= (static_cast<uint64_t>(in.get()) << 32);
					inp |= (static_cast<uint64_t>(in.get()) << 24);
					inp |= (static_cast<uint64_t>(in.get()) << 16);
					inp |= (static_cast<uint64_t>(in.get()) <<  8);
					inp |= (static_cast<uint64_t>(in.get())      );
					
					out.put ( (inp >> 49) & 0x7F );
					out.put ( (inp >> 42) & 0x7F );
					out.put ( (inp >> 35) & 0x7F );
					out.put ( (inp >> 28) & 0x7F );
					out.put ( (inp >> 21) & 0x7F );
					out.put ( (inp >> 14) & 0x7F );
					out.put ( (inp >> 7) & 0x7F );
					out.put ( (inp >> 0) & 0x7F );
					n -= 8;
				}
				
				if ( n )
				{
					assert ( n < 8 );

					#if 0
					[0,7) 6/8 = 0 : 1
					[7,14) 13/8 = 1 : 2
					[14,21) 20/8 = 2 : 3
					[21,28) 27/8 = 3 : 4
					[28,35) 34/8 = 4 : 5
					[35,42) 41/8 = 5 : 6
					[42,49) 48/8 = 6 : 7
					[49,56) 55/8 = 6 : 8
					#endif

					uint64_t inp  = static_cast<uint64_t>(in.get()) << 48;
					if ( n >= 2 )
						 inp |= static_cast<uint64_t>(in.get()) << 40;
					if ( n >= 3 )
						 inp |= static_cast<uint64_t>(in.get()) << 32;
					if ( n >= 4 )
						 inp |= static_cast<uint64_t>(in.get()) << 24;
					if ( n >= 5 )
						 inp |= static_cast<uint64_t>(in.get()) << 16;
					if ( n >= 6 )
						 inp |= static_cast<uint64_t>(in.get()) <<  8;
					if ( n >= 7 )
						 inp |= static_cast<uint64_t>(in.get()) <<  0;
					
					for ( int shift = 49 ; n--; shift -= 7 )
						out.put ( (inp >> shift) & 0x7F );
				}
			}

			/*
			 * decode array of 8 bit values
			 */
			template<typename stream_type, typename out_stream_type>
			static void decodeArray8(stream_type & in, out_stream_type & out, uint64_t n)
			{
				while ( n-- )
					out.put ( in.get() );
			}


			template<typename stream_type, typename out_stream_type>
			static void decodeArray(stream_type & in, out_stream_type & out, uint64_t n, unsigned int const k)
			{
				switch ( k )
				{
					case 0: for ( uint64_t i = 0; i < n; ++i ) out.put(0); break;
					case 1: decodeArray1(in,out,n); break;
					case 2: decodeArray2(in,out,n); break;
					case 3: decodeArray3(in,out,n); break;
					case 4: decodeArray4(in,out,n); break;
					case 5: decodeArray5(in,out,n); break;
					case 6: decodeArray6(in,out,n); break;
					case 7: decodeArray7(in,out,n); break;
					case 8: decodeArray8(in,out,n); break;
					default: 
					{
						::libmaus::bitio::StreamBitInputStreamTemplate<stream_type> SBIS(in);
						for ( uint64_t i = 0; i < n; ++i )
							out.put ( SBIS.read(k) );
					}
				}
			}

			static void decodeArray(uint8_t const * in, uint8_t * out, uint64_t n, unsigned int const k)
			{
				::libmaus::util::GetObject<uint8_t const *> G(in);
				::libmaus::util::PutObject<uint8_t *> P(out);
				decodeArray(G,P,n,k);
			}
		};
	}
}
#endif
