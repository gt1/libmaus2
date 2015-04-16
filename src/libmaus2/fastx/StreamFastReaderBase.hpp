/*
    libmaus2
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

#if ! defined(LIBMAUS_FASTX_STREAMFASTREADERBASE_HPP)
#define LIBMAUS_FASTX_STREAMFASTREADERBASE_HPP

#include <libmaus2/exception/LibMausException.hpp>
#include <libmaus2/fastx/CharBuffer.hpp>
#include <libmaus2/network/Socket.hpp>
#include <libmaus2/types/types.hpp>

#include <string>
#include <vector>

namespace libmaus2
{
	namespace fastx
	{
		struct StreamFastReaderBase
		{
			typedef StreamFastReaderBase this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
		
			private:
			std::istream & stream;
			::libmaus2::autoarray::AutoArray<uint8_t> B;
			uint8_t * const pa;
			uint8_t * pc;
			uint8_t * pe;
			uint64_t c;
			
			::libmaus2::fastx::CharBuffer cb;
			
			uint64_t readNumber1()
			{
				int const v = getNextCharacter();
				if ( v < 0 )
				{
					::libmaus2::exception::LibMausException ex;
					ex.getStream() << "Failed to read number in ::libmaus2::aio::SocketFastReaderBase::readNumber1().";
					ex.finish();
					throw ex;
				}
				return v;
			}
			uint64_t readNumber2()
			{
				uint64_t const v0 = readNumber1();
				uint64_t const v1 = readNumber1();
				return (v0<<8)|v1;
			}
			uint64_t readNumber4()
			{
				uint64_t const v0 = readNumber2();
				uint64_t const v1 = readNumber2();
				return (v0<<16)|v1;
			}
			uint64_t readNumber8()
			{
				uint64_t const v0 = readNumber4();
				uint64_t const v1 = readNumber4();
				return (v0<<32)|v1;
			}

			public:
			StreamFastReaderBase(std::istream * rstream, uint64_t const bufsize)
			: 
				stream(*rstream),
				B(bufsize),
				pa(B.get()),
				pc(pa),
				pe(pc),
				c(0)
			{
			}

			uint64_t getC() const
			{
				return c;
			}


			int get()
			{
				return getNextCharacter();
			}

			int getNextCharacter()
			{
				if ( pc == pe )
				{
					stream.read ( reinterpret_cast<char *>(pa), B.size() );
					ssize_t red = stream.gcount();
					
					if (  red > 0 )
					{
						pc = pa;
						pe = pc+red;
					}
					else
					{
						return -1;	
					}
				}
				
				c += 1;
				return *(pc++);
			}
			
			std::pair < char const *, uint64_t > getLineRaw()
			{
				int c;
				cb.reset();
				while ( (c=getNextCharacter()) >= 0 && c != '\n' )
					cb.bufferPush(c);
				
				if ( cb.length == 0 && c == -1 )
					return std::pair<char const *, uint64_t>(reinterpret_cast<char const *>(0),0);
				else
					return std::pair<char const *, uint64_t>(cb.buffer,cb.length);
			}
			
			bool getLine(std::string & s)
			{
				std::pair < char const *, uint64_t > P = getLineRaw();
				
				if ( P.first )
				{
					s = std::string(P.first,P.first+P.second);
					return true;
				}
				else
				{
					return false;
				}
			}			
		};
	}
}
#endif
