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
#if !defined(LIBMAUS2_LZ_SIMPLECOMPRESSEDINPUTSTREAM_HPP)
#define LIBMAUS2_LZ_SIMPLECOMPRESSEDINPUTSTREAM_HPP

#include <libmaus2/lz/DecompressorObjectFactory.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/util/utf8.hpp>
#include <libmaus2/util/NumberSerialisation.hpp>
#include <libmaus2/util/CountPutObject.hpp>
#include <map>

namespace libmaus2
{
	namespace lz
	{
		template<typename _stream_type>
		struct SimpleCompressedInputStream
		{
			typedef _stream_type stream_type;
			
			private:
			stream_type & stream;
			libmaus2::lz::DecompressorObject::unique_ptr_type decompressor;
			libmaus2::autoarray::AutoArray<char> C;
			libmaus2::autoarray::AutoArray<char> B;
			char * pa;
			char * pc;
			char * pe;
			
			uint64_t streambytesread;
			bool const blockseek;
			
			uint64_t gcnt;
			
			bool fillBuffer()
			{
				if ( blockseek )
				{
					stream.seekg(streambytesread);
					
					if ( ! stream )
					{
						libmaus2::exception::LibMausException se;
						se.getStream() << "SimpleCompressedInputStream: failed to seek on init" << std::endl;
						se.finish();
						throw se;
					}
				}
				
				if ( stream.peek() == stream_type::traits_type::eof() )
					return false;
			
				libmaus2::util::CountPutObject CPO;
				uint64_t const uncomp = libmaus2::util::UTF8::decodeUTF8(stream);
				::libmaus2::util::UTF8::encodeUTF8(uncomp,CPO);
				uint64_t const comp = ::libmaus2::util::NumberSerialisation::deserialiseNumber(stream);
				::libmaus2::util::NumberSerialisation::serialiseNumber(CPO,comp);
				
				if ( comp > C.size() )
					C = libmaus2::autoarray::AutoArray<char>(comp,false);
				if ( uncomp > B.size() )
					B = libmaus2::autoarray::AutoArray<char>(uncomp,false);
					
				stream.read(C.begin(),comp);
				CPO.write(C.begin(),comp);
				
				if ( ! stream )
				{
					libmaus2::exception::LibMausException se;
					se.getStream() << "SimpleCompressedInputStream: failed to read data" << std::endl;
					se.finish();
					throw se;
				}
				
				streambytesread += CPO.c;

				bool const ok = decompressor->rawuncompress(C.begin(),comp,B.begin(),uncomp);

				if ( ! ok )
				{
					libmaus2::exception::LibMausException se;
					se.getStream() << "SimpleCompressedInputStream: failed to decompress data" << std::endl;
					se.finish();
					throw se;
				}
				
				pa = B.begin();
				pc = pa;
				pe = pa + uncomp;
				
				return true;
			}
			
			public:
			typedef std::pair<uint64_t,uint64_t> u64pair;
			
			SimpleCompressedInputStream(
				stream_type & rstream, 
				libmaus2::lz::DecompressorObjectFactory & decompfactory,
				u64pair const offset = u64pair(0,0),
				bool rblockseek = false
			)
			: stream(rstream), decompressor(decompfactory()), B(), pa(0), pc(0), pe(0), streambytesread(0), blockseek(rblockseek), gcnt(0)
			{
				if ( offset != std::pair<uint64_t,uint64_t>(0,0) )
				{
					stream.seekg(offset.first);
					
					if ( ! stream )
					{
						libmaus2::exception::LibMausException se;
						se.getStream() << "SimpleCompressedInputStream: failed to seek on init" << std::endl;
						se.finish();
						throw se;
					}
					
					streambytesread = offset.first;
					
					uint64_t off = offset.second;
					
					while ( off )
					{
						assert ( pc == pe );
					
						bool const ok = fillBuffer();
						
						if ( ! ok )
						{
							libmaus2::exception::LibMausException se;
							se.getStream() << "SimpleCompressedInputStream: init position is past end of stream" << std::endl;
							se.finish();
							throw se;
						}
						
						uint64_t const sub = std::min(off,static_cast<uint64_t>(pe-pc));
						
						pc += sub;
						off -= sub;
					}
				}
			}
			
			uint64_t read(char * p, uint64_t n)
			{
				uint64_t r = 0;
				
				while ( n )
				{
					if ( pc == pe )
					{
						bool const ok = fillBuffer();
						if ( ! ok )
						{
							gcnt = r;
							return r;
						}
					}
						
					uint64_t const avail = pe-pc;
					uint64_t const ln = std::min(avail,n);
					
					std::copy(pc,pc+ln,p);
					
					pc += ln;
					p += ln;
					n -= ln;
					r += ln;
				}
				
				gcnt = r;
				return r;
			}
			
			int get()
			{
				while ( pc == pe )
				{
					bool ok = fillBuffer();
					if ( ! ok )
					{
						gcnt = 0;
						return -1;
					}
				}
					
				gcnt = 1;
				return static_cast<int>(static_cast<uint8_t>(*(pc++)));
			}
			
			uint64_t gcount() const
			{
				return gcnt;
			}
		};
	}
}
#endif
