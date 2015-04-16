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
#if ! defined(LIBMAUS2_LZ_SIMPLECOMPRESSEDINPUTBLOCKCONCAT_HPP)
#define LIBMAUS2_LZ_SIMPLECOMPRESSEDINPUTBLOCKCONCAT_HPP

#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/lz/DecompressorObjectFactory.hpp>
#include <libmaus2/util/CountPutObject.hpp>
#include <libmaus2/util/NumberSerialisation.hpp>
#include <libmaus2/util/utf8.hpp>
#include <libmaus2/lz/SimpleCompressedStreamNamedInterval.hpp>
#include <libmaus2/lz/SimpleCompressedInputBlockConcatBlock.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct SimpleCompressedInputBlockConcat
		{
			typedef SimpleCompressedInputBlockConcat this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
			
			std::vector<libmaus2::lz::SimpleCompressedStreamNamedInterval> const intervals;
			std::vector<libmaus2::lz::SimpleCompressedStreamNamedInterval>::const_iterator intervalsIt;
			libmaus2::lz::SimpleCompressedStreamNamedInterval const * currentInterval;
			
			uint64_t streampos;
			
			libmaus2::aio::CheckedInputStream::unique_ptr_type Pcis;
						
			SimpleCompressedInputBlockConcat(
				std::vector<libmaus2::lz::SimpleCompressedStreamNamedInterval> const & rintervals
			)
			: intervals(rintervals), intervalsIt(intervals.begin()), streampos(0)
			{
			
			}
			
			bool readBlock(SimpleCompressedInputBlockConcatBlock & block)
			{
				block.compsize = 0;
				block.uncompsize = 0;
				block.currentInterval = currentInterval;
				block.eof = false;

				// check whether we need to open the next file
				if ( (!(Pcis.get())) )
				{
					// skip over empty intervals
					while ( intervalsIt != intervals.end() && intervalsIt->empty() )
						++intervalsIt;
					
					// check whether we are done
					if ( intervalsIt == intervals.end() )
					{
						block.eof = true;
						return true;
					}
					
					// get interval	
					currentInterval = &(*(intervalsIt++));
					block.currentInterval = currentInterval;
					// open file
					libmaus2::aio::CheckedInputStream::unique_ptr_type Tcis(new libmaus2::aio::CheckedInputStream(currentInterval->name));
					Pcis = UNIQUE_PTR_MOVE(Tcis);
					
					// seek
					Pcis->seekg(currentInterval->start.first);
					streampos = currentInterval->start.first;
				}
				
				block.blockstreampos = streampos;
			
				libmaus2::util::CountPutObject CPO;
				block.uncompsize = libmaus2::util::UTF8::decodeUTF8(*Pcis);
				::libmaus2::util::UTF8::encodeUTF8(block.uncompsize,CPO);
				
				block.compsize = ::libmaus2::util::NumberSerialisation::deserialiseNumber(*Pcis);
				::libmaus2::util::NumberSerialisation::serialiseNumber(CPO,block.compsize);
				
				block.metasize = CPO.c;
				
				if ( block.compsize > block.I.size() )
					block.I = libmaus2::autoarray::AutoArray<uint8_t>(block.compsize,false);
				
				Pcis->read(reinterpret_cast<char *>(block.I.begin()),block.compsize);
				
				streampos += (block.metasize+block.compsize);
				
				bool const gcountok = Pcis->gcount() == static_cast<int64_t>(block.compsize);
				
				if ( block.blockstreampos == currentInterval->end.first )
				{
					Pcis.reset();

					// skip over empty intervals
					while ( intervalsIt != intervals.end() && intervalsIt->empty() )
						++intervalsIt;
						
					if ( intervalsIt == intervals.end() )
						block.eof = true;
				}
					
				if ( gcountok )
					return true;
				else
					return false;
			}	
		};
	}
}
#endif
