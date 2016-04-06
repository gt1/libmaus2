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
#include <libmaus2/lz/ZlibInterface.hpp>
#include <cstring>
#include <map>
#include <cassert>
#include <libmaus2/util/DynamicLoading.hpp>
#include <zlib.h>

libmaus2::parallel::PosixMutex libmaus2::lz::ZlibInterface::initlock;

#if defined(LIBMAUS2_HAVE_DL_FUNCS)
struct ZlibFunctions
{
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

	int (*p_inflateReset)(z_stream *);
	int (*p_inflateEnd)(z_stream *);
	int (*p_inflateInit)(z_stream *, char const *, int);
	int (*p_inflateInit2)(z_stream *, int, char const *, int);
	int (*p_inflate)(z_stream *, int);
	int (*p_deflateReset)(z_stream *);
	int (*p_deflateEnd)(z_stream *);
	int (*p_deflateInit)(z_stream *, int, char const *, int);
	int (*p_deflateInit2)(z_stream *, int, int, int, int, int, char const *, int);
	int (*p_deflate)(z_stream *, int);
	unsigned long (*p_deflateBound)(z_stream *, unsigned long);

	ZlibFunctions(std::string const & libname) :
		zlib(libname),
		zlib_inflateReset(zlib,"inflateReset"),
		zlib_inflateInit(zlib,"inflateInit_"),
		zlib_inflateInit2(zlib,"inflateInit2_"),
		zlib_inflateEnd(zlib,"inflateEnd"),
		zlib_inflate(zlib,"inflate"),
		zlib_deflateReset(zlib,"deflateReset"),
		zlib_deflateInit(zlib,"deflateInit_"),
		zlib_deflateInit2(zlib,"deflateInit2_"),
		zlib_deflateEnd(zlib,"deflateEnd"),
		zlib_deflate(zlib,"deflate"),
		zlib_deflateBound(zlib,"deflateBound"),
		p_inflateReset(reinterpret_cast< int (*)(z_stream *) >(zlib_inflateReset.func)),
		p_inflateEnd(reinterpret_cast< int (*)(z_stream *) >(zlib_inflateEnd.func)),
		p_inflateInit(reinterpret_cast< int (*)(z_stream *, char const *, int) >(zlib_inflateInit.func)),
		p_inflateInit2(reinterpret_cast< int (*)(z_stream *, int, char const *, int) >(zlib_inflateInit2.func)),
		p_inflate(reinterpret_cast< int (*)(z_stream *, int) >(zlib_inflate.func)),
		p_deflateReset(reinterpret_cast< int (*)(z_stream *) >(zlib_deflateReset.func)),
		p_deflateEnd(reinterpret_cast< int (*)(z_stream *) >(zlib_deflateEnd.func)),
		p_deflateInit(reinterpret_cast< int (*)(z_stream *, int, char const *, int) >(zlib_deflateInit.func)),
		p_deflateInit2(reinterpret_cast< int (*)(z_stream *, int, int, int, int, int, char const *, int) >(zlib_deflateInit2.func)),
		p_deflate(reinterpret_cast< int (*)(z_stream *, int) >(zlib_deflate.func)),
		p_deflateBound(reinterpret_cast< unsigned long (*)(z_stream *, unsigned long) >(zlib_deflateBound.func))
	{

	}

	static void destruct(void * p)
	{
		delete reinterpret_cast<ZlibFunctions *>(p);
	}

	static libmaus2::util::Destructable::unique_ptr_type construct(std::string const & s)
	{
		ZlibFunctions * f = 0;

		try
		{
			f = new ZlibFunctions(s);
			libmaus2::util::Destructable::unique_ptr_type tptr(libmaus2::util::Destructable::construct(f,destruct));
			return UNIQUE_PTR_MOVE(tptr);
		}
		catch(...)
		{
			delete f;
			throw;
		}
	}

	static libmaus2::util::Destructable::shared_ptr_type sconstruct(std::string const & s)
	{
		ZlibFunctions * f = 0;

		try
		{
			f = new ZlibFunctions(s);
			libmaus2::util::Destructable::shared_ptr_type tptr(libmaus2::util::Destructable::sconstruct(f,destruct));
			return tptr;
		}
		catch(...)
		{
			delete f;
			throw;
		}
	}
};
#else
struct ZlibFunctions
{
	int (*p_inflateReset)(z_stream *);
	int (*p_inflateEnd)(z_stream *);
	int (*p_inflateInit)(z_stream *, char const *, int);
	int (*p_inflateInit2)(z_stream *, int, char const *, int);
	int (*p_inflate)(z_stream *, int);
	int (*p_deflateReset)(z_stream *);
	int (*p_deflateEnd)(z_stream *);
	int (*p_deflateInit)(z_stream *, int, char const *, int);
	int (*p_deflateInit2)(z_stream *, int, int, int, int, int, char const *, int);
	int (*p_deflate)(z_stream *, int);
	unsigned long (*p_deflateBound)(z_stream *, unsigned long);

