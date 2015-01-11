/*
    libmaus
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2015 Genome Research Limited

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
#if ! defined(LIBMAUS_BAMBAM_READENDSBLOCKDECODERBASECOLLECTION_HPP)
#define LIBMAUS_BAMBAM_READENDSBLOCKDECODERBASECOLLECTION_HPP

#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/bambam/BamAlignment.hpp>
#include <libmaus/bambam/CompactReadEndsBase.hpp>
#include <libmaus/bambam/CompactReadEndsComparator.hpp>
#include <libmaus/bambam/ReadEnds.hpp>
#include <libmaus/bambam/ReadEndsContainerBase.hpp>
#include <libmaus/bambam/SortedFragDecoder.hpp>
#include <libmaus/util/DigitTable.hpp>
#include <libmaus/util/CountGetObject.hpp>
#include <libmaus/sorting/SerialRadixSort64.hpp>
#include <libmaus/lz/SnappyOutputStream.hpp>

#include <libmaus/bambam/ReadEndsBlockDecoderBase.hpp>

namespace libmaus
{
	namespace bambam
	{
		struct ReadEndsBlockDecoderBaseCollection
		{
			typedef ReadEndsBlockDecoderBaseCollection this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

			std::string const datafilename;
			std::string const indexfilename;
			
			libmaus::aio::CheckedInputStream datastr;
			libmaus::aio::CheckedInputStream indexstr;
							
			std::vector < std::pair<uint64_t,uint64_t> > const dataoffset;
			std::vector < uint64_t > const blockelcnt;
			std::vector < uint64_t > const indexoffset;
			
			libmaus::autoarray::AutoArray< ::libmaus::bambam::ReadEndsBlockDecoderBase::unique_ptr_type> Adecoders;
			
			ReadEndsBlockDecoderBaseCollection(
				std::string const & rdatafilename,
				std::string const & rindexfilename,
				std::vector < std::pair<uint64_t,uint64_t> > const & rdataoffset,
				std::vector < uint64_t > const & rblockelcnt,
				std::vector < uint64_t > const & rindexoffset
			) : datafilename(rdatafilename), indexfilename(rindexfilename),
			    datastr(datafilename), indexstr(indexfilename),
			    dataoffset(rdataoffset),
			    blockelcnt(rblockelcnt),
			    indexoffset(rindexoffset),
			    Adecoders(indexoffset.size(),false)
			{
				for ( uint64_t i = 0; i < Adecoders.size(); ++i )
				{
					libmaus::bambam::ReadEndsBlockDecoderBase::unique_ptr_type tptr(
						new libmaus::bambam::ReadEndsBlockDecoderBase(
							datastr,
							indexstr,
							dataoffset.at(i),
							indexoffset.at(i),
							blockelcnt.at(i)
						)
					);
					
					Adecoders[i] = UNIQUE_PTR_MOVE(tptr);
				}
			}
			
			uint64_t size() const
			{
				return blockelcnt.size();
			}
			
			::libmaus::bambam::ReadEndsBlockDecoderBase * getBlock(uint64_t const i)
			{
				return Adecoders[i].get();
			}
			
			ReadEnds max()
			{
				ReadEnds RE;
				for ( uint64_t i = 0; i < size(); ++i )
					if ( getBlock(i) && getBlock(i)->size() )
						RE = std::max(RE,getBlock(i)->get(getBlock(i)->size()-1));
				return RE;
			}
		};
	}
}
#endif
