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
#if ! defined(LIBMAUS_LZ_SIMPLECOMPRESSEDCONCATINPUTSTREAM_HPP)
#define LIBMAUS_LZ_SIMPLECOMPRESSEDCONCATINPUTSTREAM_HPP

#include <libmaus/lz/SimpleCompressedConcatInputStreamFragment.hpp>
#include <libmaus/lz/DecompressorObjectFactory.hpp>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/util/CountPutObject.hpp>
#include <libmaus/util/utf8.hpp>
#include <libmaus/util/NumberSerialisation.hpp>

namespace libmaus
{
	namespace lz
	{
		template<typename _stream_type>
		struct SimpleCompressedConcatInputStream
		{
			typedef _stream_type stream_type;
			
			private:
			std::vector< ::libmaus::lz::SimpleCompressedConcatInputStreamFragment<stream_type> > fragments;
			::libmaus::lz::DecompressorObject::unique_ptr_type decompressor;
			
			libmaus::autoarray::AutoArray<char> C;
			libmaus::autoarray::AutoArray<char> B;
			char * pa;
			char * pc;
			char * pe;
			
			uint64_t curstream;
			uint64_t curstreampos;

			bool fillBuffer()
			{
				if ( curstream >= fragments.size() )
					return false;
				
				// seek
				fragments[curstream].stream->seekg(curstreampos);
					
				if ( ! (*(fragments[curstream].stream)) )
				{
					libmaus::exception::LibMausException se;
					se.getStream() << "SimpleCompressedConcatInputStream: failed to seek on init" << std::endl;
					se.finish();
					throw se;
				}

				// byte count accumulator			
				libmaus::util::CountPutObject CPO;
				// read number of uncompressed bytes
				uint64_t const uncomp = libmaus::util::UTF8::decodeUTF8(*(fragments[curstream].stream));
				::libmaus::util::UTF8::encodeUTF8(uncomp,CPO);
				// read number of compressed bytes
				uint64_t const comp = ::libmaus::util::NumberSerialisation::deserialiseNumber(*(fragments[curstream].stream));
				::libmaus::util::NumberSerialisation::serialiseNumber(CPO,comp);
				
				// resize buffers if necessary
				if ( comp > C.size() )
					C = libmaus::autoarray::AutoArray<char>(comp,false);
				if ( uncomp > B.size() )
					B = libmaus::autoarray::AutoArray<char>(uncomp,false);
					
				fragments[curstream].stream->read(C.begin(),comp);
				CPO.write(C.begin(),comp);
				
				if ( ! (*(fragments[curstream].stream)) )
				{
					libmaus::exception::LibMausException se;
					se.getStream() << "SimpleCompressedConcatInputStream: failed to read data" << std::endl;
					se.finish();
					throw se;
				}
				
				bool const ok = decompressor->rawuncompress(C.begin(),comp,B.begin());

				if ( ! ok )
				{
					libmaus::exception::LibMausException se;
					se.getStream() << "SimpleCompressedConcatInputStream: failed to decompress data" << std::endl;
					se.finish();
					throw se;
				}
				
				pa = B.begin();
				pc = pa;
				pe = pa + uncomp;
				
				// start of fragment?
				if ( curstreampos == fragments[curstream].low.first )
				{
					// skip bytes
					pc = pa + fragments[curstream].low.second;
				}

				// end of fragment
				if ( curstreampos == fragments[curstream].high.first )
				{
					// set end of buffer
					pe = pa + fragments[curstream].high.second;

					// switch to next fragment (if any)
					if ( ++curstream < fragments.size() )
						curstreampos = fragments[curstream].low.first;
				}
				// not end of fragment
				else
				{
					// shift stream pointer
					curstreampos += CPO.c;
				}
				
				return true;
			}
			
			public:
			SimpleCompressedConcatInputStream(
				std::vector< SimpleCompressedConcatInputStreamFragment<stream_type> > const & rfragments,
				::libmaus::lz::DecompressorObjectFactory & decompfact
			)
			: fragments(SimpleCompressedConcatInputStreamFragment<stream_type>::filter(rfragments)),
			  decompressor(decompfact()),
			  B(), pa(0), pc(0), pe(0), curstream(0), curstreampos(0)
			{
				if ( fragments.size() )
				{
					curstream = 0;
					curstreampos = fragments[curstream].low.first;
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
							return r;
					}
						
					uint64_t const avail = pe-pc;
					uint64_t const ln = std::min(avail,n);
					
					std::copy(pc,pc+ln,p);
					
					pc += ln;
					p += ln;
					n -= ln;
					r += ln;
				}
				
				return r;
			}
			
			int get()
			{
				while ( pc == pe )
				{
					bool const ok = fillBuffer();
					if ( ! ok )
						return -1;
				}
					
				return *(pc++);
			}
		};
	}
}
#endif
