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
#if ! defined(LIBMAUS_LZ_SNAPPYINPUTSTREAM_HPP)
#define LIBMAUS_LZ_SNAPPYINPUTSTREAM_HPP

#include <libmaus/lz/SnappyCompress.hpp>
#include <libmaus/util/NumberSerialisation.hpp>
#include <libmaus/util/utf8.hpp>

namespace libmaus
{
	namespace lz
	{
		struct SnappyInputStream
		{
			typedef SnappyInputStream this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
		
			std::istream & in;
			uint64_t readpos;
			bool const setpos;
			uint64_t const endpos;

			::libmaus::autoarray::AutoArray<char> B;
			char const * pa;
			char const * pc;
			char const * pe;
			uint64_t gcnt;
			
			bool eofthrow;
			
			void setEofThrow(bool const v)
			{
				eofthrow = v;
			}
			
			uint8_t checkedGet()
			{
				if ( setpos )
				{
					in.seekg(readpos,std::ios::beg);
					in.clear();
				}
			
				int const c = in.get();
				
				if ( c < 0 )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Unexpected EOF in SnappyInputStream::checkedGet()" << std::endl;
					se.finish();
					throw se;	
				}
				
				readpos += 1;
				
				return c;
			}
			
			SnappyInputStream(
				std::istream & rin, 
				uint64_t rreadpos = 0, 
				bool const rsetpos = false,
				uint64_t rendpos = std::numeric_limits<uint64_t>::max()
			)
			: in(rin), readpos(rreadpos), setpos(rsetpos), endpos(rendpos),
			  B(), pa(0), pc(0), pe(0), gcnt(0), eofthrow(false)
			{
			}
			
			int peek()
			{
				// std::cerr << "peek." << std::endl;
			
				if ( pc == pe )
				{
					// std::cerr << "buffer empty." << std::endl;
					
					fillBuffer();
					
					if ( pc == pe )
					{
						// std::cerr << "Buffer still empty." << std::endl;
						gcnt = 0;
						return -1;
					}
					
					// std::cerr << "Buffer ok." << std::endl;
				}
				
				assert ( pc != pe );
				
				gcnt = 1;
				return *reinterpret_cast<uint8_t const *>(pc);	
			}
			
			int get()
			{
				if ( pc == pe )
				{
					fillBuffer();
					
					if ( pc == pe )
					{
						gcnt = 0;
						if ( eofthrow )
						{
							libmaus::exception::LibMausException se;
							se.getStream() << "EOF in SnappyInputStream::get()" << std::endl;
							se.finish();
							throw se;
						}
						else
						{
							return -1;
						}
					}
				}
				
				assert ( pc != pe );
				
				gcnt = 1;
				return *(reinterpret_cast<uint8_t const *>(pc++));	
			}
			
			uint64_t read(char * B, uint64_t n)
			{
				uint64_t r = 0;
			
				while ( n )
				{
					if ( pc == pe )
					{
						fillBuffer();
						
						if ( pc == pe )
							break;
					}
					
					uint64_t const av = pe-pc;
					uint64_t const tocopy = std::min(av,n);
					
					std::copy(pc,pc+tocopy,B);
					
					pc += tocopy;
					n -= tocopy;
					B += tocopy;
					r += tocopy;
				}
				
				gcnt = r;
				
				return r;
			}

			uint64_t ignore(uint64_t n)
			{
				uint64_t r = 0;
			
				while ( n )
				{
					if ( pc == pe )
					{
						fillBuffer();
						
						if ( pc == pe )
							break;
					}
					
					uint64_t const av = pe-pc;
					uint64_t const tocopy = std::min(av,n);
					
					pc += tocopy;
					n -= tocopy;
					r += tocopy;
				}
				
				gcnt = r;
				
				return r;
			}
			
			uint64_t gcount() const
			{
				return gcnt;
			}
			
			void fillBuffer()
			{
				assert ( pc == pe );
				
				if ( setpos )
				{
					// std::cerr << "Seeking to " << readpos << std::endl;
					in.seekg(readpos);
					in.clear();
				}

				if ( in.peek() >= 0 && readpos < endpos )
				{
					#if 0
					std::cerr << "Filling block, readpos " << readpos 
						<< " stream at pos " << in.tellg() 
						<< " endpos " << endpos
						<< std::endl;
					#endif
				
					uint64_t blocksize = sizeof(uint64_t) + sizeof(uint64_t);
					
					// size of uncompressed buffer
					uint64_t const n        = ::libmaus::util::NumberSerialisation::deserialiseNumber(in);
					// size of compressed data
					uint64_t const datasize = ::libmaus::util::NumberSerialisation::deserialiseNumber(in);
					// add to block size
					blocksize += datasize;
						
					if ( n > B.size() )
					{
						B = ::libmaus::autoarray::AutoArray<char>(0,false);
						B = ::libmaus::autoarray::AutoArray<char>(n,false);
					}
					
					pa = B.begin();
					pc = pa;
					pe = pa + n;

					::libmaus::aio::IStreamWrapper wrapper(in);
					::libmaus::lz::IstreamSource< ::libmaus::aio::IStreamWrapper> insource(wrapper,datasize);

					try
					{
						SnappyCompress::uncompress(insource,B.begin(),n);
					}
					catch(std::exception const & ex)
					{
						libmaus::exception::LibMausException lme;
						lme.getStream() << "Failed to decompress snappy compressed data, comp=" << datasize << ", uncomp=" << n << ":\n" << ex.what() << "\n";
						lme.finish();
						throw lme;
					}

					readpos += blocksize;
				}
			}
		};		
	}
}
#endif
