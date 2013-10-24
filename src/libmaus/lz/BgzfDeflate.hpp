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
#if ! defined(LIBMAUS_LZ_BGZFDEFLATE_HPP)
#define LIBMAUS_LZ_BGZFDEFLATE_HPP

#include <libmaus/lz/GzipHeader.hpp>
#include <libmaus/lz/BgzfDeflateBase.hpp>
#include <zlib.h>

namespace libmaus
{
	namespace lz
	{
		template<typename _stream_type>
		struct BgzfDeflate : public BgzfDeflateBase
		{	
			typedef BgzfDeflateBase base_type;
			typedef _stream_type stream_type;
			typedef BgzfDeflate<_stream_type> this_type;
			typedef typename libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			stream_type & stream;

			BgzfDeflate(stream_type & rstream, int const level = Z_DEFAULT_COMPRESSION, bool const rflushmode = false)
			: BgzfDeflateBase(level,rflushmode), stream(rstream)
			{
			}

			void streamWrite(uint8_t const * p, uint64_t const n)
			{
				stream.write(reinterpret_cast<char const *>(p),n);

				if ( ! stream )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "failed to write compressed data to bgzf stream." << std::endl;
					se.finish();
					throw se;				
				}		
			}

			uint64_t flush()
			{
				BgzfDeflateZStreamBaseFlushInfo const BDZSBFI = base_type::flush(flushmode);
				uint64_t const outbytes = BDZSBFI.getCompressedSize();
				/* write data to stream */
				streamWrite(outbuf.begin(),outbytes);
				/* return number of compressed bytes written */
				return outbytes;
			}
			
			void write(char const * const cp, unsigned int n)
			{
				uint8_t const * p = reinterpret_cast<uint8_t const *>(cp);
			
				while ( n )
				{
					unsigned int const towrite = std::min(n,static_cast<unsigned int>(pe-pc));
					std::copy(p,p+towrite,pc);

					p += towrite;
					pc += towrite;
					n -= towrite;
					
					if ( pc == pe )
						flush();
				}
			}

			std::pair<uint64_t,uint64_t> writeSyncedCount(char const * const cp, unsigned int n)
			{
				// check that buffer is empty before we start
				if ( pc != pa )
				{
					libmaus::exception::LibMausException se;
					se.getStream() << "BgzfDeflate::writeSyncedCount() called for unsynced object." << std::endl;
					se.finish();
					throw se;
				}
				
				assert ( ! flushmode ); // flush should write exactly one block
			
				uint64_t bcnt = 0;
				uint64_t ccnt = 0;
				uint8_t const * p = reinterpret_cast<uint8_t const *>(cp);
			
				while ( n )
				{
					unsigned int const towrite = std::min(n,static_cast<unsigned int>(pe-pc));
					std::copy(p,p+towrite,pc);

					p += towrite;
					pc += towrite;
					n -= towrite;
					
					if ( pc == pe )
					{
						ccnt += flush();
						bcnt++;
					}
				}
				
				// drain buffer
				while ( pc != pa )
				{
					ccnt += flush();
					bcnt++;				
				}
				
				return std::pair<uint64_t,uint64_t>(bcnt,ccnt);
			}
			
			void put(uint8_t const c)
			{
				*(pc++) = c;
				if ( pc == pe )
					flush();
			}
			
			void addEOFBlock()
			{
				// flush, if there is any uncompressed data
				while ( pc != pa )
					flush();
				// set compression level to default
				deflatereinit();
				// write empty block
				flush();
			}
		};                                                                                                                                                                                                                                                                                                                                                                                                                                                        

		template<typename _stream_type>
		struct BgzfDeflateWrapper
		{
			typedef _stream_type stream_type;
			
			BgzfDeflate<stream_type> object;

			BgzfDeflateWrapper(stream_type & rstream, int const level = Z_DEFAULT_COMPRESSION, bool const rflushmode = false)
			: object(rstream,level,rflushmode)
			{
			}
			
		};

		struct BgzfOutputStreamBuffer : public BgzfDeflateWrapper<std::ostream>, public ::std::streambuf
		{
			::libmaus::autoarray::AutoArray<char> buffer;
		
			BgzfOutputStreamBuffer(std::ostream & out, int const level = Z_DEFAULT_COMPRESSION)
			: BgzfDeflateWrapper<std::ostream>(out,level,true), buffer(BgzfConstants::getBgzfMaxBlockSize(),false) 
			{
				setp(buffer.begin(),buffer.end());
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
				BgzfDeflateWrapper<std::ostream>::object.write(pbase(),n);
			}
			int sync()
			{
				doSync();
				BgzfDeflateWrapper<std::ostream>::object.flush();
				return 0; // no error, -1 for error
			}
			
			void addEOFBlock()
			{
				BgzfDeflateWrapper<std::ostream>::object.addEOFBlock();
			}
		};
		
		struct BgzfOutputStream : public BgzfOutputStreamBuffer, public std::ostream
		{	
			typedef BgzfOutputStream this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
		
			BgzfOutputStream(std::ostream & out, int const level = Z_DEFAULT_COMPRESSION)
			: BgzfOutputStreamBuffer(out,level), std::ostream(this)
			{
			}
		};

	}
}
#endif
