/*
    libmaus2
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
#if !defined(LIBMAUS2_LZ_LZ4COMPRESSWRAPPER_HPP)
#define LIBMAUS2_LZ_LZ4COMPRESSWRAPPER_HPP

#include <libmaus2/lz/Lz4Compress.hpp>
#include <ostream>

namespace libmaus2
{
	namespace lz
	{
		struct Lz4CompressWrapper
		{
			Lz4Compress wrapped;

			Lz4CompressWrapper(std::ostream & rout, uint64_t const inputblocksize)
			: wrapped(rout,inputblocksize) {}
			~Lz4CompressWrapper() {}
		};
	}
}
#endif
