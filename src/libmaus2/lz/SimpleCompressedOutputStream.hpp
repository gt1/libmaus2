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
#if !defined(LIBMAUS2_LZ_SIMPLECOMPRESSEDOUTPUTSTREAM_HPP)
#define LIBMAUS2_LZ_SIMPLECOMPRESSEDOUTPUTSTREAM_HPP

#include <libmaus2/lz/CompressorObjectFactory.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/util/CountPutObject.hpp>
#include <libmaus2/util/utf8.hpp>
#include <libmaus2/util/NumberSerialisation.hpp>

namespace libmaus2
{
	namespace lz
	{
		template<typename _stream_type>
		struct SimpleCompressedOutputStream
		{
			typedef _stream_type stream_type;
			typedef SimpleCompressedOutputStream<stream_type> this_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
		
			stream_type & out;
			CompressorObject::unique_ptr_type compressor;
			::libmaus2::autoarray::AutoArray<char> B;
			char * const pa;
			char *       pc;
			char * const pe;

			::libmaus2::autoarray::AutoArray<char> O;

			uint64_t compressedwritten;
			
			SimpleCompressedOutputStream(
				stream_type & rout,
				CompressorObjectFactory & cfactory,
				uint64_t const bufsize = 64*1024
			)
			: out(rout), compressor(cfactory()), B(bufsize), pa(B.begin()), pc(pa), pe(B.end()), compressedwritten(0)
			{
			}
			~SimpleCompressedOutputStream()
			{
				flush();
				out.flush();
			}
			
			operator bool()
			{
				return out;
			}
			
			void flush()
			{
				if ( pc != pa )
				{
					// byte counter
					libmaus2::util::CountPutObject CPO;
					
					// compress data
					// std::string const cdata = compressor->compress(std::string(pa,pc));
					uint64_t const cdatasize = compressor->compress(pa,(pc-pa),O);

					// number of uncompressed bytes					
					::libmaus2::util::UTF8::encodeUTF8(pc-pa,out);
					::libmaus2::util::UTF8::encodeUTF8(pc-pa,CPO);
					
					//  number of compressed bytes
					::libmaus2::util::NumberSerialisation::serialiseNumber(out,cdatasize);
					::libmaus2::util::NumberSerialisation::serialiseNumber(CPO,cdatasize);

					// write compressed data
					out.write(O.begin(),cdatasize);
					CPO.write(O.begin(),cdatasize);
					
					if ( ! out )
					{
						::libmaus2::exception::LibMausException se;
						se.getStream() << "Failed to flush SimpleCompressedOutputStream." << std::endl;
						se.finish();
						throw se;
					}
					
					compressedwritten += CPO.c;

					pc = pa;
				}			
			}
			
			void put(int const c)
			{
				if ( pc == pe )
				{
					flush();
					assert ( pc != pe );
				}
				
				*(pc++) = c;
			}
			
			void write(char const * c, uint64_t n)
			{
				while ( n )
				{
					if ( pc == pe )
					{
						flush();
						assert ( pc != pe );
					}

					uint64_t const sp = (pe-pc);
					uint64_t const tocopy = std::min(n,sp);
					std::copy(c,c+tocopy,pc);

					c += tocopy;
					pc += tocopy;
					n -= tocopy;
				}
			}
			
			std::pair<uint64_t,uint64_t> getOffset() const
			{
				return std::pair<uint64_t,uint64_t>(compressedwritten,pc-pa);
			}
		};
	}
}
#endif
