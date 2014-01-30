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
#include <libmaus/lz/BgzfInflateInfo.hpp>
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


			BgzfInflateInfo readAndInfo(char * const decomp, uint64_t const n)
			{
				/* check if buffer given is large enough */	
				if ( n < getBgzfMaxBlockSize() )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "BgzfInflate::decompressBlock(): provided buffer is too small: " << n << " < " << getBgzfMaxBlockSize();
					se.finish();
					throw se;				
				}
				
				/* reset gcnt */
				gcnt = 0;

				/* return eof if terminate flag is set (we reached the end of the given interval) */
				if ( terminated )
					return BgzfInflateInfo(0,0,true);

				/* first block flag if we are processing an interval on the file */
				bool const firstblock = haveoffsets && (compressedread == 0);	
				/* last block flag if we are processing an interval on the file	*/
				bool const lastblock = haveoffsets && (compressedread == endoffset.getBlockOffset()-startoffset.getBlockOffset());
			
				/* read block */
				std::pair<uint64_t,uint64_t> const blockinfo = readBlock(stream);

				/* copy compressed block if ostr is not null */
				if ( ostr )
				{
					ostr->write(reinterpret_cast<char const *>(header.begin()),getBgzfHeaderSize());
					ostr->write(reinterpret_cast<char const *>(block.begin()),blockinfo.first + getBgzfFooterSize());
					
					if ( ! (*ostr) )
					{
						libmaus::exception::LibMausException ex;
						ex.getStream() << "BgzfInflate::readAndInfo(): failed to write compressed input to copy stream." << std::endl;
						ex.finish();
						throw ex;
					}
				}

				/* check for empty block and flush copy stream if necessary */
				if ( (! blockinfo.second) && ostr )
				{
					ostr->flush();

					/* check for I/O failure */
					if ( ! (*ostr) )
					{
						libmaus::exception::LibMausException ex;
						ex.getStream() << "BgzfInflate::readAndInfo(): failed to flush copy stream." << std::endl;
						ex.finish();
						throw ex;
					}
				}

				/* decompress block */
				gcnt = decompressBlock(decomp,blockinfo);

				/* if this is the last block we will read, then set the terminate flag
				  and cut off */
				if ( lastblock )
				{
					gcnt = std::min(gcnt,endoffset.getSubOffset());
					terminated = true;
				}
				/* if this is the first block then move data into place as required */
				if ( firstblock )
				{
					uint64_t const soff = startoffset.getSubOffset();
					if ( soff )
					{
						memmove(decomp,decomp+soff,gcnt-soff);
						gcnt -= soff;
					}
				}

				/* increase number of compressed bytes we have read by this block */
				compressedread += getBgzfHeaderSize() + blockinfo.first + getBgzfFooterSize();
				
				return BgzfInflateInfo(
					blockinfo.first+getBgzfHeaderSize()+getBgzfFooterSize(),
					gcnt,
					gcnt ? false : (stream.peek() < 0)
				);
			}

			/**
			 * read a BGZF block
			 * 
			 * @param decomp space for decompressed data
			 * @param n number of bytes availabel in decomp, must be at least the BGZF block size (64k)
			 * @return (number of compressed bytes read, number of uncompressed bytes returned)
			 **/
			std::pair<uint64_t,uint64_t> readPlusInfo(char * const decomp, uint64_t const n)
			{
				BgzfInflateInfo const info = readAndInfo(decomp,n);				
				return std::pair<uint64_t,uint64_t>(info.compressed,info.uncompressed);
			}

			/**
			 * read a bgzf block
			 *
			 * @param decomp space for decompressed data
			 * @param n number of bytes availabel in decomp, must be at least the BGZF block size (64k)
			 * @return number of bytes in block
			 **/
			uint64_t read(char * const decomp, uint64_t const n)
			{			
				return readAndInfo(decomp,n).uncompressed;
			}

			/**
			 * @return number of uncompressed bytes returned by last read call
			 **/			
			uint64_t gcount() const
			{
				return gcnt;
			}
		};
	}
}
#endif
