/*
    libmaus
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
#if ! defined(LIBMAUS_LZ_ISGZIP_HPP)
#define LIBMAUS_LZ_ISGZIP_HPP

#include <istream>
#include <libmaus/lz/GzipHeader.hpp>

namespace libmaus
{
	namespace lz
	{
		struct IsGzip
		{
			static bool isGzip(std::istream & in)
			{
				int const b0 = in.get();
				
				if ( b0 < 0 )
					return false;
				
				int const b1 = in.get();
				
				if ( b1 < 0 )
				{
					in.clear();
					in.putback(b0);
					return false;
				}
				
				in.clear();
				in.putback(b1);
				in.putback(b0);
				
				return b0 == libmaus::lz::GzipHeaderConstantsBase::ID1 &&
				       b1 == libmaus::lz::GzipHeaderConstantsBase::ID2;
			}
		};
	}
}
#endif