	ZlibFunctions(std::string const & /* libname */) :
		p_inflateReset(reinterpret_cast< int (*)(z_stream *) >(::inflateReset)),
		p_inflateEnd(reinterpret_cast< int (*)(z_stream *) >(::inflateEnd)),
		p_inflateInit(reinterpret_cast< int (*)(z_stream *, char const *, int) >(::inflateInit_)),
		p_inflateInit2(reinterpret_cast< int (*)(z_stream *, int, char const *, int) >(::inflateInit2_)),
		p_inflate(reinterpret_cast< int (*)(z_stream *, int) >(::inflate)),
		p_deflateReset(reinterpret_cast< int (*)(z_stream *) >(::deflateReset)),
		p_deflateEnd(reinterpret_cast< int (*)(z_stream *) >(::deflateEnd)),
		p_deflateInit(reinterpret_cast< int (*)(z_stream *, int, char const *, int) >(::deflateInit_)),
		p_deflateInit2(reinterpret_cast< int (*)(z_stream *, int, int, int, int, int, char const *, int) >(::deflateInit2_)),
		p_deflate(reinterpret_cast< int (*)(z_stream *, int) >(::deflate)),
		p_deflateBound(reinterpret_cast< unsigned long (*)(z_stream *, unsigned long) >(deflateBound))
	{

	}

	static void destruct(void * p)
	{
		delete reinterpret_cast<ZlibFunctions *>(p);
	}

	static libmaus2::util::Destructable::unique_ptr_type construct(std::string const & s)
	{
		ZlibFunctions * f = 0;

		try
		{
			f = new ZlibFunctions(s);
			libmaus2::util::Destructable::unique_ptr_type tptr(libmaus2::util::Destructable::construct(f,destruct));
			return UNIQUE_PTR_MOVE(tptr);
		}
		catch(...)
		{
			delete f;
			throw;
		}
	}

	static libmaus2::util::Destructable::shared_ptr_type sconstruct(std::string const & s)
	{
		ZlibFunctions * f = 0;

		try
		{
			f = new ZlibFunctions(s);
			libmaus2::util::Destructable::shared_ptr_type tptr(libmaus2::util::Destructable::sconstruct(f,destruct));
			return tptr;
		}
		catch(...)
		{
			delete f;
			throw;
		}
	}
};

#endif

static std::map < std::string, libmaus2::util::Destructable::shared_ptr_type > fmap;

static libmaus2::util::Destructable::shared_ptr_type getFunctionSet(std::string const & fn)
{
	libmaus2::parallel::ScopePosixMutex slock(libmaus2::lz::ZlibInterface::initlock);
	if ( fmap.find(fn) == fmap.end() )
	{
		libmaus2::util::Destructable::shared_ptr_type tptr = ZlibFunctions::sconstruct(fn);
		fmap[fn] = tptr;
	}
	assert ( fmap.find(fn) != fmap.end() );

	libmaus2::util::Destructable::shared_ptr_type tptr = fmap.find(fn)->second;

	return tptr;
}

libmaus2::lz::ZlibInterface::ZlibInterface(std::string const & libname) : context(createContext()), intf(getFunctionSet(libname))
{

}

libmaus2::lz::ZlibInterface::~ZlibInterface() {}

void libmaus2::lz::ZlibInterface::destructContext(void * context)
{
	delete reinterpret_cast<z_stream *>(context);
}

libmaus2::util::Destructable::unique_ptr_type libmaus2::lz::ZlibInterface::createContext()
{
	z_stream * strm = 0;

	try
	{
		strm = new z_stream();
		libmaus2::util::Destructable::unique_ptr_type tptr(libmaus2::util::Destructable::construct(strm,destructContext));
		return UNIQUE_PTR_MOVE(tptr);
	}
	catch(...)
	{
		if ( strm )
			delete strm;
		throw;
	}
}

libmaus2::lz::ZlibInterface::unique_ptr_type libmaus2::lz::ZlibInterface::construct(std::string const & libname)
{
	unique_ptr_type tptr(new this_type(libname));
	return UNIQUE_PTR_MOVE(tptr);
}

void libmaus2::lz::ZlibInterface::eraseContext()
{
	z_stream * strm = reinterpret_cast<z_stream *>(context->getObject());
	memset(strm,0,sizeof(z_stream));
}


