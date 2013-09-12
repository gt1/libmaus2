/*
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
*/
#if ! defined(LIBMAUS_LZ_SNAPPYCOMPRESS_HPP)
#define LIBMAUS_LZ_SNAPPYCOMPRESS_HPP

#include <libmaus/LibMausConfig.hpp>
#include <libmaus/util/utf8.hpp>
#include <libmaus/util/NumberSerialisation.hpp>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/lz/IstreamSource.hpp>
#include <libmaus/aio/IStreamWrapper.hpp>
#include <libmaus/aio/CheckedOutputStream.hpp>
#include <istream>
#include <ostream>
#include <string>
#include <sstream>

namespace libmaus
{
	namespace lz
	{
		struct SnappyCompress
		{
			static uint64_t compress(std::istream & in, uint64_t const n, std::ostream & out);
			static uint64_t compress(char const * in, uint64_t const n, std::ostream & out);
			static uint64_t compress(::libmaus::lz::IstreamSource< ::libmaus::aio::IStreamWrapper> & in, std::ostream & out);

			static void uncompress(char const * in, uint64_t const insize, std::string & out);
			static void uncompress(std::istream & in, uint64_t const insize, char * out);
			static void uncompress(::libmaus::lz::IstreamSource< ::libmaus::aio::IStreamWrapper> & in, char * out, int64_t const length = -1);

			static std::string compress(std::istream & in, uint64_t const n)
			{
				std::ostringstream out;
				compress(in,n,out);
				return out.str();
			}
			static std::string compress(char const * in, uint64_t const n)
			{
				std::ostringstream ostr;
				compress(in,n,ostr);
				return ostr.str();
			}
			static std::string compress(std::string const & in)
			{
				return compress(in.c_str(),in.size());
			}
			static void uncompress(std::string const & in, std::string & out)
			{
				uncompress(in.c_str(),in.size(),out);
			}
			static std::string uncompress(char const * in, uint64_t const insize)
			{
				std::string out;
				uncompress(in,insize,out);
				return out;
			}
			static std::string uncompress(std::string const & in)
			{
				return uncompress(in.c_str(),in.size());
			}
		};
		
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

		struct SnappyFileOutputStream
		{
			typedef SnappyFileOutputStream this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
		
			// std::ofstream ostr;
			::libmaus::aio::CheckedOutputStream ostr;
			SnappyOutputStream< ::libmaus::aio::CheckedOutputStream > sos;

			SnappyFileOutputStream(std::string const & filename, uint64_t const bufsize = 64*1024)
			: ostr(filename.c_str()), sos(ostr,bufsize) {}
			
			void flush()
			{
				sos.flush();
				ostr.flush();
			}
			
			void put(int const c)
			{
				sos.put(c);
			}
			
			void write(char const * c, uint64_t const n)
			{
				sos.write(c,n);
			}
		};
		
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
			bool const bigbuf;
			uint64_t gcnt;
			
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
			  B(), pa(0), pc(0), pe(0), bigbuf(checkedGet()), gcnt(0)
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
						return -1;
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
				
					uint64_t blocksize = sizeof(uint64_t) + ( bigbuf ? sizeof(uint64_t) : 0 );
					
					// size of uncompressed buffer
					uint64_t const n = 
						bigbuf ?
							::libmaus::util::NumberSerialisation::deserialiseNumber(in)
							:
							::libmaus::util::UTF8::decodeUTF8(in,blocksize)
						;

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

					SnappyCompress::uncompress(insource,B.begin(),n);

					readpos += blocksize;
				}
			}
		};
		
		struct SnappyFileInputStream
		{
			std::ifstream istr;
			SnappyInputStream instream;
			
			SnappyFileInputStream(std::string const & filename)
			: istr(filename.c_str(),std::ios::binary), instream(istr,0,true) {}
			int get() { return instream.get(); }
			uint64_t read(char * c, uint64_t const n) { return instream.read(c,n); }
			uint64_t gcount() const { return instream.gcount(); }
		};

		struct SnappyOffsetFileInputStream
		{
			typedef SnappyOffsetFileInputStream this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
		
			typedef libmaus::aio::CheckedInputStream stream_type;
			typedef stream_type::unique_ptr_type stream_ptr_type;
		
			#if 0
			::libmaus::util::unique_ptr<std::ifstream>::type Pistr;
			std::ifstream & istr;
			#endif
			stream_ptr_type Pistr;
			stream_type & istr;
			SnappyInputStream instream;
			
			stream_ptr_type openFileAtOffset(std::string const & filename, uint64_t const offset)
			{
				stream_ptr_type Pistr(new stream_type(filename.c_str()));
				Pistr->seekg(offset,std::ios::beg);
				
				return UNIQUE_PTR_MOVE(Pistr);
			}
			
			SnappyOffsetFileInputStream(std::string const & filename, uint64_t const roffset)
			: Pistr(openFileAtOffset(filename,roffset)), 
			  istr(*Pistr), instream(istr,roffset,true) {}
			int get() { return instream.get(); }
			int peek() { return instream.peek(); }
			uint64_t read(char * c, uint64_t const n) { return instream.read(c,n); }
			uint64_t gcount() const { return instream.gcount(); }
		};

		struct SnappyStringInputStream
		{
			std::istringstream istr;
			SnappyInputStream instream;

			SnappyStringInputStream(std::string const & data)
			: istr(data), instream(istr,0,true)
			{
			
			}
			int get() { return instream.get(); }
			uint64_t read(char * c, uint64_t const n) { return instream.read(c,n); }
			uint64_t gcount() const { return instream.gcount(); }
		};

		struct SnappyInputStreamArray
		{
			::libmaus::autoarray::AutoArray<SnappyInputStream::unique_ptr_type> A;

			template<typename iterator>			
			SnappyInputStreamArray(std::istream & in, iterator offa, iterator offe)
			: A(offe-offa)
			{
				uint64_t i = 0;
				for ( iterator offc = offa; offc != offe && offc+1 != offe; ++offc, ++i )
				{
					SnappyInputStream::unique_ptr_type ptr(
                                                        new SnappyInputStream(in,*offc,true,*(offc+1))
                                                );
					A [ i ] = UNIQUE_PTR_MOVE(ptr);
				}
			}
			
			SnappyInputStream & operator[](uint64_t const i)
			{
				return *(A[i]);
			}

			SnappyInputStream const & operator[](uint64_t const i) const
			{
				return *(A[i]);
			}
		};
		
		struct SnappyInputStreamArrayFile
		{
			typedef SnappyInputStreamArrayFile this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			std::ifstream istr;
			SnappyInputStreamArray array;
			
			template<typename iterator>
			SnappyInputStreamArrayFile(std::string const & filename, iterator offa, iterator offe)
			: istr(filename.c_str(),std::ios::binary), array(istr,offa,offe)
			{
			
			}
			
			template<typename iterator>
			static unique_ptr_type construct(std::string const & filename, iterator offa, iterator offe)
			{
				unique_ptr_type ptr(new this_type(filename,offa,offe));
				return UNIQUE_PTR_MOVE(ptr);
			}
			
			static unique_ptr_type construct(std::string const & filename, std::vector<uint64_t> const & V)
			{
				unique_ptr_type ptr(construct(filename,V.begin(),V.end()));
				return UNIQUE_PTR_MOVE(ptr);
			}

			SnappyInputStream & operator[](uint64_t const i)
			{
				return array[i];
			}

			SnappyInputStream const & operator[](uint64_t const i) const
			{
				return array[i];
			}
		};
	}
}
#endif
