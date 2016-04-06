/*
    libmaus2
    Copyright (C) 2016 German Tischler

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
#if ! defined(LIBMAUS2_LZ_ZLIBINTERFACE_HPP)
#define LIBMAUS2_LZ_ZLIBINTERFACE_HPP

#include <libmaus2/util/Destructable.hpp>
#include <libmaus2/util/DynamicLoading.hpp>

#include <zlib.h>

namespace libmaus2
{
	namespace lz
	{
		struct ZlibInterface
		{
			typedef ZlibInterface this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			typedef void * (*alloc_function)(void *,unsigned int, unsigned int);
			typedef void (*free_function)(void *,void *);

			private:
			libmaus2::util::Destructable::unique_ptr_type context;
			libmaus2::util::DynamicLibrary zlib;
			libmaus2::util::DynamicLibraryFunction< void (*)(void) > zlib_inflateReset;
			libmaus2::util::DynamicLibraryFunction< void (*)(void) > zlib_inflateInit;
			libmaus2::util::DynamicLibraryFunction< void (*)(void) > zlib_inflateInit2;
			libmaus2::util::DynamicLibraryFunction< void (*)(void) > zlib_inflateEnd;
			libmaus2::util::DynamicLibraryFunction< void (*)(void) > zlib_inflate;
			libmaus2::util::DynamicLibraryFunction< void (*)(void) > zlib_deflateReset;
			libmaus2::util::DynamicLibraryFunction< void (*)(void) > zlib_deflateInit;
			libmaus2::util::DynamicLibraryFunction< void (*)(void) > zlib_deflateInit2;
			libmaus2::util::DynamicLibraryFunction< void (*)(void) > zlib_deflateEnd;
			libmaus2::util::DynamicLibraryFunction< void (*)(void) > zlib_deflate;
			libmaus2::util::DynamicLibraryFunction< void (*)(void) > zlib_deflateBound;

			ZlibInterface(std::string const & libname);

			static void destructContext(void * context);
			static libmaus2::util::Destructable::unique_ptr_type createContext();

			public:
			~ZlibInterface();

			static std::string getDefaultZLibName() { return "libz.so"; }
			static unique_ptr_type construct(std::string const & libname = getDefaultZLibName());

			void eraseContext();

			int z_inflateReset();
			int z_inflateEnd();
			int z_inflateInit();
			int z_inflateInit2(int windowbits);
			int z_inflate(int flush);

			int z_deflateReset();
			int z_deflateInit(int level);
			int z_deflateInit2(int level, int method, int windowBits, int memLevel, int strategy);
			int z_deflateEnd();
			int z_deflate(int flush);
			unsigned long z_deflateBound(unsigned long in);

			void setNextIn(unsigned char * p);
			void setAvailIn(uint64_t const s);
			void setTotalIn(uint64_t const s);
			void setNextOut(unsigned char * p);
			void setAvailOut(uint64_t const s);
			void setTotalOut(uint64_t const s);
			unsigned char * getNextOut();
			uint64_t getAvailOut() const;
			uint64_t getTotalOut() const;
			unsigned char * getNextIn();
			uint64_t getAvailIn() const;
			uint64_t getTotalIn() const;
			void setZAlloc(alloc_function alloc);
			alloc_function getZAlloc();
			void setZFree(free_function f);
			free_function getZFree();
			void setOpaque(void * p);
			void * getOpaque();
		};
	}
}
#endif