void libmaus2::lz::ZlibInterface::setNextIn(unsigned char * p)
{
	z_stream * strm = reinterpret_cast<z_stream *>(context->getObject());
	strm->next_in = p;
}

void libmaus2::lz::ZlibInterface::setAvailIn(uint64_t const s)
{
	z_stream * strm = reinterpret_cast<z_stream *>(context->getObject());
	strm->avail_in = s;
}

void libmaus2::lz::ZlibInterface::setTotalIn(uint64_t const s)
{
	z_stream * strm = reinterpret_cast<z_stream *>(context->getObject());
	strm->total_in = s;
}

void libmaus2::lz::ZlibInterface::setNextOut(unsigned char * p)
{
	z_stream * strm = reinterpret_cast<z_stream *>(context->getObject());
	strm->next_out = p;
}

void libmaus2::lz::ZlibInterface::setAvailOut(uint64_t const s)
{
	z_stream * strm = reinterpret_cast<z_stream *>(context->getObject());
	strm->avail_out = s;
}

void libmaus2::lz::ZlibInterface::setTotalOut(uint64_t const s)
{
	z_stream * strm = reinterpret_cast<z_stream *>(context->getObject());
	strm->total_out = s;
}

unsigned char * libmaus2::lz::ZlibInterface::getNextOut()
{
	z_stream * strm = reinterpret_cast<z_stream *>(context->getObject());
	return strm->next_out;
}

uint64_t libmaus2::lz::ZlibInterface::getAvailOut() const
{
	z_stream * strm = reinterpret_cast<z_stream *>(context->getObject());
	return strm->avail_out;
}

uint64_t libmaus2::lz::ZlibInterface::getTotalOut() const
{
	z_stream * strm = reinterpret_cast<z_stream *>(context->getObject());
	return strm->total_out;
}

unsigned char * libmaus2::lz::ZlibInterface::getNextIn()
{
	z_stream * strm = reinterpret_cast<z_stream *>(context->getObject());
	return strm->next_in;
}

uint64_t libmaus2::lz::ZlibInterface::getAvailIn() const
{
	z_stream * strm = reinterpret_cast<z_stream *>(context->getObject());
	return strm->avail_in;
}

uint64_t libmaus2::lz::ZlibInterface::getTotalIn() const
{
	z_stream * strm = reinterpret_cast<z_stream *>(context->getObject());
	return strm->total_in;
}

void libmaus2::lz::ZlibInterface::setZAlloc(libmaus2::lz::ZlibInterface::alloc_function alloc)
{
	z_stream * strm = reinterpret_cast<z_stream *>(context->getObject());
	strm->zalloc = alloc;
}

libmaus2::lz::ZlibInterface::alloc_function libmaus2::lz::ZlibInterface::getZAlloc()
{
	z_stream * strm = reinterpret_cast<z_stream *>(context->getObject());
	return strm->zalloc;
}

void libmaus2::lz::ZlibInterface::setZFree(libmaus2::lz::ZlibInterface::free_function f)
{
	z_stream * strm = reinterpret_cast<z_stream *>(context->getObject());
	strm->zfree = f;
}

libmaus2::lz::ZlibInterface::free_function libmaus2::lz::ZlibInterface::getZFree()
{
	z_stream * strm = reinterpret_cast<z_stream *>(context->getObject());
	return strm->zfree;
}

void libmaus2::lz::ZlibInterface::setOpaque(void * p)
{
	z_stream * strm = reinterpret_cast<z_stream *>(context->getObject());
	strm->opaque = p;
}

void * libmaus2::lz::ZlibInterface::getOpaque()
{
	z_stream * strm = reinterpret_cast<z_stream *>(context->getObject());
	return strm->opaque;
}

char const * libmaus2::lz::ZlibInterface::getMsg() const
{
	z_stream * strm = reinterpret_cast<z_stream *>(context->getObject());
	return strm->msg;
}

int libmaus2::lz::ZlibInterface::z_inflateReset()
{
	//int (*p_inflateReset)(z_stream *) = reinterpret_cast< int (*)(z_stream *) >(zlib_inflateReset.func);
	z_stream * strm = reinterpret_cast<z_stream *>(context->getObject());
	return reinterpret_cast<ZlibFunctions *>(intf->getObject())->p_inflateReset(strm);
}

int libmaus2::lz::ZlibInterface::z_inflateEnd()
{
	//int (*p_inflateEnd)(z_stream *) = reinterpret_cast< int (*)(z_stream *) >(zlib_inflateEnd.func);
	z_stream * strm = reinterpret_cast<z_stream *>(context->getObject());
	return reinterpret_cast<ZlibFunctions *>(intf->getObject())->p_inflateEnd(strm);
}

