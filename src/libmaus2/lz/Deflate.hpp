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

#if ! defined(DEFLATE_HPP)
#define DEFLATE_HPP

#include <libmaus2/types/types.hpp>
#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/exception/LibMausException.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/util/NumberSerialisation.hpp>
#include <libmaus2/lz/GzipHeader.hpp>
#include <zlib.h>
#include <cassert>

namespace libmaus2
{
	namespace lz
	{
		struct Deflate
		{
			static uint64_t const maxbufsize = 256*1024;
			
			typedef libmaus2::aio::OutputStreamInstance::unique_ptr_type out_file_ptr_type;
			
			z_stream strm;
			out_file_ptr_type out_ptr;
			std::ostream & out;
			::libmaus2::autoarray::AutoArray<Bytef> Bin;
			::libmaus2::autoarray::AutoArray<Bytef> Bout;

			void initNoHeader(int const level)
			{
				memset ( &strm , 0, sizeof(z_stream) );
				strm.zalloc = Z_NULL;
				strm.zfree = Z_NULL;
				strm.opaque = Z_NULL;
				int ret = deflateInit2(&strm, level, Z_DEFLATED, -15 /* window size */, 
					8 /* mem level, gzip default */, Z_DEFAULT_STRATEGY);
				if ( ret != Z_OK )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "deflateInit failed.";
					se.finish();
					throw se;
				}
				Bin = ::libmaus2::autoarray::AutoArray<Bytef>(maxbufsize,false);	
				Bout = ::libmaus2::autoarray::AutoArray<Bytef>(maxbufsize,false);	
			}

			void init(int const level)
			{
				memset ( &strm , 0, sizeof(z_stream) );
				strm.zalloc = Z_NULL;
				strm.zfree = Z_NULL;
				strm.opaque = Z_NULL;
				int ret = deflateInit(&strm,level);
				if ( ret != Z_OK )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "deflateInit failed.";
					se.finish();
					throw se;
				}
				Bin = ::libmaus2::autoarray::AutoArray<Bytef>(maxbufsize,false);	
				Bout = ::libmaus2::autoarray::AutoArray<Bytef>(maxbufsize,false);	
			}
			
			Deflate(
				std::ostream & rout,
				int level = Z_DEFAULT_COMPRESSION,
				bool const addHeader = true
			) : out(rout)
			{
				if ( addHeader )
					init(level);
				else
					initNoHeader(level);
			}
			
			Deflate(
				std::string const & filename,
				int level = Z_DEFAULT_COMPRESSION,
				bool const addHeader = true
			) : out_ptr ( new libmaus2::aio::OutputStreamInstance(filename) ),
			    out(*out_ptr)
			{
				if ( addHeader )
					init(level);
				else
					initNoHeader(level);
			}
			
			static uint32_t compressNoHeader(
				char const * p,
				uint64_t const len,
				std::ostream & out,
				int const level = Z_DEFAULT_COMPRESSION
				)
			{
				Deflate defl(out,level,false);
				defl.write(p,len);
				defl.flush();

				uint32_t crc = crc32(0,0,0);
				crc = crc32(crc, reinterpret_cast<Bytef const *>(p), len);        
				
				return crc;
			}
			
			static std::pair<uint64_t,uint32_t> compressNoHeaderMaxLen(
				char const * p, uint64_t len, std::ostream & out,
				uint64_t const maxlen,
				int const level = Z_DEFAULT_COMPRESSION,
				uint64_t const lenred = 1024
				)
			{
				do
				{
					std::ostringstream ostr;
					uint32_t const crc = compressNoHeader(p,len,ostr,level);
					
					if ( ostr.str().size() <= maxlen )
					{
						out.write ( ostr.str().c_str(), ostr.str().size() );
						return std::pair<uint64_t,uint32_t>(len,crc);
					}
					else
					{
						len -= std::min(len,static_cast<uint64_t>(lenred));
					}
				} while ( len );
				
				::libmaus2::exception::LibMausException se;
				se.getStream() << "Unable to compress data into space " << maxlen << std::endl;
				se.finish();
				throw se;
			}

			static std::pair<uint64_t,uint32_t> compressNoHeaderBGZF(
				char const * p, uint64_t len, std::ostream & out, int const level = Z_DEFAULT_COMPRESSION
			)
			{
				return compressNoHeaderMaxLen(p,len,out,64*1024-(18+8),level);
			}

			static std::pair<uint64_t,uint32_t> compressNoHeaderBGZFAdjust(
				char * p, uint64_t const len, std::ostream & out, int const level = Z_DEFAULT_COMPRESSION
			)
			{
				std::pair<uint64_t,uint32_t> const P = compressNoHeaderBGZF(p,len,out,level);
				// remaining data
				uint64_t const rem = len-P.first;
				// move remaining data to front
				::std::memmove(p, p+len-rem, rem);
				//
				return P;
			}
			
