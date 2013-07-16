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

			public:	
			BgzfInflate(stream_type & rstream) : stream(rstream), gcnt(0), ostr(0) {}
			BgzfInflate(stream_type & rstream, std::ostream & rostr) : stream(rstream), gcnt(0), ostr(&rostr) {}

			uint64_t read(char * const decomp, uint64_t const n)
			{
				gcnt = 0;
			
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
				
				return gcnt;
			}

			std::pair<uint64_t,uint64_t> readPlusInfo(char * const decomp, uint64_t const n)
			{
				gcnt = 0;
			
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
