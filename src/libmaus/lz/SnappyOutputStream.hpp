/*
    libmaus
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
#if ! defined(LIBMAUS_LZ_SNAPPYOUTPUTSTREAM_HPP)
#define LIBMAUS_LZ_SNAPPYOUTPUTSTREAM_HPP

#include <libmaus/lz/SnappyCompress.hpp>
#include <libmaus/util/NumberSerialisation.hpp>
#include <libmaus/util/utf8.hpp>

namespace libmaus
{
	namespace lz
	{
		template<typename _stream_type>
		struct SnappyOutputStream
		{
			typedef _stream_type stream_type;
		
			::std::ostream & out;
			::libmaus::autoarray::AutoArray<char> B;
			char * const pa;
			char *       pc;
			char * const pe;
			bool const bigbuf;
			
			bool putbigbuf;
			
			SnappyOutputStream(stream_type & rout, uint64_t const bufsize = 64*1024, bool delaybigbuf = false)
			: out(rout), B(bufsize), pa(B.begin()), pc(pa), pe(B.end()), bigbuf(bufsize > ((1ull<<31)-1)),
			  putbigbuf(false)
			{
				if ( ! delaybigbuf )
				{
					out.put(bigbuf);
					putbigbuf = true;				
				}
			}
			~SnappyOutputStream()
			{
				flush();
			}
			
			operator bool()
			{
				return true;
			}
			
			void flush()
			{
				if ( pc != pa )
				{
					if ( ! putbigbuf )
					{
						out.put(bigbuf);
						putbigbuf = true;
					}
					// compress data
					std::string const cdata = SnappyCompress::compress(std::string(pa,pc));
					
					// store size of uncompressed buffer
					if ( bigbuf )
						::libmaus::util::NumberSerialisation::serialiseNumber(out,pc-pa);
					else
						::libmaus::util::UTF8::encodeUTF8(pc-pa,out);

					// store size of compressed buffer
					::libmaus::util::NumberSerialisation::serialiseNumber(out,cdata.size());
					
					// write compressed data
					out.write(cdata.c_str(),cdata.size());
					
					if ( ! out )
					{
						::libmaus::exception::LibMausException se;
						se.getStream() << "Failed to flush SnappyOutputStream." << std::endl;
						se.finish();
						throw se;
					}
				}
				
				pc = pa;
			}
			
			void put(int const c)
			{
				*(pc++) = c;
				
				if ( pc == pe )
					flush();
			}
			
			void write(char const * c, uint64_t n)
			{
				while ( n )
				{
					uint64_t const sp = (pe-pc);
					uint64_t const tocopy = std::min(n,sp);
					std::copy(c,c+tocopy,pc);

					c += tocopy;
					pc += tocopy;
					n -= tocopy;
					
					if ( pc == pe )
						flush();
				}
			}
		};
	}
}
#endif

