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
#if ! defined(LIBMAUS_LZ_ZLIBCOMPRESSOROBJECTFREELISTALLOCATOR_HPP)
#define LIBMAUS_LZ_ZLIBCOMPRESSOROBJECTFREELISTALLOCATOR_HPP

#include <libmaus2/lz/CompressorObjectFreeListAllocator.hpp>
#include <libmaus2/lz/ZlibCompressorObjectFactory.hpp>
#include <libmaus2/lz/ZlibCompressorObjectFactoryWrapper.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct ZlibCompressorObjectFreeListAllocator : 
			public libmaus2::lz::ZlibCompressorObjectFactoryWrapper, 
			public libmaus2::lz::CompressorObjectFreeListAllocator
		{
			ZlibCompressorObjectFreeListAllocator(int const level = Z_DEFAULT_COMPRESSION)
			: libmaus2::lz::ZlibCompressorObjectFactoryWrapper(libmaus2::lz::ZlibCompressorObjectFactory::shared_ptr_type(new ZlibCompressorObjectFactory(level))), 
			  libmaus2::lz::CompressorObjectFreeListAllocator(libmaus2::lz::ZlibCompressorObjectFactoryWrapper::factory) {}
		};
	}
}
#endif
