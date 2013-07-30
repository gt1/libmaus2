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
#if ! defined(LIBMAUS_LZ_BGZFINFLATE_HPP)
#define LIBMAUS_LZ_BGZFINFLATE_HPP

#include <libmaus/lz/BgzfInflateBase.hpp>
#include <libmaus/lz/BgzfVirtualOffset.hpp>
#include <ostream>

namespace libmaus
{
	namespace lz
	{
		template<typename _stream_type>
		struct BgzfInflate : public BgzfInflateBase
		{
			typedef _stream_type stream_type;

			private:
			stream_type & stream;
			uint64_t gcnt;
			std::ostream * ostr;

			bool const haveoffsets;
			libmaus::lz::BgzfVirtualOffset const startoffset;
			libmaus::lz::BgzfVirtualOffset const endoffset;
			
			uint64_t compressedread;
			
			bool terminated;

			public:	
			BgzfInflate(stream_type & rstream) 
			: stream(rstream), gcnt(0), ostr(0), 
			  haveoffsets(false), startoffset(0), endoffset(0), compressedread(0), terminated(false) {}
			BgzfInflate(
				stream_type & rstream, 
				libmaus::lz::BgzfVirtualOffset const rstartoffset,
				libmaus::lz::BgzfVirtualOffset const rendoffset
			) 
			: 
				stream(rstream),
				gcnt(0), ostr(0), 
				haveoffsets(true), startoffset(rstartoffset), endoffset(rendoffset), 
				compressedread(0), terminated(false) 
			{
				stream.clear();
				stream.seekg(startoffset.getBlockOffset(), std::ios::beg);
				stream.clear();
			}
			BgzfInflate(stream_type & rstream, std::ostream & rostr) 
			: stream(rstream), gcnt(0), ostr(&rostr), 
			  haveoffsets(false), startoffset(0), endoffset(0), compressedread(0), terminated(false) {}

			uint64_t read(char * const decomp, uint64_t const n)
			{			
				gcnt = 0;

				if ( terminated )
					return 0;
				
				bool const firstblock = haveoffsets && (compressedread == 0);	
				bool const lastblock = haveoffsets && (compressedread == endoffset.getBlockOffset()-startoffset.getBlockOffset());
			
				/* check if buffer given is large enough */	
				if ( n < getBgzfMaxBlockSize() )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "BgzfInflate::decompressBlock(): provided buffer is too small: " << n << " < " << getBgzfMaxBlockSize();
					se.finish();
					throw se;				
				}
				
				/* read block */
				std::pair<uint64_t,uint64_t> const blockinfo = readBlock(stream);

				/* no more data ? */
				if ( ! blockinfo.first )
				{
					if ( ostr )
					{
						ostr->flush();

						if ( ! (*ostr) )
						{
							libmaus::exception::LibMausException ex;
							ex.getStream() << "BgzfInflate::read(): failed to flush copy stream." << std::endl;
							ex.finish();
							throw ex;
						}
					}
					
					return 0;
				}
					
				if ( ostr )
				{
					ostr->write(reinterpret_cast<char const *>(header.begin()),getBgzfHeaderSize());
					ostr->write(reinterpret_cast<char const *>(block.begin()),blockinfo.first + getBgzfFooterSize());
					
					if ( ! (*ostr) )
					{
						libmaus::exception::LibMausException ex;
						ex.getStream() << "BgzfInflate::read(): failed to write compressed input to copy stream." << std::endl;
						ex.finish();
						throw ex;
					}
				}

				/* decompress block */
				gcnt = decompressBlock(decomp,blockinfo);
				
				if ( lastblock )
				{
					gcnt = std::min(gcnt,endoffset.getSubOffset());
					terminated = true;
				}
				if ( firstblock )
				{
					uint64_t const soff = startoffset.getSubOffset();
					if ( soff )
					{
						memmove(decomp,decomp+soff,gcnt-soff);
						gcnt -= soff;
					}
				}
				
				compressedread += getBgzfHeaderSize() + blockinfo.first + getBgzfFooterSize();
				
				return gcnt;
			}

			std::pair<uint64_t,uint64_t> readPlusInfo(char * const decomp, uint64_t const n)
			{
				gcnt = 0;

				if ( terminated )
					return 0;

				bool const firstblock = haveoffsets && (compressedread == 0);	
				bool const lastblock = haveoffsets && (compressedread == endoffset.getBlockOffset()-startoffset.getBlockOffset());
			
				/* check if buffer given is large enough */	
				if ( n < getBgzfMaxBlockSize() )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "BgzfInflate::decompressBlock(): provided buffer is too small: " << n << " < " << getBgzfMaxBlockSize();
					se.finish();
					throw se;				
				}
				
				/* read block */
				std::pair<uint64_t,uint64_t> const blockinfo = readBlock(stream);

				/* no more data ? */
				if ( ! blockinfo.first )
				{
					if ( ostr )
					{
						ostr->flush();

						if ( ! (*ostr) )
						{
							libmaus::exception::LibMausException ex;
							ex.getStream() << "BgzfInflate::read(): failed to flush copy stream." << std::endl;
							ex.finish();
							throw ex;
						}
					}
					
					return std::pair<uint64_t,uint64_t>(0,0);
				}
					
				if ( ostr )
				{
					ostr->write(reinterpret_cast<char const *>(header.begin()),getBgzfHeaderSize());
					ostr->write(reinterpret_cast<char const *>(block.begin()),blockinfo.first + getBgzfFooterSize());
					
					if ( ! (*ostr) )
					{
						libmaus::exception::LibMausException ex;
						ex.getStream() << "BgzfInflate::read(): failed to write compressed input to copy stream." << std::endl;
						ex.finish();
						throw ex;
					}
				}

				/* decompress block */
				gcnt = decompressBlock(decomp,blockinfo);

				if ( lastblock )
				{
					gcnt = std::min(gcnt,endoffset.getSubOffset());
					terminated = true;
				}
				if ( firstblock )
				{
					uint64_t const soff = startoffset.getSubOffset();
					if ( soff )
					{
						memmove(decomp,decomp+soff,gcnt-soff);
						gcnt -= soff;
					}
				}

				compressedread += getBgzfHeaderSize() + blockinfo.first + getBgzfFooterSize();
				
				return std::pair<uint64_t,uint64_t>(
					blockinfo.first+getBgzfHeaderSize()+getBgzfFooterSize(),
					gcnt // uncompressed size
				);
			}
			
			uint64_t gcount() const
			{
				return gcnt;
			}
		};
	}
}
#endif
