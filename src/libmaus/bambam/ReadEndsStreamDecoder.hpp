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
#if ! defined(LIBMAUS_BAMBAM_READENDSSTREAMDECODER_HPP)
#define LIBMAUS_BAMBAM_READENDSSTREAMDECODER_HPP

#include <libmaus/bambam/ReadEndsStreamDecoderFileBase.hpp>
#include <libmaus/bambam/ReadEndsStreamDecoderBase.hpp>

namespace libmaus
{
	namespace bambam
	{		
		struct ReadEndsStreamDecoder : public ReadEndsStreamDecoderFileBase, public ReadEndsStreamDecoderBase
		{
			typedef ReadEndsStreamDecoder this_type;
			//! unique pointer type
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			//! shared pointer type
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			ReadEndsStreamDecoder(std::string const & rfilename)
			: ReadEndsStreamDecoderFileBase(rfilename), ReadEndsStreamDecoderBase(ReadEndsStreamDecoderFileBase::in)
			{}
			
			ReadEndsStreamDecoder(std::istream & rin)
			: ReadEndsStreamDecoderFileBase(rin), ReadEndsStreamDecoderBase(ReadEndsStreamDecoderFileBase::in)
			{
			
			}
		};
	}
}
#endif
