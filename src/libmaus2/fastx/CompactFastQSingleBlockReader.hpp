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
#if ! defined(LIBMAUS_FASTX_COMPACTFASTQSINGLEBLOCKREADER_HPP)
#define LIBMAUS_FASTX_COMPACTFASTQSINGLEBLOCKREADER_HPP

#include <libmaus/fastx/CompactFastQHeader.hpp>
#include <libmaus/fastx/CompactFastQContext.hpp>
#include <libmaus/fastx/CompactFastQDecoderBase.hpp>

namespace libmaus
{
	namespace fastx
	{
		template<typename _stream_type>
		struct CompactFastQSingleBlockReader : public ::libmaus::fastx::CompactFastQHeader, public ::libmaus::fastx::CompactFastQContext, public ::libmaus::fastx::CompactFastQDecoderBase
		{
			typedef _stream_type stream_type;
			typedef CompactFastQSingleBlockReader<stream_type> this_type;
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

			stream_type & stream;
			uint64_t const todecode;
			uint64_t numdecoded;
			
			static uint64_t getEmptyBlockSize()
			{
				return ::libmaus::fastx::CompactFastQHeader::getEmptyBlockHeaderSize();
			}
			
			CompactFastQSingleBlockReader(
				stream_type & rstream, 
				uint64_t const rnextid = 0,
				uint64_t const maxdecode = std::numeric_limits<uint64_t>::max()
			)
			: 
			  ::libmaus::fastx::CompactFastQHeader(rstream),
			  ::libmaus::fastx::CompactFastQContext(rnextid),
			  stream(rstream), 
			  todecode(std::min(numreads,maxdecode)),
			  numdecoded(0)
			{
				// quant.printClosestPhred(std::cerr);	
			}
			
			bool getNextPatternUnlocked(pattern_type & pattern)
			{
				if ( numdecoded >= todecode )
					return false;

				if ( decodePattern(stream,*this,*this,pattern) )
				{
					numdecoded += 1;
					return true;
				}
				else
				{
					return false;
				}
			}
		};
	}
}
#endif
