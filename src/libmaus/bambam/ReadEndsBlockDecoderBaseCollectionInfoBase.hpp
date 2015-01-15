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
#if ! defined(LIBMAUS_BAMBAM_READENDSBLOCKDECODERBASECOLLECTIONINFOBASE_HPP)
#define LIBMAUS_BAMBAM_READENDSBLOCKDECODERBASECOLLECTIONINFOBASE_HPP

#include <string>
#include <vector>
#include <libmaus/util/unique_ptr.hpp>
#include <libmaus/util/shared_ptr.hpp>

namespace libmaus
{
	namespace bambam
	{
		struct ReadEndsBlockDecoderBaseCollectionInfoBase
		{
			typedef ReadEndsBlockDecoderBaseCollectionInfoBase this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;			

			std::string datafilename;
			std::string indexfilename;
										
			std::vector < uint64_t > blockelcnt;
			std::vector < uint64_t > indexoffset;
			
			ReadEndsBlockDecoderBaseCollectionInfoBase()
			: datafilename(), indexfilename(), blockelcnt(), indexoffset()
			{
			
			}

			ReadEndsBlockDecoderBaseCollectionInfoBase(
				std::string const & rdatafilename,
				std::string const & rindexfilename,
				std::vector < uint64_t > const & rblockelcnt,
				std::vector < uint64_t > const & rindexoffset
			
			)
			: datafilename(rdatafilename), indexfilename(rindexfilename), blockelcnt(rblockelcnt), indexoffset(rindexoffset)
			{
			
			}
			
			ReadEndsBlockDecoderBaseCollectionInfoBase(
				ReadEndsBlockDecoderBaseCollectionInfoBase const & O
			) : datafilename(O.datafilename), indexfilename(O.indexfilename), blockelcnt(O.blockelcnt), indexoffset(O.indexoffset)
			{
			
			}
		};
	}
}
#endif
