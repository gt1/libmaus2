/*
    libmaus2
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#if !defined(LIBMAUS_NETWORK_HTTPBODY_HPP)
#define LIBMAUS_NETWORK_HTTPBODY_HPP

#include <istream>
#include <cassert>
#include <libmaus2/exception/LibMausException.hpp>
#include <libmaus2/network/SocketInputOutputInterface.hpp>

namespace libmaus2
{
	namespace network
	{
		struct HttpBody : public libmaus2::network::SocketInputInterface
		{
			std::istream & in;
			bool eof;
			bool ischunked;
			int64_t length;
			uint64_t chunkunread;
			
			HttpBody(std::istream & rin, bool const rischunked, int64_t const rlength)
			: in(rin), eof(false), ischunked(rischunked), length(rlength), chunkunread(0)
			{
			
			}
			
			ssize_t read(char * p, size_t n)
			{
				if ( ! n )
					return 0;
				if ( eof )
					return 0;
			
				if ( ischunked )
				{
					// start new chunk if no data left
					if ( ! chunkunread )
					{
						// read length of chunk
						char T[2] = {0,0};
						
						std::ostringstream lenostr;
						while ( (!eof) && (T[0] != '\r' || T[1] != '\n') )
						{
							int const c = in.get();
							
							if ( c == std::istream::traits_type::eof() )
							{
								eof = true;
							}
							else
							{
								lenostr.put(c);
							
								T[0] = T[1];
								T[1] = c;
							}
						}
							
						if ( ! eof )
						{
							// decode length of chunk
							std::string slen = lenostr.str();
							assert ( slen.size() >= 2 );
							assert ( slen[slen.size()-2] == '\r' );
							assert ( slen[slen.size()-1] == '\n' );
							slen = slen.substr(0,slen.size()-2);
							
							uint64_t length = 0;
							for ( uint64_t i = 0; i < slen.size(); ++i )
							{
								length *= 16;
								switch ( slen[i] )
								{
									case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
										length += slen[i]-'0';
										break;
									case 'a': case 'A': case 'b': case 'B': case 'c': case 'C': case 'd': case 'D': case 'e': case 'E': case 'f': case 'F':
										length += ((::std::toupper(slen[i]) - 'A')+10);
										break;
									default:
										break;
								}
							}
							
							chunkunread = length;
						}
					}
					
					uint64_t toread = std::min(
						static_cast<uint64_t>(n),
						static_cast<uint64_t>(chunkunread)
					);
					in.read(p,toread);
					uint64_t const got = in.gcount();
					chunkunread -= got;
					// no more data?
					if ( ! got )
					{
						eof = true;
					}
					// read chunk trailer if chunk is finished
					else if ( ! chunkunread )
					{
						int c0 = in.get();
						if ( c0 == std::istream::traits_type::eof() )
						{
							eof = true;
							return got;
						}
						assert ( c0 == '\r' );
						int c1 = in.get();
						if ( c1 == std::istream::traits_type::eof() )
						{
							eof = true;
							return got;
						}
						assert ( c1 == '\n' );
					}

					return got;
				}
				else
				{
					if ( length >= 0 )
					{
						uint64_t toread = std::min(static_cast<uint64_t>(n),static_cast<uint64_t>(length));
						
						in.readsome(p,toread);
						
						if ( ! in.gcount() )
						{
							in.get();
							in.unget();

							in.readsome(p,toread);
						}
						
						length -= in.gcount();
						if ( in.gcount() == 0 || length == 0 )
							eof = true;
						return in.gcount();
					}
					else
					{
						in.readsome(p,n);
						
						if ( ! in.gcount() )
						{
							in.get();
							in.unget();

							in.readsome(p,n);
						}
																		
						if ( in.gcount() == 0 )
							eof = true;
						return in.gcount();
					}
				}
				return 0;
			}

			ssize_t readPart(char * p, size_t n)
			{
				return read(p,n);
			}
		};
	}
}
#endif
