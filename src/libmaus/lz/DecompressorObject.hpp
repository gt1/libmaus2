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
#if ! defined(LIBMAUS_LZ_DECOMPRESSOROBJECT_HPP)
#define LIBMAUS_LZ_DECOMPRESSOROBJECT_HPP

#include <libmaus/util/unique_ptr.hpp>

namespace libmaus
{
	namespace lz
	{
		struct DecompressorObject
		{
			typedef DecompressorObject this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			
			virtual ~DecompressorObject() {}
			virtual bool rawuncompress(
				char const * compressed, 
				size_t compressed_length, 
				char * uncompressed,
				size_t uncompressed_length) = 0;
		};
	}
}
#endif
