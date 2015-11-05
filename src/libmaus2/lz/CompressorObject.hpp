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
#if ! defined(LIBMAUS2_LZ_COMPRESSOROBJECT_HPP)
#define LIBMAUS2_LZ_COMPRESSOROBJECT_HPP

#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct CompressorObject
		{
			typedef CompressorObject this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			virtual ~CompressorObject() {}

			virtual size_t compress(char const * input, size_t inputLength, libmaus2::autoarray::AutoArray<char> & output) = 0;

			virtual std::string getDescription() const = 0;
		};
	}
}
#endif