			static uint64_t compressBlockBGZF(std::ostream & out, char * p, uint64_t const len,
				int const level = Z_DEFAULT_COMPRESSION)
			{
				std::ostringstream tostr;
				std::pair<uint64_t,uint32_t> const P = compressNoHeaderBGZFAdjust(p,len,tostr,level);
				GzipHeader::writeBGZFHeader(out,tostr.str().size());
				out.write(tostr.str().c_str(),tostr.str().size());
				GzipHeader::putLEInteger(out,P.second,4); // CRC32
				GzipHeader::putLEInteger(out,P.first,4); // uncompressed size
				// std::cerr << "Compressed block of size " << len << " remaining " << len-P.first << std::endl;
				return len-P.first; // unstored rest
			}
			
			static void compressStreamBGZF(std::istream & in, std::ostream & out,
				int const level = Z_DEFAULT_COMPRESSION)
			{
				::libmaus2::autoarray::AutoArray<char> B(64*1024,false);
				uint64_t used = 0;
				
				while ( in )
				{
					in.read(B.begin()+used,B.size()-used);
					used += in.gcount();
					used = compressBlockBGZF(out,B.begin(),used,level);
				}
				
				while ( used )
				{
					used = compressBlockBGZF(out,B.begin(),used,level);
				}
			}
			
			void write(char const * p, uint64_t len)
			{
				while ( len )
				{
					uint64_t const clen = std::min(Bin.size(),len);
					strm.avail_in = clen;
					std::copy(p,p+clen,Bin.begin());
					strm.next_in = Bin.begin();
					
					#if 0
					std::cerr << "Feeding " <<
						std::string(
							reinterpret_cast<char const *>(Bin.begin()),
							reinterpret_cast<char const *>(Bin.begin()+clen))
							<< std::endl;
					#endif
					
					do
					{
						strm.avail_out = Bout.size();
						strm.next_out = Bout.begin();
						int ret = deflate(&strm,Z_NO_FLUSH);
						if ( ret == Z_STREAM_ERROR )
						{
							::libmaus2::exception::LibMausException se;
							se.getStream() << "deflate failed.";
							se.finish();
							throw se;
						}
						uint64_t const have = Bout.size() - strm.avail_out;
						out.write(reinterpret_cast<char const *>(Bout.begin()),have);
					} while (strm.avail_out == 0);
					
					assert ( strm.avail_in == 0);
					
					len -= clen;
					p += clen;
				}
				
				assert ( ! len );
			}
			
