/*
    libmaus2
    Copyright (C) 2018 German Tischler-Höhle

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
#if ! defined(LIBMAUS2_UTIL_BASE64_HPP)
#define LIBMAUS2_UTIL_BASE64_HPP

#include <libmaus2/exception/LibMausException.hpp>
#include <cassert>

namespace libmaus2
{
	namespace util
	{
		struct Base64
		{
			static void encode(std::istream & in, std::ostream & out)
			{
				static char const table[64] = {
					'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
					'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z',
					'0','1','2','3','4','5','6','7','8','9','+','/'
				};

				while ( in && in.peek() != std::istream::traits_type::eof() )
				{
					uint32_t v = 0;

					char tout[4];
					int n = 0;

					if ( in && in.peek() != std::istream::traits_type::eof() )
					{
						int const c = in.get();
						v |= (static_cast<uint32_t>(static_cast<unsigned char>(c)) << 16);
						n += 1;
					}
					if ( in && in.peek() != std::istream::traits_type::eof() )
					{
						int const c = in.get();
						v |= (static_cast<uint32_t>(static_cast<unsigned char>(c)) << 8);
						n += 1;
					}
					if ( in && in.peek() != std::istream::traits_type::eof() )
					{
						int const c = in.get();
						v |= (static_cast<uint32_t>(static_cast<unsigned char>(c)) << 0);
						n += 1;
					}

					assert ( n );

					tout[0] = table [ (v >> 18) & 63 ];
					tout[1] = table [ (v >> 12) & 63 ];
					tout[2] = table [ (v >>  6) & 63 ];
					tout[3] = table [ (v >>  0) & 63 ];

					if ( n == 2 )
						tout[3] = '=';
					else if ( n == 1 )
						tout[2] = tout[3] = '=';

					out.write(&tout[0],4);
				}
			}
			static std::string encode(std::string const & in)
			{
				std::istringstream istr(in);
				std::ostringstream ostr;
				encode(istr,ostr);
				return ostr.str();
			}

			static void decode(std::istream & in, std::ostream & out)
			{
				#if 0
				{
					unsigned char c[256];

					for ( int i = 0; i < 256; ++i )
						c[i] = 255;

					for ( int i = 'A'; i <= 'Z'; ++i )
						c[i] = i-'A';
					for ( int i = 'a'; i <= 'z'; ++i )
						c[i] = i-'a' + ('Z'-'A'+1);
					for ( int i = '0'; i <= '9'; ++i )
						c[i] = i-'0' + ('Z'-'A'+1) + ('z'-'a'+1);

					c['+'] = 62;
					c['/'] = 63;
					c['='] = 0;

					for ( uint64_t i = 0; i < 256; ++i )
					{
						std::cerr << (int)c[i] << ",";
						if ( (i+1) % 32 == 0 )
							std::cerr << std::endl;
					}
				}
				#endif

				static unsigned char const table[256] = {
					255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
					255,255,255,255,255,255,255,255,255,255,255,62,255,255,255,63,52,53,54,55,56,57,58,59,60,61,255,255,255,0,255,255,
					255,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,255,255,255,255,255,
					255,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,255,255,255,255,255,
					255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
					255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
					255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
					255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
				};

				while ( in && in.peek() != std::istream::traits_type::eof() )
				{
					char tin[4] = { '=','=','=','=' };

					in.read(&tin[0],4);

					int const r = in.gcount();

					assert ( r != 0 );

					uint32_t v = 0;

					uint32_t o = 3;
					if ( tin[3] == '=' )
						o -= 1;
					if ( tin[2] == '=' )
						o -= 1;

					if ( r >= 1 )
					{
						uint32_t const u = table[static_cast<int>(tin[0])];

						if ( u == 255 )
						{
							libmaus2::exception::LibMausException lme;
							lme.getStream() << "[E] base64decode: invalid symbol " << static_cast<int>(tin[0]) << std::endl;
							lme.finish();
							throw lme;
						}

						v |= (u << 18);
					}
					if ( r >= 2 )
					{
						uint32_t const u = table[static_cast<int>(tin[1])];

						if ( u == 255 )
						{
							libmaus2::exception::LibMausException lme;
							lme.getStream() << "[E] base64decode: invalid symbol " << static_cast<int>(tin[1]) << std::endl;
							lme.finish();
							throw lme;
						}

						v |= (u << 12);
					}
					if ( r >= 3 )
					{
						uint32_t const u = table[static_cast<int>(tin[2])];

						if ( u == 255 )
						{
							libmaus2::exception::LibMausException lme;
							lme.getStream() << "[E] base64decode: invalid symbol " << static_cast<int>(tin[2]) << std::endl;
							lme.finish();
							throw lme;
						}

						v |= (u << 6);
					}
					if ( r >= 4 )
					{
						uint32_t const u = table[static_cast<int>(tin[3])];

						if ( u == 255 )
						{
							libmaus2::exception::LibMausException lme;
							lme.getStream() << "[E] base64decode: invalid symbol " << static_cast<int>(tin[3]) << std::endl;
							lme.finish();
							throw lme;
						}

						v |= (u << 0);
					}

					unsigned char tout[3];

					tout[0] = (v >> 16)&0xFF;
					tout[1] = (v >> 8)&0xFF;
					tout[2] = (v >> 0)&0xFF;

					out.write(reinterpret_cast<char const *>(&tout[0]),o);
				}
			}
			static std::string decode(std::string const & in)
			{
				std::istringstream istr(in);
				std::ostringstream ostr;
				decode(istr,ostr);
				return ostr.str();
			}
		};
	}
}
#endif
