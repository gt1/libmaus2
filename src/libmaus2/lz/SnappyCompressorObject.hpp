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
#if ! defined(LIBMAUS_LZ_SNAPPYCOMPRESSOROBJECT_HPP)
#define LIBMAUS_LZ_SNAPPYCOMPRESSOROBJECT_HPP

#include <libmaus2/lz/CompressorObject.hpp>
#include <libmaus2/lz/SnappyCompress.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct SnappyCompressorObject : public CompressorObject
		{
			typedef SnappyCompressorObject this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
			
			SnappyCompressorObject() {}
			~SnappyCompressorObject() {}

			virtual size_t compress(char const * input, size_t inputLength, libmaus2::autoarray::AutoArray<char> & output)
			{
				uint64_t compressBound = SnappyCompress::compressBound(inputLength);
				if ( output.size() < compressBound )
					output = libmaus2::autoarray::AutoArray<char>(compressBound,false);
				
				return SnappyCompress::rawcompress(input,inputLength,output.begin());
			}
			virtual std::string getDescription() const
			{
				return "SnappyCompressorObject";
			}
		};
	}
}
#endif
