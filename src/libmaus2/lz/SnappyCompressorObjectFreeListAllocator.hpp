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
#if ! defined(LIBMAUS_LZ_SNAPPYCOMPRESSOROBJECTFREELISTALLOCATOR_HPP)
#define LIBMAUS_LZ_SNAPPYCOMPRESSOROBJECTFREELISTALLOCATOR_HPP

#include <libmaus/lz/CompressorObjectFreeListAllocator.hpp>
#include <libmaus/lz/SnappyCompressorObjectFactory.hpp>
#include <libmaus/lz/SnappyCompressorObjectFactoryWrapper.hpp>

namespace libmaus
{
	namespace lz
	{
		struct SnappyCompressorObjectFreeListAllocator : 
			public libmaus::lz::SnappyCompressorObjectFactoryWrapper, 
			public libmaus::lz::CompressorObjectFreeListAllocator
		{
			SnappyCompressorObjectFreeListAllocator()
			: libmaus::lz::SnappyCompressorObjectFactoryWrapper(libmaus::lz::SnappyCompressorObjectFactory::shared_ptr_type(new SnappyCompressorObjectFactory)), 
			  libmaus::lz::CompressorObjectFreeListAllocator(libmaus::lz::SnappyCompressorObjectFactoryWrapper::factory) {}
		};
	}
}
#endif