			void flush()
			{
				int ret;
				
				do
				{
					strm.avail_in = 0;
					strm.next_in = Bin.begin();
					strm.avail_out = Bout.size();
					strm.next_out = Bout.begin();
					ret = deflate(&strm,Z_FINISH );
					if ( ret == Z_STREAM_ERROR )
					{
						::libmaus2::exception::LibMausException se;
						se.getStream() << "deflate failed.";
						se.finish();
						throw se;
					}
					uint64_t have = Bout.size() - strm.avail_out;
					out.write(reinterpret_cast<char const *>(Bout.begin()),have);
					
					// std::cerr << "Writing " << have << " bytes in flush" << std::endl;
				} while (strm.avail_out == 0);
						
				assert ( ret == Z_STREAM_END );
				
				deflateEnd(&strm);
				out.flush();
				
				if ( out_ptr.get() )
				{
					out_ptr->flush();
					out_ptr.reset();
				}
			}
		};
		
		struct BGZFWriter
		{
			typedef BGZFWriter this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			
			typedef libmaus2::aio::OutputStreamInstance ostream_type;
			typedef ::libmaus2::util::unique_ptr<ostream_type>::type ostream_ptr_type;
			
			ostream_ptr_type Postr;
			std::ostream & ostr;
			int const level;
			
			::libmaus2::autoarray::AutoArray<char> B;
			char * const pa;
			char * pc;
			char * const pe;
			
			BGZFWriter(std::ostream & rostr, int const rlevel = Z_DEFAULT_COMPRESSION)
			: ostr(rostr), level(rlevel), B(64*1024,false), pa(B.begin()), pc(pa), pe(B.end())
			{}
			BGZFWriter(std::string const & filename, int const rlevel = Z_DEFAULT_COMPRESSION)
			: Postr(new ostream_type(filename)), ostr(*Postr), level(rlevel),
			  B(64*1024,false), pa(B.begin()), pc(pa), pe(B.end())
			{}
			
			void flushInternal()
			{
				if ( pc != pa )
				{
					uint64_t const rem = Deflate::compressBlockBGZF(ostr,pa,pc-pa,level);
					assert ( rem != static_cast<uint64_t>(pc-pa) );
					pc = pa + rem;
				}
			}
			
			void flush()
			{
				while ( pc != pa )
					flushInternal();
				ostr.flush();
			}
	
			void write(char const * p, uint64_t len)
			{
				while ( len )
				{
					if ( pc == pe )
						flushInternal();
					
					uint64_t const avail = pe-pc;
					uint64_t const towrite = std::min(avail,len);
					
					std::copy(p,p+towrite,pc);
					p += towrite;
					pc += towrite;
					len -= towrite;
				}
			}
			
			void put(int d)
			{
				uint8_t const c = d;
				write(reinterpret_cast<char const *>(c),1);
			}

			void addEOFBlock()
			{
				std::string input;
				std::istringstream istr(input);
				flush();
				Deflate::compressStreamBGZF(istr,ostr);
				flush();
			}
		};
		
		struct BGZFWriterWrapper
		{
			BGZFWriter writer;
		
			BGZFWriterWrapper(std::ostream & out, int const level = Z_DEFAULT_COMPRESSION) : writer(out,level) {}
			BGZFWriterWrapper(std::string const & filename, int const level = Z_DEFAULT_COMPRESSION) : writer(filename,level) {}
		};
		
		struct BGZFOutputStreamBuffer : public BGZFWriterWrapper, public ::std::streambuf
		{
			uint64_t const buffersize;
			::libmaus2::autoarray::AutoArray<char> buffer;
		
			BGZFOutputStreamBuffer(std::ostream & out, uint64_t const rbuffersize, int const level = Z_DEFAULT_COMPRESSION)
			: BGZFWriterWrapper(out,level), buffersize(rbuffersize), buffer(buffersize,false) 
			{
				setp(buffer.begin(),buffer.end());
			}
			BGZFOutputStreamBuffer(std::string const & filename, uint64_t const rbuffersize, int const level = Z_DEFAULT_COMPRESSION)
			: BGZFWriterWrapper(filename,level), buffersize(rbuffersize), buffer(buffersize,false) 
			{
				setp(buffer.begin(),buffer.end()-1);
			}
			
			int_type overflow(int_type c = traits_type::eof())
			{
				if ( c != traits_type::eof() )
				{
					*pptr() = c;
					pbump(1);
					doSync();
				}

				return c;
			}
			
			void doSync()
			{
				int64_t const n = pptr()-pbase();
				pbump(-n);
				writer.write(pbase(),n);
			}
			int sync()
			{
				doSync();
				writer.flush();
				return 0; // no error, -1 for error
			}
			
			void addEOFBlock()
			{
				writer.addEOFBlock();
			}
		};
		
		struct BGZFOutputStream : public BGZFOutputStreamBuffer, public std::ostream
		{	
			typedef BGZFOutputStream this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
		
			BGZFOutputStream(std::ostream & out, uint64_t const rbuffersize = 64*1024, int const level = Z_DEFAULT_COMPRESSION)
			: BGZFOutputStreamBuffer(out,rbuffersize,level), std::ostream(this)
			{
			}
			BGZFOutputStream(std::string const & filename, uint64_t const rbuffersize = 64*1024, int const level = Z_DEFAULT_COMPRESSION)
			: BGZFOutputStreamBuffer(filename,rbuffersize,level), std::ostream(this)
			{
			}
		};
		struct BlockDeflate
		{
			typedef BlockDeflate this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			
			typedef libmaus2::aio::OutputStreamInstance ostream_type;
			typedef ::libmaus2::util::unique_ptr<ostream_type>::type ostream_ptr_type;
			
			ostream_ptr_type Postr;
			ostream_type & ostr;
			
			std::vector< std::pair<uint64_t, uint64_t> > index;
			
			::libmaus2::autoarray::AutoArray<uint8_t> B;
			uint8_t * const pa;
			uint8_t * pc;
			uint8_t * const pe;
			
			int const level;
			
			BlockDeflate(std::string const & filename, uint64_t const blocksize = 128ull*1024ull, int rlevel = Z_DEFAULT_COMPRESSION)
			: Postr(new ostream_type(filename)), ostr(*Postr),
			  B(blocksize), pa(B.begin()), pc(pa), pe(B.end()), level(rlevel)
			{}
			
			void write(uint8_t const * p, uint64_t n)
			{
				while ( n )
				{
					uint64_t const towrite = std::min(n,static_cast<uint64_t>(pe-pc));
					std::copy ( p, p+towrite, pc );
					pc += towrite;
					p += towrite;
					n -= towrite;
					if ( pc == pe )
						dataFlush();
				}
			}
			
			void put(uint8_t const v)
			{
				*(pc++) = v;
				if ( pc == pe )
					dataFlush();
			}
			
			void dataFlush()
			{
				if ( pc != pa )
				{
					index.push_back ( std::pair<uint64_t,uint64_t>(ostr.tellp(),pc-pa) );
					::libmaus2::util::NumberSerialisation::serialiseNumber(ostr,pc-pa);
					Deflate D(ostr,level);
					D.write( reinterpret_cast<char const *>(pa), pc-pa );
					D.flush();
					pc = pa;
				}
			}
			
			void flush()
			{
				dataFlush();
				uint64_t const indexpos = ostr.tellp();
				::libmaus2::util::NumberSerialisation::serialiseNumber(ostr,index.size());
				for ( uint64_t i = 0; i < index.size(); ++i )
				{
					::libmaus2::util::NumberSerialisation::serialiseNumber(ostr,index[i].first);
					::libmaus2::util::NumberSerialisation::serialiseNumber(ostr,index[i].second);
				}
				::libmaus2::util::NumberSerialisation::serialiseNumber(ostr,indexpos);
				ostr.flush();
			}
		};
	}
}
#endif
