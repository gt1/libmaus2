/*
    libmaus2
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
#if ! defined(LIBMAUS2_BAMBAM_READENDSSTREAMDECODERFILEBASE_HPP)
#define LIBMAUS2_BAMBAM_READENDSSTREAMDECODERFILEBASE_HPP

#include <libmaus2/aio/CheckedInputStream.hpp>

namespace libmaus2
{
	namespace bambam
	{
		struct ReadEndsStreamDecoderFileBase
		{
			typedef ReadEndsStreamDecoderFileBase this_type;
			//! unique pointer type
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			//! shared pointer type
			typedef ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
			
			std::string const filename;
			libmaus2::aio::CheckedInputStream::unique_ptr_type pCIS;
			std::istream * in;
			
			ReadEndsStreamDecoderFileBase(std::string const & rfilename)
			: filename(rfilename), pCIS(new libmaus2::aio::CheckedInputStream(filename)), in(pCIS.get())
			{}
			
			ReadEndsStreamDecoderFileBase(std::istream & rin)
			: filename(), pCIS(), in(&rin)
			{
			
			}

			ReadEndsStreamDecoderFileBase()
			: filename(), pCIS(), in()
			{
			
			}
		
		};
	}
}
#endif