int libmaus2::lz::ZlibInterface::z_inflateInit()
{
	//int (*p_inflateInit)(z_stream *, char const *, int) = reinterpret_cast< int (*)(z_stream *, char const *, int) >(zlib_inflateInit.func);
	z_stream * strm = reinterpret_cast<z_stream *>(context->getObject());
	return reinterpret_cast<ZlibFunctions *>(intf->getObject())->p_inflateInit(strm, ZLIB_VERSION, static_cast<int>(sizeof(z_stream)));
}

int libmaus2::lz::ZlibInterface::z_inflateInit2(int windowbits)
{
	//int (*p_inflateInit2)(z_stream *, int, char const *, int) = reinterpret_cast< int (*)(z_stream *, int, char const *, int) >(zlib_inflateInit2.func);
	z_stream * strm = reinterpret_cast<z_stream *>(context->getObject());
	return reinterpret_cast<ZlibFunctions *>(intf->getObject())->p_inflateInit2(strm, windowbits, ZLIB_VERSION, static_cast<int>(sizeof(z_stream)));
}

int libmaus2::lz::ZlibInterface::z_inflate(int flush)
{
	//int (*p_inflate)(z_stream *, int) = reinterpret_cast< int (*)(z_stream *, int) >(zlib_inflate.func);
	z_stream * strm = reinterpret_cast<z_stream *>(context->getObject());
	return reinterpret_cast<ZlibFunctions *>(intf->getObject())->p_inflate(strm,flush);
}

int libmaus2::lz::ZlibInterface::z_deflateReset()
{
	//int (*p_deflateReset)(z_stream *) = reinterpret_cast< int (*)(z_stream *) >(zlib_deflateReset.func);
	z_stream * strm = reinterpret_cast<z_stream *>(context->getObject());
	return reinterpret_cast<ZlibFunctions *>(intf->getObject())->p_deflateReset(strm);
}

int libmaus2::lz::ZlibInterface::z_deflateEnd()
{
	//int (*p_deflateEnd)(z_stream *) = reinterpret_cast< int (*)(z_stream *) >(zlib_deflateEnd.func);
	z_stream * strm = reinterpret_cast<z_stream *>(context->getObject());
	return reinterpret_cast<ZlibFunctions *>(intf->getObject())->p_deflateEnd(strm);
}

int libmaus2::lz::ZlibInterface::z_deflateInit(int level)
{
	//int (*p_deflateInit)(z_stream *, int, char const *, int) = reinterpret_cast< int (*)(z_stream *, int, char const *, int) >(zlib_deflateInit.func);
	z_stream * strm = reinterpret_cast<z_stream *>(context->getObject());
	return reinterpret_cast<ZlibFunctions *>(intf->getObject())->p_deflateInit(strm, level, ZLIB_VERSION, static_cast<int>(sizeof(z_stream)));
}

int libmaus2::lz::ZlibInterface::z_deflateInit2(int level, int method, int windowBits, int memLevel, int strategy)
{
	// int (*p_deflateInit2)(z_stream *, int, int, int, int, int, char const *, int) = reinterpret_cast< int (*)(z_stream *, int, int, int, int, int, char const *, int) >(zlib_deflateInit2.func);
	z_stream * strm = reinterpret_cast<z_stream *>(context->getObject());
	return reinterpret_cast<ZlibFunctions *>(intf->getObject())->p_deflateInit2(strm, level, method, windowBits, memLevel, strategy, ZLIB_VERSION, static_cast<int>(sizeof(z_stream)));
}

int libmaus2::lz::ZlibInterface::z_deflate(int flush)
{
	// int (*p_deflate)(z_stream *, int) = reinterpret_cast< int (*)(z_stream *, int) >(zlib_deflate.func);
	z_stream * strm = reinterpret_cast<z_stream *>(context->getObject());
	return reinterpret_cast<ZlibFunctions *>(intf->getObject())->p_deflate(strm,flush);
}

unsigned long libmaus2::lz::ZlibInterface::z_deflateBound(unsigned long in)
{
	//unsigned long (*p_deflateBound)(z_stream *, unsigned long) = reinterpret_cast< unsigned long (*)(z_stream *, unsigned long) >(zlib_deflateBound.func);
	z_stream * strm = reinterpret_cast<z_stream *>(context->getObject());
	return reinterpret_cast<ZlibFunctions *>(intf->getObject())->p_deflateBound(strm,in);
}
